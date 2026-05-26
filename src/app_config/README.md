# app_config

Централизованные **пороги**, **тайминги** и **runtime-настройка зон** дистанции (из прошивки и по BLE).

## Назначение

- Константы времени опроса, hazard-фильтра, мигания и кнопки — в `app_config.h`.
- Границы зон дистанции (мм) — по умолчанию из макросов, с возможностью перезаписи через `app_config_set_vibro_thresholds_cm()` (вызов из GATT CONFIG).

## Параметры (compile-time)

| Макрос | Значение | Смысл |
|--------|----------|--------|
| `APP_MEASURE_INTERVAL_MS` | 500 | Период UART-лога и BLE notify дистанции/статуса |
| `APP_HAZARD_SAMPLE_INTERVAL_MS` | 200 | Опрос ToF и обновление hazard |
| `APP_FEEDBACK_TICK_MS` | 5 | Шаг `feedback_output_tick` |
| `APP_BUTTON_POLL_MS` | 10 | Опрос кнопки в `button_notify` |
| `APP_BUTTON_MESSAGE` | `"Нажата кнопка помощи"` | Текст при SOS (лог; BLE — отдельный notify) |
| `HAZARD_V_APPROACH_MM_S` | 80 | Порог скорости сближения (мм/с), отрицательная v |
| `HAZARD_M_APPROACH_SAMPLES` | 2 | Подряд «быстрых» замеров для APPROACHING |
| `DISTANCE_ZONE_MAX_MM` | 1000 | Вне зоны — нет hazard и мигания |
| `DISTANCE_ZONE_1..3_MAX_MM` | 250 / 500 / 750 | Границы зон 1–3 |
| `FEEDBACK_BLINK_PERIOD_ZONE_1..4_MS` | 150 / 300 / 600 / 1200 | Период мигания по зоне |

## Runtime API

```c
uint16_t app_config_zone_max_mm(void);
uint16_t app_config_zone1_max_mm(void);
uint16_t app_config_zone2_max_mm(void);
uint16_t app_config_zone3_max_mm(void);

void app_config_set_vibro_thresholds_cm(uint16_t safe_cm, uint16_t warn_cm,
                                        uint16_t near_cm, uint16_t critical_cm);
```

### Маппинг порогов (см → мм)

Запись из BLE CONFIG (см., `drivers/ble_message`):

| Параметр приложения | Поле в прошивке | Роль |
|---------------------|-----------------|------|
| `safe_cm` | `s_zone_max_mm` | Максимальная рабочая дистанция |
| `warn_cm` | `s_zone3_max_mm` | Верх зоны 3 |
| `near_cm` | `s_zone2_max_mm` | Верх зоны 2 |
| `critical_cm` | `s_zone1_max_mm` | Верх зоны 1 («в упор») |

Перед применением пороги **нормализуются**: `safe ≥ warn ≥ near ≥ critical`, `critical ≥ 1`.

## Зависимости

- **Читает**: `hazard_filter`, `distance_feedback`, `ble_message_gatt` (CONFIG write).
- **Не зависит** от драйверов GPIO/I2C.

## Файлы

| Файл | Содержание |
|------|------------|
| `app_config.h` | Макросы и объявления API |
| `app_config.c` | Статические `s_zone*_mm` и реализация setter |
