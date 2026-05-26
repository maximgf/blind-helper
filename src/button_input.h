#pragma once

/**
 * @file button_input.h
 * @brief Абстракция пользовательской кнопки.
 *
 * Прикладной код не знает тип GPIO — только poll_pressed() (одно срабатывание на нажатие).
 * Реализация подставляется в button_registry.c.
 */

#include <stdbool.h>

#include "esp_err.h"

struct button_input;

typedef esp_err_t (*button_input_init_fn)(struct button_input *self);
typedef esp_err_t (*button_input_deinit_fn)(struct button_input *self);
/** true — зафиксировано одно нажатие с момента прошлого опроса. */
typedef bool (*button_input_poll_pressed_fn)(struct button_input *self);

typedef struct button_input {
    const char *name;
    button_input_init_fn init;
    button_input_deinit_fn deinit;
    button_input_poll_pressed_fn poll_pressed;
    void *impl;
} button_input_t;

static inline esp_err_t button_input_init(button_input_t *btn)
{
    if (!btn || !btn->init) {
        return ESP_ERR_INVALID_ARG;
    }
    return btn->init(btn);
}

static inline esp_err_t button_input_deinit(button_input_t *btn)
{
    if (!btn || !btn->deinit) {
        return ESP_OK;
    }
    return btn->deinit(btn);
}

static inline bool button_input_poll_pressed(button_input_t *btn)
{
    if (!btn || !btn->poll_pressed) {
        return false;
    }
    return btn->poll_pressed(btn);
}
