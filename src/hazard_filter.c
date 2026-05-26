/**
 * @file hazard_filter.c
 * @brief Оценка скорости изменения дистанции между замерами ToF.
 */

#include "hazard_filter.h"

#include "app_config.h"
#include "distance_sensor.h"

#include "esp_timer.h"

static struct {
    uint16_t prev_mm;
    int64_t prev_time_us;
    bool prev_valid;
    uint8_t approach_count; /**< Подряд замеров с v «в минус». */
    hazard_state_t state;
    int32_t velocity_mm_s;
    uint16_t last_mm;
} s;

void hazard_filter_reset(void)
{
    s.prev_valid = false;
    s.approach_count = 0;
    s.state = HAZARD_NONE;
    s.velocity_mm_s = 0;
    s.last_mm = DISTANCE_MM_INVALID;
}

hazard_state_t hazard_filter_get_state(void)
{
    return s.state;
}

uint16_t hazard_filter_get_last_mm(void)
{
    return s.last_mm;
}

int32_t hazard_filter_get_velocity_mm_s(void)
{
    return s.velocity_mm_s;
}

bool hazard_filter_should_alert(hazard_state_t state)
{
    return state == HAZARD_APPROACHING;
}

const char *hazard_filter_state_name(hazard_state_t state)
{
    switch (state) {
    case HAZARD_APPROACHING:
        return "APPROACHING";
    case HAZARD_STABLE:
        return "STABLE";
    case HAZARD_RECEDING:
        return "RECEDING";
    default:
        return "NONE";
    }
}

static void update_velocity(uint16_t mm, int64_t now_us)
{
    s.velocity_mm_s = 0;
    if (!s.prev_valid) {
        return;
    }

    int64_t dt_us = now_us - s.prev_time_us;
    if (dt_us <= 0) {
        return;
    }

    int32_t delta = (int32_t)mm - (int32_t)s.prev_mm;
    s.velocity_mm_s = (int32_t)((delta * 1000000LL) / dt_us);
}

hazard_state_t hazard_filter_update(uint16_t mm, bool valid)
{
    int64_t now_us = esp_timer_get_time();

    if (!valid || mm == DISTANCE_MM_INVALID || mm > app_config_zone_max_mm()) {
        s.state = HAZARD_NONE;
        s.approach_count = 0;
        s.prev_valid = false;
        s.velocity_mm_s = 0;
        s.last_mm = DISTANCE_MM_INVALID;
        return s.state;
    }

    s.last_mm = mm;
    update_velocity(mm, now_us);

    if (s.velocity_mm_s < -HAZARD_V_APPROACH_MM_S) {
        if (s.approach_count < UINT8_MAX) {
            s.approach_count++;
        }
        if (s.approach_count >= HAZARD_M_APPROACH_SAMPLES) {
            s.state = HAZARD_APPROACHING;
        } else {
            /* Ещё не подтвердили — не пугаем ложным срабатыванием. */
            s.state = HAZARD_STABLE;
        }
    } else {
        s.approach_count = 0;
        if (s.velocity_mm_s > HAZARD_V_APPROACH_MM_S) {
            s.state = HAZARD_RECEDING;
        } else if (s.prev_valid) {
            s.state = HAZARD_STABLE;
        } else {
            s.state = HAZARD_NONE;
        }
    }

    s.prev_mm = mm;
    s.prev_time_us = now_us;
    s.prev_valid = true;
    return s.state;
}
