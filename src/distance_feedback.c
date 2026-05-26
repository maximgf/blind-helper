/**
 * @file distance_feedback.c
 * @brief Оркестрация: замер → приближается ли? → как часто мигать.
 */

#include "distance_feedback.h"

#include "app_config.h"
#include "distance_sensor.h"
#include "hazard_filter.h"
#include "sdkconfig.h"

#if CONFIG_BT_NIMBLE_ENABLED
#include "drivers/ble_message/ble_message_gatt.h"
#endif

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "DIST_FEEDBACK";

uint16_t distance_feedback_blink_period_ms(uint16_t mm)
{
    if (mm == DISTANCE_MM_INVALID || mm > app_config_zone_max_mm()) {
        return 0;
    }
    if (mm <= app_config_zone1_max_mm()) {
        return FEEDBACK_BLINK_PERIOD_ZONE_1_MS;
    }
    if (mm <= app_config_zone2_max_mm()) {
        return FEEDBACK_BLINK_PERIOD_ZONE_2_MS;
    }
    if (mm <= app_config_zone3_max_mm()) {
        return FEEDBACK_BLINK_PERIOD_ZONE_3_MS;
    }
    return FEEDBACK_BLINK_PERIOD_ZONE_4_MS;
}

/** Индикация только при APPROACHING; иначе LED/вибро выключены. */
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

#if CONFIG_BT_NIMBLE_ENABLED
        ble_message_gatt_flush_pending();
#endif

        /* Частый опрос: успеть заметить сближение между шагами пользователя. */
        if ((int32_t)(now - next_hazard) >= 0) {
            uint16_t mm = DISTANCE_MM_INVALID;
            esp_err_t err = distance_sensor_read_mm(sensor, &mm);
            bool valid = (err == ESP_OK && mm != DISTANCE_MM_INVALID);

            hazard_state_t hazard = hazard_filter_update(mm, valid);
            uint16_t period_ms = period_for_hazard(hazard, mm);
            ESP_ERROR_CHECK(feedback_output_set_blink_period_ms(feedback, period_ms));

            next_hazard = now + hazard_ticks;
        }

        /* Редкий лог для отладки на ПК, не влияет на индикацию. */
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

#if CONFIG_BT_NIMBLE_ENABLED
            {
                const bool valid = (mm != DISTANCE_MM_INVALID);
                const uint8_t status = ble_message_gatt_status_from_hazard(hazard, valid);
                (void)ble_message_gatt_notify_status(status);
                if (valid) {
                    const uint16_t distance_cm = (uint16_t)((mm + 5U) / 10U);
                    (void)ble_message_gatt_notify_distance(distance_cm);
                }
            }
#endif

            next_log = now + log_ticks;
        }

        /* Мигание обновляется независимо от опроса дальномера. */
        ESP_ERROR_CHECK(feedback_output_tick(feedback));
        vTaskDelay(feedback_step);
    }
}
