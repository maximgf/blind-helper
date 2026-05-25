#include "distance_feedback.h"

#include "app_config.h"
#include "distance_sensor.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "DIST_FEEDBACK";

uint16_t distance_feedback_blink_period_ms(uint16_t mm)
{
    if (mm == DISTANCE_MM_INVALID || mm > DISTANCE_ZONE_MAX_MM) {
        return 0;
    }
    if (mm <= DISTANCE_ZONE_1_MAX_MM) {
        return FEEDBACK_BLINK_PERIOD_ZONE_1_MS;
    }
    if (mm <= DISTANCE_ZONE_2_MAX_MM) {
        return FEEDBACK_BLINK_PERIOD_ZONE_2_MS;
    }
    if (mm <= DISTANCE_ZONE_3_MAX_MM) {
        return FEEDBACK_BLINK_PERIOD_ZONE_3_MS;
    }
    return FEEDBACK_BLINK_PERIOD_ZONE_4_MS;
}

void distance_feedback_run(distance_sensor_t *sensor, feedback_output_t *feedback)
{
    ESP_LOGI(TAG, "Sensor: %s, feedback: %s, interval %d ms", sensor->name, feedback->name,
             APP_MEASURE_INTERVAL_MS);

    const TickType_t measure_ticks = pdMS_TO_TICKS(APP_MEASURE_INTERVAL_MS);
    const TickType_t feedback_ticks = pdMS_TO_TICKS(APP_FEEDBACK_TICK_MS);
    const TickType_t feedback_step = feedback_ticks > 0 ? feedback_ticks : 1;

    while (true) {
        uint16_t mm = DISTANCE_MM_INVALID;
        esp_err_t err = distance_sensor_read_mm(sensor, &mm);
        uint16_t period_ms = 0;

        if (err != ESP_OK || mm == DISTANCE_MM_INVALID) {
            ESP_LOGW(TAG, "Distance: invalid (%s)", esp_err_to_name(err));
        } else {
            period_ms = distance_feedback_blink_period_ms(mm);
            ESP_LOGI(TAG, "Distance: %u mm (%.2f m), blink %u ms", mm, mm / 1000.0f, period_ms);
        }

        ESP_ERROR_CHECK(feedback_output_set_blink_period_ms(feedback, period_ms));

        TickType_t deadline = xTaskGetTickCount() + measure_ticks;
        while ((int32_t)(deadline - xTaskGetTickCount()) > 0) {
            ESP_ERROR_CHECK(feedback_output_tick(feedback));
            vTaskDelay(feedback_step);
        }
    }
}
