/**
 * @file ble_message.c
 * @brief ESP-BLE-MESH узел: vendor notify при user_message_send (кнопка помощи).
 */

#include "drivers/ble_message/ble_message.h"

#include "drivers/ble_message/ble_message_config.h"
#include "drivers/ble_message/ble_mesh_bluetooth.h"

#include <inttypes.h>
#include <string.h>

#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_local_data_operation_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "BLE_MSG";

typedef struct __attribute__((packed)) {
    uint8_t event;
    uint8_t seq;
    uint8_t text_len;
    char text[BLE_MSG_MAX_TEXT_LEN];
} ble_help_payload_t;

static uint8_t s_dev_uuid[ESP_BLE_MESH_OCTET16_LEN] = {
    BLE_MSG_DEV_UUID_PREFIX_BYTE0,
    BLE_MSG_DEV_UUID_PREFIX_BYTE1,
};

static uint16_t s_net_idx = ESP_BLE_MESH_KEY_UNUSED;
static uint16_t s_app_idx = ESP_BLE_MESH_KEY_UNUSED;
static bool s_prov_complete = false;
static bool s_model_bound = false;
static uint8_t s_seq = 0;

static esp_ble_mesh_cfg_srv_t s_config_server = {
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay = ESP_BLE_MESH_RELAY_DISABLED,
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
    .default_ttl = 7,
};

static esp_ble_mesh_model_t s_root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&s_config_server),
};

static esp_ble_mesh_model_op_t s_vnd_op[] = {
    ESP_BLE_MESH_MODEL_OP(BLE_MSG_OP_HELP_NOTIFY, 3),
    ESP_BLE_MESH_MODEL_OP_END,
};

static esp_ble_mesh_model_t s_vnd_models[] = {
    ESP_BLE_MESH_VENDOR_MODEL(BLE_MSG_CID_ESP, BLE_MSG_VND_MODEL_ID_SERVER, s_vnd_op, NULL, NULL),
};

static esp_ble_mesh_elem_t s_elements[] = {
    ESP_BLE_MESH_ELEMENT(0, s_root_models, s_vnd_models),
};

static esp_ble_mesh_comp_t s_composition = {
    .cid = BLE_MSG_CID_ESP,
    .element_count = ARRAY_SIZE(s_elements),
    .elements = s_elements,
};

static esp_ble_mesh_prov_t s_provision = {
    .uuid = s_dev_uuid,
    .output_size = 0,
    .output_actions = 0,
};

static void on_prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index)
{
    s_net_idx = net_idx;
    s_prov_complete = true;
    ESP_LOGI(TAG, "Provisioned: net_idx 0x%04x, addr 0x%04x, flags 0x%02x, iv 0x%08" PRIx32, net_idx, addr,
             flags, iv_index);
    ESP_LOGI(TAG, "Bind AppKey to vendor model 0x%04x (CID 0x%04x), subscribe to group 0x%04x",
             BLE_MSG_VND_MODEL_ID_SERVER, BLE_MSG_CID_ESP, BLE_MSG_HELP_GROUP_ADDR);
}

static void ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event, esp_ble_mesh_prov_cb_param_t *param)
{
    switch (event) {
    case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT:
        on_prov_complete(param->node_prov_complete.net_idx, param->node_prov_complete.addr,
                         param->node_prov_complete.flags, param->node_prov_complete.iv_index);
        break;
    case ESP_BLE_MESH_NODE_PROV_RESET_EVT:
        s_prov_complete = false;
        s_model_bound = false;
        s_net_idx = ESP_BLE_MESH_KEY_UNUSED;
        s_app_idx = ESP_BLE_MESH_KEY_UNUSED;
        ESP_LOGW(TAG, "Provisioning reset");
        break;
    default:
        break;
    }
}

