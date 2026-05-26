#pragma once

/**
 * @file message_registry.h
 * @brief Куда уходит сообщение пользователю (консоль, позже BLE).
 *
 * Единственное место замены «UART → BLE» для всего приложения.
 */

#include "esp_err.h"
#include "user_message.h"

esp_err_t user_message_get_active(user_message_t *out);
