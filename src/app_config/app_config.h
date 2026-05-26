/**
 * @file app_config.h
 * @brief Пороги и тайминги приложения (подбор под ходьбу и шум датчика).
 */

#pragma once

#include <stdint.h>

/** Как часто печатать диагностику в UART (мс). */
#define APP_MEASURE_INTERVAL_MS 500

/** Как часто оценивать приближение/отдаление (мс); быстрее лога — точнее ловится движение. */
#define APP_HAZARD_SAMPLE_INTERVAL_MS 200

/** Период вызова tick мигания LED/вибромотора (мс). */
#define APP_FEEDBACK_TICK_MS 5

/** Как часто опрашивать кнопку (мс). */
#define APP_BUTTON_POLL_MS 10

/** Текст, уходящий в канал сообщений при нажатии кнопки. */
#define APP_BUTTON_MESSAGE "Нажата кнопка помощи"

/**
 * Скорость сближения (мм/с): v = Δдистанция/Δt.
 * Отрицательная v — объект или пользователь сближаются с препятствием в луче.
 */
#define HAZARD_V_APPROACH_MM_S 80

/** Сколько подряд замеров «приближается» нужно, чтобы включить индикацию (защита от шума ToF). */
#define HAZARD_M_APPROACH_SAMPLES 2

/** Дальше 1 м луч не считаем препятствием в зоне поражения. */
#define DISTANCE_ZONE_MAX_MM 1000

/** Зоны громкости сигнала: от «в упор» до «далеко, но ещё в метре». */
#define DISTANCE_ZONE_1_MAX_MM 250
#define DISTANCE_ZONE_2_MAX_MM 500
#define DISTANCE_ZONE_3_MAX_MM 750

/** Период мигания: ближе препятствие — чаще вспышки (срочнее). */
#define FEEDBACK_BLINK_PERIOD_ZONE_1_MS 150
#define FEEDBACK_BLINK_PERIOD_ZONE_2_MS 300
#define FEEDBACK_BLINK_PERIOD_ZONE_3_MS 600
#define FEEDBACK_BLINK_PERIOD_ZONE_4_MS 1200

/** Пороги зон (мм); по умолчанию — макросы выше, меняются через BLE CONFIG. */
uint16_t app_config_zone_max_mm(void);
uint16_t app_config_zone1_max_mm(void);
uint16_t app_config_zone2_max_mm(void);
uint16_t app_config_zone3_max_mm(void);

/** Пороги из приложения: safe, warn, near, critical (см), как в VibroGuideApp. */
void app_config_set_vibro_thresholds_cm(uint16_t safe_cm, uint16_t warn_cm, uint16_t near_cm,
                                        uint16_t critical_cm);
