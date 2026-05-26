/**
 * @file app_config.c
 * @brief Пороги зон дистанции (по умолчанию из app_config.h, настраиваются по BLE).
 */

#include "app_config.h"

static uint16_t s_zone_max_mm = DISTANCE_ZONE_MAX_MM;
static uint16_t s_zone1_max_mm = DISTANCE_ZONE_1_MAX_MM;
static uint16_t s_zone2_max_mm = DISTANCE_ZONE_2_MAX_MM;
static uint16_t s_zone3_max_mm = DISTANCE_ZONE_3_MAX_MM;

uint16_t app_config_zone_max_mm(void)
{
    return s_zone_max_mm;
}

uint16_t app_config_zone1_max_mm(void)
{
    return s_zone1_max_mm;
}

uint16_t app_config_zone2_max_mm(void)
{
    return s_zone2_max_mm;
}

uint16_t app_config_zone3_max_mm(void)
{
    return s_zone3_max_mm;
}

void app_config_set_vibro_thresholds_cm(uint16_t safe_cm, uint16_t warn_cm, uint16_t near_cm,
                                      uint16_t critical_cm)
{
    if (safe_cm < warn_cm) {
        safe_cm = warn_cm;
    }
    if (warn_cm < near_cm) {
        warn_cm = near_cm;
    }
    if (near_cm < critical_cm) {
        near_cm = critical_cm;
    }
    if (critical_cm < 1) {
        critical_cm = 1;
    }

    s_zone_max_mm = (uint16_t)(safe_cm * 10U);
    s_zone3_max_mm = (uint16_t)(warn_cm * 10U);
    s_zone2_max_mm = (uint16_t)(near_cm * 10U);
    s_zone1_max_mm = (uint16_t)(critical_cm * 10U);
}
