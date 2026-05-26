#pragma once

/**
 * @file user_message.h
 * @brief Канал сообщений пользователю (сопровождающему, телефону).
 *
 * Сейчас — UART/монитор; позже — BLE без правок прикладной логики кнопки.
 */

#include "esp_err.h"

struct user_message;

typedef esp_err_t (*user_message_init_fn)(struct user_message *self);
typedef esp_err_t (*user_message_deinit_fn)(struct user_message *self);
typedef esp_err_t (*user_message_send_fn)(struct user_message *self, const char *text);

typedef struct user_message {
    const char *name;
    user_message_init_fn init;
    user_message_deinit_fn deinit;
    user_message_send_fn send;
    void *impl;
} user_message_t;

static inline esp_err_t user_message_init(user_message_t *msg)
{
    if (!msg || !msg->init) {
        return ESP_ERR_INVALID_ARG;
    }
    return msg->init(msg);
}

static inline esp_err_t user_message_deinit(user_message_t *msg)
{
    if (!msg || !msg->deinit) {
        return ESP_OK;
    }
    return msg->deinit(msg);
}

static inline esp_err_t user_message_send(user_message_t *msg, const char *text)
{
    if (!msg || !msg->send || !text) {
        return ESP_ERR_INVALID_ARG;
    }
    return msg->send(msg, text);
}
