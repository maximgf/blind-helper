/**
 * @file ble_message_gatt.c
 * @brief GATT VibroGuide: сервис 0000AA00 и характеристики AA01–AA05.
 */

#include "drivers/ble_message/ble_message_gatt.h"

#include "app_config.h"
#include "drivers/ble_message/ble_message_config.h"

#include <string.h>

#include "esp_log.h"
#include "host/ble_uuid.h"
#include "os/os_mbuf.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "BLE_GATT";

/** 0000AA00-0000-1000-8000-00805f9b34fb */
#define BLE_UUID_VG_BASE(uuid16)                                                                       \
    BLE_UUID128_INIT(0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00,         \
                     (uuid16) & 0xff, (uuid16) >> 8, 0x00, 0x00)

static const ble_uuid128_t s_svc_uuid = BLE_UUID_VG_BASE(0xAA00);
static const ble_uuid128_t s_dist_uuid = BLE_UUID_VG_BASE(0xAA01);
static const ble_uuid128_t s_sos_uuid = BLE_UUID_VG_BASE(0xAA02);
static const ble_uuid128_t s_status_uuid = BLE_UUID_VG_BASE(0xAA03);
static const ble_uuid128_t s_config_uuid = BLE_UUID_VG_BASE(0xAA04);
static const ble_uuid128_t s_battery_uuid = BLE_UUID_VG_BASE(0xAA05);

static uint16_t s_dist_val_handle;
static uint16_t s_sos_val_handle;
static uint16_t s_status_val_handle;
static uint16_t s_config_val_handle;
static uint16_t s_battery_val_handle;

static uint16_t s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static bool s_dist_notify;
static bool s_sos_notify;
static bool s_status_notify;
static bool s_battery_notify;

static uint16_t s_last_distance_cm;
static uint8_t s_last_sos;
static uint8_t s_last_status;
static uint8_t s_last_battery = BLE_MSG_BATTERY_DEFAULT_PERCENT;
static volatile bool s_sos_pending;

static int chr_notify(uint16_t val_handle, bool notify_enabled, const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) {
        return -1;
    }
    if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGW(TAG, "notify handle=%u: no connection", val_handle);
        return -1;
    }
    if (!notify_enabled) {
        ESP_LOGW(TAG, "notify handle=%u: CCCD not enabled", val_handle);
        return -1;
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (om == NULL) {
        return -1;
    }

    int rc = ble_gatts_notify_custom(s_conn_handle, val_handle, om);
    if (rc != 0) {
        ESP_LOGW(TAG, "notify handle=%u failed: %d", val_handle, rc);
    }
    return rc;
}

static int read_u16_le(struct ble_gatt_access_ctxt *ctxt, uint16_t value)
{
    uint8_t buf[2] = {(uint8_t)(value & 0xff), (uint8_t)((value >> 8) & 0xff)};
    return os_mbuf_append(ctxt->om, buf, sizeof(buf)) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int read_u8(struct ble_gatt_access_ctxt *ctxt, uint8_t value)
{
    return os_mbuf_append(ctxt->om, &value, 1) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int dist_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt,
                           void *arg)
{
    (void)conn_handle;
    (void)arg;

    if (attr_handle != s_dist_val_handle) {
        return BLE_ATT_ERR_UNLIKELY;
    }
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        return read_u16_le(ctxt, s_last_distance_cm);
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static int sos_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt,
                          void *arg)
{
    (void)conn_handle;
    (void)arg;

    if (attr_handle != s_sos_val_handle) {
        return BLE_ATT_ERR_UNLIKELY;
    }
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        return read_u8(ctxt, s_last_sos);
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static int status_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt,
                             void *arg)
{
    (void)conn_handle;
    (void)arg;

    if (attr_handle != s_status_val_handle) {
        return BLE_ATT_ERR_UNLIKELY;
    }
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        return read_u8(ctxt, s_last_status);
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static int config_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt,
                             void *arg)
{
    (void)conn_handle;
    (void)arg;

    if (attr_handle != s_config_val_handle) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    if (len < BLE_MSG_CONFIG_MIN_LEN) {
        ESP_LOGW(TAG, "config write too short: %u", len);
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    uint8_t buf[10];
    if (len > sizeof(buf)) {
        len = sizeof(buf);
    }
    int rc = ble_hs_mbuf_to_flat(ctxt->om, buf, len, NULL);
    if (rc != 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    const uint16_t safe_cm = (uint16_t)(buf[0] | (buf[1] << 8));
    const uint16_t warn_cm = (uint16_t)(buf[2] | (buf[3] << 8));
    const uint16_t near_cm = (uint16_t)(buf[4] | (buf[5] << 8));
    const uint16_t critical_cm = (uint16_t)(buf[6] | (buf[7] << 8));

    app_config_set_vibro_thresholds_cm(safe_cm, warn_cm, near_cm, critical_cm);
    ESP_LOGI(TAG, "config: safe=%u warn=%u near=%u critical=%u cm", safe_cm, warn_cm, near_cm,
             critical_cm);
    return 0;
}

static int battery_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt,
                              void *arg)
{
    (void)conn_handle;
    (void)arg;

    if (attr_handle != s_battery_val_handle) {
        return BLE_ATT_ERR_UNLIKELY;
    }
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        return read_u8(ctxt, s_last_battery);
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static const struct ble_gatt_svc_def s_gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &s_svc_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]){
                {
                    .uuid = &s_dist_uuid.u,
                    .access_cb = dist_chr_access,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                    .val_handle = &s_dist_val_handle,
                },
                {
                    .uuid = &s_sos_uuid.u,
                    .access_cb = sos_chr_access,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                    .val_handle = &s_sos_val_handle,
                },
                {
                    .uuid = &s_status_uuid.u,
                    .access_cb = status_chr_access,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                    .val_handle = &s_status_val_handle,
                },
                {
                    .uuid = &s_config_uuid.u,
                    .access_cb = config_chr_access,
                    .flags = BLE_GATT_CHR_F_WRITE,
                    .val_handle = &s_config_val_handle,
                },
                {
                    .uuid = &s_battery_uuid.u,
                    .access_cb = battery_chr_access,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                    .val_handle = &s_battery_val_handle,
                },
                {0},
            },
    },
    {0},
};

