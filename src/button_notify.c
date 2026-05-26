/**
 * @file button_notify.c
 * @brief Опрос кнопки и вызов user_message_send при нажатии.
 */

#include "button_notify.h"

#include "app_config.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BTN_NOTIFY";

typedef struct {
    button_input_t button;
    user_message_t message;
} button_notify_ctx_t;

static button_notify_ctx_t s_ctx;

static void button_notify_task(void *arg)
{
    (void)arg;

    ESP_LOGI(TAG, "Button: %s, message: %s, poll %d ms", s_ctx.button.name, s_ctx.message.name,
             APP_BUTTON_POLL_MS);

    const TickType_t poll_ticks = pdMS_TO_TICKS(APP_BUTTON_POLL_MS);

    while (true) {
        if (button_input_poll_pressed(&s_ctx.button)) {
            esp_err_t err = user_message_send(&s_ctx.message, APP_BUTTON_MESSAGE);
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "send failed: %s", esp_err_to_name(err));
            }
        }
        vTaskDelay(poll_ticks);
    }
}

esp_err_t button_notify_start(button_input_t *button, user_message_t *message)
{
    if (!button || !message) {
        return ESP_ERR_INVALID_ARG;
    }

    s_ctx.button = *button;
    s_ctx.message = *message;

    const BaseType_t ok = xTaskCreate(button_notify_task, "btn_notify", 3072, NULL, 5, NULL);
    if (ok != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
