#include "sensor_registry.h"

#include "drivers/vl53l0x_gy530/vl53l0x_gy530.h"

esp_err_t distance_sensor_get_active(distance_sensor_t *out)
{
    /* Точка подмены драйвера: замените вызов ниже на фабрику своего датчика. */
    return vl53l0x_gy530_get(out);
}