void ble_message_gatt_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    (void)arg;
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGD(TAG, "svc %s handle=%d", ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf), ctxt->svc.handle);
        break;
    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGI(TAG, "chr %s val_handle=%d", ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.val_handle);
        break;
    default:
        break;
    }
}

static void update_notify_flag(uint16_t attr_handle, bool enabled)
{
    if (attr_handle == s_dist_val_handle) {
        s_dist_notify = enabled;
    } else if (attr_handle == s_sos_val_handle) {
        s_sos_notify = enabled;
    } else if (attr_handle == s_status_val_handle) {
        s_status_notify = enabled;
    } else if (attr_handle == s_battery_val_handle) {
        s_battery_notify = enabled;
    }
}

void ble_message_gatt_subscribe_cb(struct ble_gap_event *event)
{
    s_conn_handle = event->subscribe.conn_handle;
    update_notify_flag(event->subscribe.attr_handle, event->subscribe.cur_notify);
    ESP_LOGI(TAG, "subscribe attr=%u notify=%d", event->subscribe.attr_handle,
             event->subscribe.cur_notify);
}

void ble_message_gatt_on_connect(uint16_t conn_handle)
{
    s_conn_handle = conn_handle;
    ESP_LOGI(TAG, "connected, handle=%u", conn_handle);
    (void)ble_message_gatt_notify_battery(s_last_battery);
}

void ble_message_gatt_on_disconnect(void)
{
    s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
    s_dist_notify = false;
    s_sos_notify = false;
    s_status_notify = false;
    s_battery_notify = false;
    s_sos_pending = false;
    ESP_LOGI(TAG, "disconnected");
}

int ble_message_gatt_init(void)
{
    ble_svc_gatt_init();
    int rc = ble_gatts_count_cfg(s_gatt_svcs);
    if (rc != 0) {
        return rc;
    }
    return ble_gatts_add_svcs(s_gatt_svcs);
}

uint8_t ble_message_gatt_status_from_hazard(hazard_state_t hazard, bool valid)
{
    uint8_t status = 0;
    if (valid) {
        status |= 0x01;
    }
    if (hazard == HAZARD_APPROACHING) {
        status |= 0x02;
    }
    if (hazard_filter_should_alert(hazard)) {
        status |= 0x04;
    }
    return status;
}

int ble_message_gatt_notify_distance(uint16_t distance_cm)
{
    s_last_distance_cm = distance_cm;
    uint8_t buf[2] = {(uint8_t)(distance_cm & 0xff), (uint8_t)((distance_cm >> 8) & 0xff)};
    return chr_notify(s_dist_val_handle, s_dist_notify, buf, sizeof(buf));
}

int ble_message_gatt_notify_sos(void)
{
    s_last_sos++;
    if (s_last_sos == 0) {
        s_last_sos = 1;
    }

    int rc = chr_notify(s_sos_val_handle, s_sos_notify, &s_last_sos, 1);
    if (rc != 0 && s_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        /* Подписка могла не попасть в событие — пробуем, если CCCD на телефоне включён. */
        rc = chr_notify(s_sos_val_handle, true, &s_last_sos, 1);
    }
    return rc;
}

bool ble_message_gatt_request_sos(void)
{
    if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGW(TAG, "SOS request: no BLE connection");
        return false;
    }
    s_sos_pending = true;
    return true;
}

void ble_message_gatt_flush_pending(void)
{
    if (!s_sos_pending) {
        return;
    }
    s_sos_pending = false;
    if (ble_message_gatt_notify_sos() != 0) {
        ESP_LOGW(TAG, "SOS notify failed");
    }
}

int ble_message_gatt_notify_status(uint8_t status)
{
    s_last_status = status;
    return chr_notify(s_status_val_handle, s_status_notify, &s_last_status, 1);
}

int ble_message_gatt_notify_battery(uint8_t percent)
{
    if (percent > 100) {
        percent = 100;
    }
    s_last_battery = percent;
    return chr_notify(s_battery_val_handle, s_battery_notify, &s_last_battery, 1);
}
