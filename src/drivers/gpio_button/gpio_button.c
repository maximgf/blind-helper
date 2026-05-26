/**
 * @file gpio_button.c
 * @brief Опрос GPIO с антидребезгом; одно событие на физическое нажатие.
 */

#include "drivers/gpio_button/gpio_button.h"

#include "drivers/gpio_button/gpio_button_config.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_timer.h"

typedef enum {
    GPIO_BUTTON_STATE_IDLE,
    GPIO_BUTTON_STATE_DEBOUNCE,
    GPIO_BUTTON_STATE_HELD,
} gpio_button_state_t;

typedef struct {
    gpio_button_state_t state;
    int64_t debounce_start_us;
} gpio_button_ctx_t;

static gpio_button_ctx_t s_ctx;

static bool is_pressed(void)
{
    int level = gpio_get_level(GPIO_BUTTON_GPIO);
#if GPIO_BUTTON_ACTIVE_LOW
    return level == 0;
#else
    return level != 0;
#endif
}

static esp_err_t gpio_button_init(button_input_t *self)
{
    (void)self;

    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << GPIO_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
#if GPIO_BUTTON_ACTIVE_LOW
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
#else
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
#endif
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&cfg), "GPIO_BUTTON", "gpio_config");

    s_ctx.state = GPIO_BUTTON_STATE_IDLE;
    s_ctx.debounce_start_us = 0;
    return ESP_OK;
}

static bool gpio_button_poll_pressed(button_input_t *self)
{
    (void)self;

    const bool pressed = is_pressed();
    const int64_t now_us = esp_timer_get_time();
    const int64_t debounce_us = (int64_t)GPIO_BUTTON_DEBOUNCE_MS * 1000;

    switch (s_ctx.state) {
    case GPIO_BUTTON_STATE_IDLE:
        if (pressed) {
            s_ctx.state = GPIO_BUTTON_STATE_DEBOUNCE;
            s_ctx.debounce_start_us = now_us;
        }
        break;

    case GPIO_BUTTON_STATE_DEBOUNCE:
        if (!pressed) {
            s_ctx.state = GPIO_BUTTON_STATE_IDLE;
        } else if ((now_us - s_ctx.debounce_start_us) >= debounce_us) {
            s_ctx.state = GPIO_BUTTON_STATE_HELD;
            return true;
        }
        break;

    case GPIO_BUTTON_STATE_HELD:
        if (!pressed) {
            s_ctx.state = GPIO_BUTTON_STATE_IDLE;
        }
        break;
    }

    return false;
}

static const button_input_t s_input = {
    .name = "GPIO button",
    .init = gpio_button_init,
    .deinit = NULL,
    .poll_pressed = gpio_button_poll_pressed,
    .impl = &s_ctx,
};

esp_err_t gpio_button_get(button_input_t *out)
{
    if (!out) {
        return ESP_ERR_INVALID_ARG;
    }
    *out = s_input;
    return ESP_OK;
}
