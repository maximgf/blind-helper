/**
 * @file sensor_registry.c
 * @brief Фабрика активного дальномера.
 */

#include "sensor_registry/sensor_registry.h"

#include "drivers/vl53l0x_gy530/vl53l0x_gy530.h"

esp_err_t distance_sensor_get_active(distance_sensor_t *out)
{
    return vl53l0x_gy530_get(out);
}
