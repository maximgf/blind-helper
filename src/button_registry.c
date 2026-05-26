/**
 * @file button_registry.c
 * @brief Фабрика активной кнопки.
 */

#include "button_registry.h"

#include "drivers/gpio_button/gpio_button.h"

esp_err_t button_input_get_active(button_input_t *out)
{
    return gpio_button_get(out);
}
