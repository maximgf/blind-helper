#pragma once

/**
 * @file button_registry.h
 * @brief Какая кнопка установлена на плате.
 *
 * Прикладной код (button_notify) не меняется при смене GPIO/матрицы —
 * только реализация button_input_t и вызов здесь.
 */

#include "button_input/button_input.h"
#include "esp_err.h"

esp_err_t button_input_get_active(button_input_t *out);
