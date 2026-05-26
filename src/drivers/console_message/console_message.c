/**
 * @file console_message.c
 * @brief Реализация user_message через ESP_LOG (видно в serial monitor).
 */

#include "drivers/console_message/console_message.h"

#include "esp_log.h"

static const char *TAG = "USER_MSG";

static esp_err_t console_message_init(user_message_t *self)
{
    (void)self;
    return ESP_OK;
}

static esp_err_t console_message_send(user_message_t *self, const char *text)
{
    (void)self;
    ESP_LOGI(TAG, "%s", text);
    return ESP_OK;
}

static const user_message_t s_message = {
    .name = "Console",
    .init = console_message_init,
    .deinit = NULL,
    .send = console_message_send,
    .impl = NULL,
};

esp_err_t console_message_get(user_message_t *out)
{
    if (!out) {
        return ESP_ERR_INVALID_ARG;
    }
    *out = s_message;
    return ESP_OK;
}
