#include "feedback_registry.h"

#include "drivers/feedback_led/feedback_led.h"

esp_err_t feedback_output_get_active(feedback_output_t *out)
{
    /* Точка подмены: feedback_vibro_get(out) вместо LED. */
    return feedback_led_get(out);
}
