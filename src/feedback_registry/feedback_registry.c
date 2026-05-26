/**
 * @file feedback_registry.c
 * @brief Фабрика активного индикатора обратной связи.
 */

#include "feedback_registry/feedback_registry.h"

#include "drivers/feedback_led/feedback_led.h"

esp_err_t feedback_output_get_active(feedback_output_t *out)
{
    return feedback_led_get(out);
}
