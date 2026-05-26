/**
 * @file ble_message.c
 * @brief user_message через NimBLE GATT VibroGuide (SOS notify).
 */

#include "drivers/ble_message/ble_message.h"

#include "drivers/ble_message/ble_message_config.h"
#include "drivers/ble_message/ble_message_gap.h"
#include "drivers/ble_message/ble_message_gatt.h"

#include "esp_err.h"
#include "esp_log.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "nvs_flash.h"

static const char *TAG = "BLE_MSG";

void ble_store_config_init(void);

static void on_stack_reset(int reason)
{
    ESP_LOGI(TAG, "nimble reset, reason=%d", reason);
}

static void on_stack_sync(void)
{
    ble_message_gap_adv_start();
}

static void nimble_host_config_init(void)
{
    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_stack_sync;
    ble_hs_cfg.gatts_register_cb = ble_message_gatt_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    ble_store_config_init();
}

static void nimble_host_task(void *param)
{
    (void)param;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static esp_err_t nimble_stack_start(void)
{
    esp_err_t err = nimble_port_init();
    if (err != ESP_OK) {
        return err;
    }

    if (ble_message_gap_init() != 0) {
        return ESP_FAIL;
    }
    if (ble_message_gatt_init() != 0) {
        return ESP_FAIL;
    }

    nimble_host_config_init();
    nimble_port_freertos_init(nimble_host_task);
    return ESP_OK;
}

static esp_err_t ble_message_init(user_message_t *self)
{
    (void)self;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        return err;
    }

    return nimble_stack_start();
}

static esp_err_t ble_message_send(user_message_t *self, const char *text)
{
    (void)self;

    ESP_LOGI(TAG, "%s", text != NULL ? text : "");
    if (!ble_message_gatt_request_sos()) {
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

static const user_message_t s_message = {
    .name = "BLE GATT",
    .init = ble_message_init,
    .deinit = NULL,
    .send = ble_message_send,
    .impl = NULL,
};

esp_err_t ble_message_get(user_message_t *out)
{
    if (!out) {
        return ESP_ERR_INVALID_ARG;
    }
    *out = s_message;
    return ESP_OK;
}
