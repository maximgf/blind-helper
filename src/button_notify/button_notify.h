#pragma once

/**
 * @file button_notify.h
 * @brief По нажатию кнопки — отправить сообщение в активный канал.
 */

#include "button_input/button_input.h"
#include "esp_err.h"
#include "user_message/user_message.h"

/** Запускает фоновую задачу опроса (не блокирует distance_feedback). */
esp_err_t button_notify_start(button_input_t *button, user_message_t *message);
