# distance_feedback

**Оркестратор** основного сценария: опрос дальномера → hazard → ритм индикации → (опционально) BLE-телеметрия.

## Принцип работы

`distance_feedback_run()` — бесконечный цикл на текущей задаче `app_main` (не возвращается).

Три независимых таймера (FreeRTOS ticks):

```text
┌─────────────────────────────────────────────────────────┐
│  каждые 200 ms: read_mm → hazard_filter_update        │
│                 → set_blink_period_ms (0 = выкл)       │
├─────────────────────────────────────────────────────────┤
│  каждые 500 ms: UART-лог; BLE notify distance/status   │
│                 ble_message_gatt_flush_pending (SOS)   │
├─────────────────────────────────────────────────────────┤
│  каждые 5 ms:   feedback_output_tick (мигание)         │
└─────────────────────────────────────────────────────────┘
```

### Период мигания

`distance_feedback_blink_period_ms(mm)` по зонам из [app_config](../app_config/README.md):

| Условие (мм) | Период (мс) |
|--------------|-------------|
| invalid или &gt; zone_max | 0 |
| ≤ zone1 | 150 |
| ≤ zone2 | 300 |
| ≤ zone3 | 600 |
| иначе в зоне | 1200 |

`period_for_hazard()`: период применяется **только** если `hazard_filter_should_alert(hazard)` — иначе 0 (тишина).

### BLE (если `CONFIG_BT_NIMBLE_ENABLED`)

На ритме лога (500 ms):

- `ble_message_gatt_notify_status(status)` — байт из hazard.
- `ble_message_gatt_notify_distance(cm)` — округление `(mm + 5) / 10`.

Каждую итерацию цикла: `ble_message_gatt_flush_pending()` — отправка отложенного SOS из задачи кнопки.

## API

```c
uint16_t distance_feedback_blink_period_ms(uint16_t mm);
void distance_feedback_run(distance_sensor_t *sensor, feedback_output_t *feedback);
```

## Зависимости

| Модуль | Роль |
|--------|------|
| `distance_sensor` | Замеры |
| `feedback_output` | LED tick |
| `hazard_filter` | Логика alert |
| `app_config` | Зоны |
| `ble_message_gatt` | Телеметрия |

## Файлы

| Файл | Роль |
|------|------|
| `distance_feedback.h` | API |
| `distance_feedback.c` | Главный цикл |
