#pragma once

/**
 * @file ble_message_config.h
 * @brief Идентификаторы BLE Mesh для события «кнопка помощи».
 *
 * Company ID 0x02E5 — Espressif (как в примерах IDF); для своего приложения
 * позже можно заменить на выделенный SIG Company ID.
 */

#include <stdint.h>

#include "esp_ble_mesh_defs.h"

#define BLE_MSG_CID_ESP                     0x02E5

/** Vendor model ID на узле (сервер). */
#define BLE_MSG_VND_MODEL_ID_SERVER         0x0001

/** Групповой адрес публикации (подписка на телефоне / provisioner). */
#define BLE_MSG_HELP_GROUP_ADDR             0xC000

/** UUID-префикс непровиженного устройства (2 байта + MAC в ble_mesh_get_dev_uuid). */
#define BLE_MSG_DEV_UUID_PREFIX_BYTE0       0xB1
#define BLE_MSG_DEV_UUID_PREFIX_BYTE1       0x48

/** Opcodes vendor model (3-octet, company ID в младших битах). */
#define BLE_MSG_OP_HELP_NOTIFY              ESP_BLE_MESH_MODEL_OP_3(0x10, BLE_MSG_CID_ESP)

#define BLE_MSG_MAX_TEXT_LEN                40

/** Тип события в payload. */
#define BLE_MSG_EVENT_HELP_BUTTON           0x01
