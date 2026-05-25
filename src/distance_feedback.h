#pragma once

/**
 * @file distance_feedback.h
 * @brief Связка измерений дистанции и обратной связи (частота мигания по зонам).
 */

#include <stdint.h>

#include "distance_sensor.h"
#include "feedback_output.h"

/** Период мигания (мс) для расстояния @p mm; 0 — вне диапазона или невалидно. */
uint16_t distance_feedback_blink_period_ms(uint16_t mm);

/** Запускает цикл: опрос датчика → обновление периода мигания → tick исполнителя. */
void distance_feedback_run(distance_sensor_t *sensor, feedback_output_t *feedback);
