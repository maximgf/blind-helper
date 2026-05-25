#include "distance_app.h"
#include "distance_sensor.h"
#include "esp_err.h"
#include "sensor_registry.h"

void app_main(void)
{
    distance_sensor_t sensor;

    ESP_ERROR_CHECK(distance_sensor_get_active(&sensor));
    ESP_ERROR_CHECK(distance_sensor_init(&sensor));
    distance_app_run(&sensor);
}
