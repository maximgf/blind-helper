/**
 * @file main.c
 * @brief Точка входа: инициализация дальномера и индикатора, запуск прикладного цикла.
 *
 * Устройство для ориентирования: лазерный дальномер «видит» препятствие впереди,
 * LED/вибромотор сигнализирует только при приближении (см. hazard_filter).
 */

#include "distance_feedback.h"
#include "distance_sensor.h"
#include "esp_err.h"
#include "feedback_output.h"
#include "feedback_registry.h"
#include "sensor_registry.h"

void app_main(void)
{
    distance_sensor_t sensor;
    feedback_output_t feedback;

    ESP_ERROR_CHECK(distance_sensor_get_active(&sensor));
    ESP_ERROR_CHECK(feedback_output_get_active(&feedback));
    ESP_ERROR_CHECK(distance_sensor_init(&sensor));
    ESP_ERROR_CHECK(feedback_output_init(&feedback));

    /* Блокирующий цикл: опрос → фильтр → мигание. */
    distance_feedback_run(&sensor, &feedback);
}
