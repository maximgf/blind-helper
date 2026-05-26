#pragma once

/**
 * @file ble_message.h
 * @brief user_message через ESP-BLE-MESH vendor model.
 */

#include "esp_err.h"
#include "user_message.h"

esp_err_t ble_message_get(user_message_t *out);
