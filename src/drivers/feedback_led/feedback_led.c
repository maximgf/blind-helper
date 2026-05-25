#include "drivers/feedback_led/feedback_led.h"

#include "drivers/feedback_led/feedback_led_config.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_timer.h"

typedef struct {
    uint16_t period_ms;
    bool level_on;
    int64_t last_toggle_us;
} feedback_led_ctx_t;

static feedback_led_ctx_t s_ctx;

static void apply_level(const feedback_led_ctx_t *ctx)
{
    gpio_set_level(FEEDBACK_LED_GPIO, ctx->level_on ? 1 : 0);
}

static esp_err_t feedback_led_init(feedback_output_t *self)
{
    (void)self;

    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << FEEDBACK_LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&cfg), "FEEDBACK_LED", "gpio_config");

    s_ctx.period_ms = 0;
    s_ctx.level_on = false;
    s_ctx.last_toggle_us = esp_timer_get_time();
    apply_level(&s_ctx);
    return ESP_OK;
}

static esp_err_t feedback_led_set_period(feedback_output_t *self, uint16_t period_ms)
{
    (void)self;

    s_ctx.period_ms = period_ms;
    if (period_ms == 0) {
        s_ctx.level_on = false;
        apply_level(&s_ctx);
        s_ctx.last_toggle_us = esp_timer_get_time();
    }
    return ESP_OK;
}

static esp_err_t feedback_led_tick(feedback_output_t *self)
{
    (void)self;

    if (s_ctx.period_ms == 0) {
        return ESP_OK;
    }

    uint32_t half_period_us = (uint32_t)s_ctx.period_ms * 500U;
    int64_t now = esp_timer_get_time();
    if ((uint32_t)(now - s_ctx.last_toggle_us) >= half_period_us) {
        s_ctx.level_on = !s_ctx.level_on;
        apply_level(&s_ctx);
        s_ctx.last_toggle_us = now;
    }
    return ESP_OK;
}

static const feedback_output_t s_output = {
    .name = "LED",
    .init = feedback_led_init,
    .deinit = NULL,
    .set_blink_period_ms = feedback_led_set_period,
    .tick = feedback_led_tick,
    .impl = &s_ctx,
};

esp_err_t feedback_led_get(feedback_output_t *out)
{
    if (!out) {
        return ESP_ERR_INVALID_ARG;
    }
    *out = s_output;
    return ESP_OK;
}
