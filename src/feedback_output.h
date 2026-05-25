#pragma once

/**
 * @file feedback_output.h
 * @brief Единый интерфейс тактильной/световой обратной связи (LED, вибромотор).
 *
 * Прикладной код задаёт период пульсации; реализация управляет GPIO (или PWM в будущем).
 * Конкретный исполнитель подключается через feedback_registry.c.
 */

#include <stdint.h>

#include "esp_err.h"

struct feedback_output;

typedef esp_err_t (*feedback_output_init_fn)(struct feedback_output *self);
typedef esp_err_t (*feedback_output_deinit_fn)(struct feedback_output *self);
/** @param period_ms полный период цикла вкл/выкл (мс); 0 — выключено. */
typedef esp_err_t (*feedback_output_set_period_fn)(struct feedback_output *self, uint16_t period_ms);
/** Вызывать периодически из главного цикла для поддержания мигания. */
typedef esp_err_t (*feedback_output_tick_fn)(struct feedback_output *self);

typedef struct feedback_output {
    const char *name;
    feedback_output_init_fn init;
    feedback_output_deinit_fn deinit;
    feedback_output_set_period_fn set_blink_period_ms;
    feedback_output_tick_fn tick;
    void *impl;
} feedback_output_t;

static inline esp_err_t feedback_output_init(feedback_output_t *out)
{
    if (!out || !out->init) {
        return ESP_ERR_INVALID_ARG;
    }
    return out->init(out);
}

static inline esp_err_t feedback_output_deinit(feedback_output_t *out)
{
    if (!out || !out->deinit) {
        return ESP_OK;
    }
    return out->deinit(out);
}

static inline esp_err_t feedback_output_set_blink_period_ms(feedback_output_t *out, uint16_t period_ms)
{
    if (!out || !out->set_blink_period_ms) {
        return ESP_ERR_INVALID_ARG;
    }
    return out->set_blink_period_ms(out, period_ms);
}

static inline esp_err_t feedback_output_tick(feedback_output_t *out)
{
    if (!out || !out->tick) {
        return ESP_ERR_INVALID_ARG;
    }
    return out->tick(out);
}
