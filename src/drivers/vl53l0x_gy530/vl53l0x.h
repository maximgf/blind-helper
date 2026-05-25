#pragma once

/**
 * @file vl53l0x.h
 * @brief Низкоуровневый протокол чипа VL53L0X (регистры, калибровка, один замер).
 */

#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

/** Контекст датчика VL53L0X (GY-530). */
typedef struct {
    i2c_master_dev_handle_t i2c;
    uint8_t stop_variable;
    uint32_t measurement_timing_budget_us;
    uint32_t io_timeout_ms;
} vl53l0x_t;

/**
 * Инициализация и калибровка VL53L0X.
 * @param dev       контекст с уже настроенным i2c-хендлом
 * @param io_2v8    true — режим I/O 2.8 В (типично для модулей GY-530)
 */
esp_err_t vl53l0x_init(vl53l0x_t *dev, bool io_2v8);

/** Одиночное измерение дистанции, мм. При ошибке/таймауте — 65535. */
uint16_t vl53l0x_read_range_single_mm(vl53l0x_t *dev);
