#pragma once

/**
 * @file gpio_button.h
 * @brief Драйвер кнопки на GPIO как исполнитель button_input.
 */

#include "button_input/button_input.h"
#include "esp_err.h"

esp_err_t gpio_button_get(button_input_t *out);
