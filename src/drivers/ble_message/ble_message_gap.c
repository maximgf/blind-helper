/**
 * @file ble_message_gap.c
 * @brief Реклама и события GAP (NimBLE).
 */

#include "drivers/ble_message/ble_message_gap.h"

#include "drivers/ble_message/ble_message_config.h"
#include "drivers/ble_message/ble_message_gatt.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"

static const char *TAG = "BLE_GAP";

static uint8_t s_own_addr_type;
static uint8_t s_addr_val[6];

static const ble_uuid16_t s_adv_svc_uuid = BLE_UUID16_INIT(0xAA00);

static int gap_event_handler(struct ble_gap_event *event, void *arg);

static void format_addr(char *addr_str, const uint8_t addr[])
{
    sprintf(addr_str, "%02X:%02X:%02X:%02X:%02X:%02X", addr[0], addr[1], addr[2], addr[3], addr[4],
            addr[5]);
}

static void start_advertising(void)
{
    const char *name = ble_svc_gap_device_name();
    struct ble_hs_adv_fields adv_fields = {0};
    struct ble_gap_adv_params adv_params = {0};
    int rc;

    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    adv_fields.name = (uint8_t *)name;
    adv_fields.name_len = strlen(name);
    adv_fields.name_is_complete = 1;
    adv_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    adv_fields.tx_pwr_lvl_is_present = 1;
    adv_fields.uuids16 = (ble_uuid16_t *)&s_adv_svc_uuid;
    adv_fields.num_uuids16 = 1;
    adv_fields.uuids16_is_complete = 1;

    rc = ble_gap_adv_set_fields(&adv_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv_set_fields failed: %d", rc);
        return;
    }

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = BLE_GAP_ADV_ITVL_MS(200);
    adv_params.itvl_max = BLE_GAP_ADV_ITVL_MS(220);

    rc = ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, gap_event_handler, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv_start failed: %d", rc);
        return;
    }
    ESP_LOGI(TAG, "advertising as \"%s\"", name);
}

static int gap_event_handler(struct ble_gap_event *event, void *arg)
{
    (void)arg;

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI(TAG, "connect %s; status=%d", event->connect.status == 0 ? "ok" : "fail",
                 event->connect.status);
        if (event->connect.status == 0) {
            ble_message_gatt_on_connect(event->connect.conn_handle);
        } else {
            start_advertising();
        }
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "disconnect; reason=%d", event->disconnect.reason);
        ble_message_gatt_on_disconnect();
        start_advertising();
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        start_advertising();
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        ble_message_gatt_subscribe_cb(event);
        break;
    default:
        break;
    }
    return 0;
}

void ble_message_gap_adv_start(void)
{
    char addr_str[18];
    int rc;

    rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "no BT address");
        return;
    }

    rc = ble_hs_id_infer_auto(0, &s_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "infer addr type failed: %d", rc);
        return;
    }

    rc = ble_hs_id_copy_addr(s_own_addr_type, s_addr_val, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "copy addr failed: %d", rc);
        return;
    }

    format_addr(addr_str, s_addr_val);
    ESP_LOGI(TAG, "address %s", addr_str);

    start_advertising();
}

int ble_message_gap_init(void)
{
    int rc;

    ble_svc_gap_init();
    rc = ble_svc_gap_device_name_set(BLE_MSG_DEVICE_NAME);
    if (rc != 0) {
        ESP_LOGE(TAG, "device name set failed: %d", rc);
        return rc;
    }
    return 0;
}
