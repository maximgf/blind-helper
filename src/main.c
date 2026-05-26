/**
 * @file main.c
 * @brief Точка входа: дальномер, индикатор, кнопка → сообщение, цикл обратной связи.
 *
 * Устройство для ориентирования: лазерный дальномер «видит» препятствие впереди,
 * LED/вибромотор сигнализирует только при приближении (см. hazard_filter).
 * Кнопка — SOS notify по BLE (GATT AA02), дублируется в UART-лог.
 */

#include "button_input.h"
#include "button_notify.h"
#include "button_registry.h"
#include "distance_feedback.h"
#include "distance_sensor.h"
#include "esp_err.h"
#include "feedback_output.h"
#include "feedback_registry.h"
#include "message_registry.h"
#include "sensor_registry.h"
#include "user_message.h"

void app_main(void)
{
    distance_sensor_t sensor;
    feedback_output_t feedback;
    button_input_t button;
    user_message_t message;

    ESP_ERROR_CHECK(distance_sensor_get_active(&sensor));
    ESP_ERROR_CHECK(feedback_output_get_active(&feedback));
    ESP_ERROR_CHECK(button_input_get_active(&button));
    ESP_ERROR_CHECK(user_message_get_active(&message));
    ESP_ERROR_CHECK(distance_sensor_init(&sensor));
    ESP_ERROR_CHECK(feedback_output_init(&feedback));
    ESP_ERROR_CHECK(button_input_init(&button));
    ESP_ERROR_CHECK(user_message_init(&message));
    ESP_ERROR_CHECK(button_notify_start(&button, &message));

    /* Блокирующий цикл: опрос → фильтр → мигание. */
    distance_feedback_run(&sensor, &feedback);
}
