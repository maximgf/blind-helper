#pragma once

/**
 * @file distance_sensor.h
 * @brief Абстракция лазерного дальномера (мм до препятствия в луче).
 *
 * Прикладной код не знает модель чипа — только read_mm().
 * Реализация подставляется в sensor_registry.c (сейчас VL53L0X на GY-530).
 */

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/** Нет достоверного замера (препятствие вне зоны, блик, сбой I2C). */
#define DISTANCE_MM_INVALID 65535

struct distance_sensor;

typedef esp_err_t (*distance_sensor_init_fn)(struct distance_sensor *self);
typedef esp_err_t (*distance_sensor_deinit_fn)(struct distance_sensor *self);
typedef esp_err_t (*distance_sensor_read_mm_fn)(struct distance_sensor *self, uint16_t *mm_out);

/** Виртуальная таблица драйвера; impl — внутреннее состояние (шина I2C и т.д.). */
typedef struct distance_sensor {
    const char *name;
    distance_sensor_init_fn init;
    distance_sensor_deinit_fn deinit;
    distance_sensor_read_mm_fn read_mm;
    void *impl;
} distance_sensor_t;

static inline esp_err_t distance_sensor_init(distance_sensor_t *sensor)
{
    if (!sensor || !sensor->init) {
        return ESP_ERR_INVALID_ARG;
    }
    return sensor->init(sensor);
}

static inline esp_err_t distance_sensor_deinit(distance_sensor_t *sensor)
{
    if (!sensor || !sensor->deinit) {
        return ESP_OK;
    }
    return sensor->deinit(sensor);
}

static inline esp_err_t distance_sensor_read_mm(distance_sensor_t *sensor, uint16_t *mm_out)
{
    if (!sensor || !sensor->read_mm || !mm_out) {
        return ESP_ERR_INVALID_ARG;
    }
    return sensor->read_mm(sensor, mm_out);
}
