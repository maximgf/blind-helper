#pragma once

/**
 * @file distance_feedback.h
 * @brief Связывает дальномер, фильтр приближения и LED/вибромотор.
 *
 * Реальный смысл: «насколько срочно» (период мигания по дистанции),
 * но только пока hazard_filter видит сближение с препятствием.
 */

#include <stdint.h>

#include "distance_sensor.h"
#include "feedback_output.h"

/** Период вспышек (мс) по зоне дистанции; 0 — не в рабочем диапазоне. */
uint16_t distance_feedback_blink_period_ms(uint16_t mm);

/** Главный цикл приложения (не возвращается). */
void distance_feedback_run(distance_sensor_t *sensor, feedback_output_t *feedback);
