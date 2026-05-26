#pragma once

/**
 * @file vl53l0x_gy530.h
 * @brief Адаптер модуля GY-530 (чип VL53L0X) к distance_sensor_t.
 */

#include "distance_sensor/distance_sensor.h"
#include "esp_err.h"

esp_err_t vl53l0x_gy530_get(distance_sensor_t *out);
