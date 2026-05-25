#pragma once

#include "distance_sensor.h"
#include "esp_err.h"

/** Заполняет @p out дескриптором драйвера GY-530 (VL53L0X). */
esp_err_t vl53l0x_gy530_get(distance_sensor_t *out);
