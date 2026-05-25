#include "distance_feedback.h"

#include "app_config.h"
#include "distance_sensor.h"
#include "hazard_filter.h"

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

static uint16_t period_for_hazard(hazard_state_t hazard, uint16_t mm)
{
    if (!hazard_filter_should_alert(hazard)) {
        return 0;
    }
    return distance_feedback_blink_period_ms(mm);
}

void distance_feedback_run(distance_sensor_t *sensor, feedback_output_t *feedback)
{
    ESP_LOGI(TAG, "Sensor: %s, feedback: %s, log %d ms, hazard %d ms", sensor->name, feedback->name,
             APP_MEASURE_INTERVAL_MS, APP_HAZARD_SAMPLE_INTERVAL_MS);

    hazard_filter_reset();

    const TickType_t log_ticks = pdMS_TO_TICKS(APP_MEASURE_INTERVAL_MS);
    const TickType_t hazard_ticks = pdMS_TO_TICKS(APP_HAZARD_SAMPLE_INTERVAL_MS);
    const TickType_t feedback_ticks = pdMS_TO_TICKS(APP_FEEDBACK_TICK_MS);
    const TickType_t feedback_step = feedback_ticks > 0 ? feedback_ticks : 1;

    TickType_t next_log = xTaskGetTickCount();
    TickType_t next_hazard = xTaskGetTickCount();

    while (true) {
        TickType_t now = xTaskGetTickCount();

        if ((int32_t)(now - next_hazard) >= 0) {
            uint16_t mm = DISTANCE_MM_INVALID;
            esp_err_t err = distance_sensor_read_mm(sensor, &mm);
            bool valid = (err == ESP_OK && mm != DISTANCE_MM_INVALID);

            hazard_state_t hazard = hazard_filter_update(mm, valid);
            uint16_t period_ms = period_for_hazard(hazard, mm);
            ESP_ERROR_CHECK(feedback_output_set_blink_period_ms(feedback, period_ms));

            next_hazard = now + hazard_ticks;
        }

        if ((int32_t)(now - next_log) >= 0) {
            uint16_t mm = hazard_filter_get_last_mm();
            hazard_state_t hazard = hazard_filter_get_state();
            int32_t velocity = hazard_filter_get_velocity_mm_s();
            uint16_t period_ms = period_for_hazard(hazard, mm);

            if (mm == DISTANCE_MM_INVALID) {
                ESP_LOGW(TAG, "Distance: invalid, hazard %s", hazard_filter_state_name(hazard));
            } else {
                ESP_LOGI(TAG, "Distance: %u mm (%.2f m), v %ld mm/s, hazard %s, blink %u ms", mm,
                         mm / 1000.0f, (long)velocity, hazard_filter_state_name(hazard), period_ms);
            }

            next_log = now + log_ticks;
        }

        ESP_ERROR_CHECK(feedback_output_tick(feedback));
        vTaskDelay(feedback_step);
    }
}
