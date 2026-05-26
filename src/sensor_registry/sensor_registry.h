#pragma once

/**
 * @file sensor_registry.h
 * @brief Какой дальномер установлен на плате.
 *
 * Прикладной код (distance_feedback) не меняется при смене модуля ToF/ультразвука —
 * только реализация distance_sensor_t и вызов здесь.
 */

#include "distance_sensor/distance_sensor.h"
#include "esp_err.h"

esp_err_t distance_sensor_get_active(distance_sensor_t *out);
