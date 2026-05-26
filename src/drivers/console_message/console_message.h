#pragma once

/**
 * @file console_message.h
 * @brief Вывод сообщений в UART (монитор PlatformIO).
 */

#include "esp_err.h"
#include "user_message.h"

esp_err_t console_message_get(user_message_t *out);