static void ble_mesh_config_server_cb(esp_ble_mesh_cfg_server_cb_event_t event,
                                      esp_ble_mesh_cfg_server_cb_param_t *param)
{
    if (event != ESP_BLE_MESH_CFG_SERVER_STATE_CHANGE_EVT) {
        return;
    }

    switch (param->ctx.recv_op) {
    case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
        s_app_idx = param->value.state_change.appkey_add.app_idx;
        s_net_idx = param->value.state_change.appkey_add.net_idx;
        ESP_LOGI(TAG, "AppKey added: net_idx 0x%04x, app_idx 0x%04x", s_net_idx, s_app_idx);
        break;
    case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
        if (param->value.state_change.mod_app_bind.company_id == BLE_MSG_CID_ESP &&
            param->value.state_change.mod_app_bind.model_id == BLE_MSG_VND_MODEL_ID_SERVER) {
            s_model_bound = true;
            ESP_LOGI(TAG, "Vendor model bound, app_idx 0x%04x",
                     param->value.state_change.mod_app_bind.app_idx);
        }
        break;
    default:
        break;
    }
}

static void ble_mesh_custom_model_cb(esp_ble_mesh_model_cb_event_t event, esp_ble_mesh_model_cb_param_t *param)
{
    if (event == ESP_BLE_MESH_MODEL_SEND_COMP_EVT) {
        if (param->model_send_comp.err_code != ESP_OK) {
            ESP_LOGW(TAG, "Send comp opcode 0x%06" PRIx32 " err %d", param->model_send_comp.opcode,
                     param->model_send_comp.err_code);
        } else {
            ESP_LOGD(TAG, "Send comp opcode 0x%06" PRIx32, param->model_send_comp.opcode);
        }
    }
}

static esp_err_t ble_mesh_stack_init(void)
{
    esp_err_t err;

    esp_ble_mesh_register_prov_callback(ble_mesh_provisioning_cb);
    esp_ble_mesh_register_config_server_callback(ble_mesh_config_server_cb);
    esp_ble_mesh_register_custom_model_callback(ble_mesh_custom_model_cb);

    err = esp_ble_mesh_init(&s_provision, &s_composition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_mesh_init: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_ble_mesh_node_prov_enable((esp_ble_mesh_prov_bearer_t)(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "node_prov_enable: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Mesh node advertising (PB-ADV | PB-GATT)");
    return ESP_OK;
}

static esp_err_t ble_message_send_help(const char *text)
{
    if (!s_prov_complete) {
        ESP_LOGW(TAG, "Not provisioned — message not sent over mesh");
        return ESP_ERR_INVALID_STATE;
    }
    if (s_app_idx == ESP_BLE_MESH_KEY_UNUSED) {
        ESP_LOGW(TAG, "No AppKey — bind key in nRF Mesh / your app");
        return ESP_ERR_INVALID_STATE;
    }
    if (!s_model_bound) {
        ESP_LOGW(TAG, "Vendor model not bound to AppKey yet");
        return ESP_ERR_INVALID_STATE;
    }

    ble_help_payload_t payload = {
        .event = BLE_MSG_EVENT_HELP_BUTTON,
        .seq = s_seq++,
        .text_len = 0,
    };

    if (text != NULL) {
        size_t len = strlen(text);
        if (len > BLE_MSG_MAX_TEXT_LEN) {
            len = BLE_MSG_MAX_TEXT_LEN;
        }
        payload.text_len = (uint8_t)len;
        memcpy(payload.text, text, len);
    }

    const uint16_t plen = (uint16_t)(3 + payload.text_len);

    esp_ble_mesh_msg_ctx_t ctx = {
        .net_idx = s_net_idx,
        .app_idx = s_app_idx,
        .addr = BLE_MSG_HELP_GROUP_ADDR,
        .send_ttl = s_config_server.default_ttl,
    };

    esp_err_t err = esp_ble_mesh_server_model_send_msg(&s_vnd_models[0], &ctx, BLE_MSG_OP_HELP_NOTIFY, plen,
                                                       (uint8_t *)&payload);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "mesh send failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Mesh HELP notify seq=%u, group 0x%04x, len=%u", payload.seq, BLE_MSG_HELP_GROUP_ADDR, plen);
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

    err = bluetooth_init();
    if (err != ESP_OK) {
        return err;
    }

    ble_mesh_get_dev_uuid(s_dev_uuid);

    return ble_mesh_stack_init();
}

static esp_err_t ble_message_send(user_message_t *self, const char *text)
{
    (void)self;

    ESP_LOGI(TAG, "%s", text != NULL ? text : "");

    return ble_message_send_help(text);
}

static const user_message_t s_message = {
    .name = "BLE Mesh",
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
