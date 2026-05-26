# hazard_filter

Решает, есть ли **угроза столкновения** по **динамике** дистанции, а не по статическому расстоянию.

## Идея

| Ситуация | Состояние | Индикация |
|----------|-----------|-----------|
| Стена в 80 см, вы стоите | `HAZARD_STABLE` | Нет |
| Идёте к препятствию, v &lt; −80 мм/с | `HAZARD_APPROACHING` | Да |
| Отходите или прошли | `HAZARD_RECEDING` | Нет |
| Нет замера / дальше `zone_max` | `HAZARD_NONE` | Нет |

Статичное препятствие в зоне 1 м **не** включает LED — только **сближение** (отрицательная скорость изменения дистанции).

## Алгоритм

На каждый вызов `hazard_filter_update(mm, valid)`:

1. При невалидном `mm` или `mm > app_config_zone_max_mm()` → сброс, `HAZARD_NONE`.
2. Скорость: \( v = \Delta mm / \Delta t \) (мм/с), `esp_timer_get_time()`.
3. Если `v < -HAZARD_V_APPROACH_MM_S` (80): счётчик `approach_count++`.
   - При `approach_count >= HAZARD_M_APPROACH_SAMPLES` (2) → `HAZARD_APPROACHING`.
   - Иначе → `HAZARD_STABLE` (антишум ToF).
4. Иначе сброс счётчика; при `v > +80` → `HAZARD_RECEDING`; иначе `HAZARD_STABLE`.

`hazard_filter_should_alert()` истинно только для `HAZARD_APPROACHING`.

## API

| Функция | Назначение |
|---------|------------|
| `hazard_filter_reset()` | Сброс при старте цикла |
| `hazard_filter_update(mm, valid)` | Новый замер → состояние |
| `hazard_filter_get_state()` | Текущее enum |
| `hazard_filter_get_last_mm()` | Последний валидный мм |
| `hazard_filter_get_velocity_mm_s()` | Текущая v (диагностика) |
| `hazard_filter_state_name()` | Строка для лога |

## Состояния (`hazard_state_t`)

```c
HAZARD_NONE        // нет данных / вне зоны
HAZARD_APPROACHING // сближение — alert
HAZARD_STABLE      // дистанция почти не меняется
HAZARD_RECEDING    // отдаление
```

## Связь с BLE

Байт статуса AA03 формируется в `ble_message_gatt_status_from_hazard()` — биты 0x02 (approaching) и 0x04 (should_alert) завязаны на этот модуль.

## Зависимости

- `app_config` — `zone_max_mm`, порог скорости из `app_config.h`.
- `DISTANCE_MM_INVALID` из `distance_sensor.h`.

## Файлы

| Файл | Роль |
|------|------|
| `hazard_filter.h` | Типы и API |
| `hazard_filter.c` | Статическое состояние `s` |
