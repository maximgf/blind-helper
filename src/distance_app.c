#include "distance_app.h"

#include "app_config.h"
#include "distance_sensor.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "DIST_APP";

void distance_app_run(distance_sensor_t *sensor)
{
    ESP_LOGI(TAG, "Sensor: %s, interval %d ms", sensor->name, APP_MEASURE_INTERVAL_MS);

    while (true) {
        uint16_t mm = DISTANCE_MM_INVALID;
        esp_err_t err = distance_sensor_read_mm(sensor, &mm);

        if (err != ESP_OK || mm == DISTANCE_MM_INVALID) {
            ESP_LOGW(TAG, "Distance: invalid (%s)", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "Distance: %u mm (%.2f m)", mm, mm / 1000.0f);
        }

        vTaskDelay(pdMS_TO_TICKS(APP_MEASURE_INTERVAL_MS));
    }
}
