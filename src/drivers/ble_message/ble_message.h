#pragma once

/**
 * @file ble_message.h
 * @brief user_message через NimBLE GATT VibroGuide (совместим с src_android).
 */

#include "esp_err.h"
#include "user_message/user_message.h"

esp_err_t ble_message_get(user_message_t *out);
