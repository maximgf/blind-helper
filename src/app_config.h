#pragma once

/** Период опроса датчика дистанции (мс). */
#define APP_MEASURE_INTERVAL_MS 500

/** Шаг главного цикла для tick мигания (мс). */
#define APP_FEEDBACK_TICK_MS 5

/** Верхняя граница рабочего диапазона (мм); дальше — обратная связь выключена. */
#define DISTANCE_ZONE_MAX_MM 1000

/** Границы зон (мм): 0–250, 251–500, 501–750, 751–1000. */
#define DISTANCE_ZONE_1_MAX_MM 250
#define DISTANCE_ZONE_2_MAX_MM 500
#define DISTANCE_ZONE_3_MAX_MM 750

/**
 * Полный период мигания (мс) по зонам: чем ближе объект, тем меньше значение.
 * Зона 1 (0–250 мм) — самая частая; зона 4 (751–1000 мм) — самая редкая.
 */
#define FEEDBACK_BLINK_PERIOD_ZONE_1_MS 150
#define FEEDBACK_BLINK_PERIOD_ZONE_2_MS 300
#define FEEDBACK_BLINK_PERIOD_ZONE_3_MS 600
#define FEEDBACK_BLINK_PERIOD_ZONE_4_MS 1200
