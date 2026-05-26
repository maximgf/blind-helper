#pragma once

/**
 * @file message_registry.h
 * @brief Куда уходит сообщение пользователю (консоль, позже BLE).
 *
 * При CONFIG_BLE_MESH — BLE Mesh (drivers/ble_message), иначе UART.
 */

#include "esp_err.h"
#include "user_message.h"

esp_err_t user_message_get_active(user_message_t *out);
