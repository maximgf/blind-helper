#pragma once

/**
 * @file hazard_filter.h
 * @brief Решает, есть ли «угроза столкновения» по динамике дистанции.
 *
 * Статичная стена в метре — без сигнала (дистанция не падает).
 * Идёте к ней или к вам приближается человек — v < 0, включается индикация.
 * Объект стоит на месте или отходит — сигнала нет.
 */

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    HAZARD_NONE,       /**< Нет данных или вне зоны 1 м. */
    HAZARD_APPROACHING,/**< Сближение — включать LED/вибро. */
    HAZARD_STABLE,     /**< Дистанция почти не меняется — не мешать. */
    HAZARD_RECEDING,   /**< Уходит / вы прошли — не мешать. */
} hazard_state_t;

void hazard_filter_reset(void);

hazard_state_t hazard_filter_update(uint16_t mm, bool valid);

hazard_state_t hazard_filter_get_state(void);
uint16_t hazard_filter_get_last_mm(void);
/** мм/с: минус — приближение, плюс — отдаление. */
int32_t hazard_filter_get_velocity_mm_s(void);

const char *hazard_filter_state_name(hazard_state_t state);

bool hazard_filter_should_alert(hazard_state_t state);
