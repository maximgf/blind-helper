#pragma once

/**
 * @file feedback_registry.h
 * @brief Выбор активного исполнителя обратной связи (LED / вибромотор).
 *
 * ## Как подключить вибромотор
 *
 * 1. Создайте `src/drivers/feedback_vibrator/` по образцу `feedback_led/`.
 * 2. Реализуйте @ref feedback_output_t (init, set_blink_period_ms, tick).
 * 3. В `feedback_registry.c` замените фабрику на `feedback_vibrator_get`.
 */

#include "esp_err.h"
#include "feedback_output.h"

esp_err_t feedback_output_get_active(feedback_output_t *out);
