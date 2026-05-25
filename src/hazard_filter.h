#pragma once

/**
 * @file hazard_filter.h
 * @brief Классификация по направлению изменения дистанции.
 *
 * Оповещение только при @ref HAZARD_APPROACHING (объект приближается).
 * Стабильная дистанция и отдаление — без индикации.
 */

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    HAZARD_NONE,
    HAZARD_APPROACHING,
    HAZARD_STABLE,
    HAZARD_RECEDING,
} hazard_state_t;

void hazard_filter_reset(void);

/**
 * @param mm дистанция (мм) или @ref DISTANCE_MM_INVALID при ошибке.
 * @param valid true, если чтение датчика успешно.
 */
hazard_state_t hazard_filter_update(uint16_t mm, bool valid);

hazard_state_t hazard_filter_get_state(void);
uint16_t hazard_filter_get_last_mm(void);
/** Скорость изменения дистанции (мм/с): отрицательная — приближение. */
int32_t hazard_filter_get_velocity_mm_s(void);

const char *hazard_filter_state_name(hazard_state_t state);

bool hazard_filter_should_alert(hazard_state_t state);
