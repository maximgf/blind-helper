# feedback_led

Драйвер светодиода на GPIO как реализация [feedback_output](../../feedback_output/README.md).

## Аппаратная конфигурация

| Параметр | Значение |
|----------|----------|
| GPIO | `GPIO_NUM_6` |
| Режим | Output push-pull |

## Принцип мигания

- `set_blink_period_ms(P)`: период полного цикла ON+OFF; `P=0` гасит LED.
- `tick()` каждые ~5 ms (из `distance_feedback`):
  - При `period_ms > 0`: переключение уровня каждые `period_ms / 2` (duty **50%**).
  - Время — `esp_timer_get_time()`, микросекунды.

Для вибромотора тот же API: половина периода — вибрация, половина — пауза.

## Протокол

| Уровень | Формат |
|---------|--------|
| Прикладной | `uint16_t period_ms` (0 = off) |
| Физический | GPIO 0/1 |

## API

```c
esp_err_t feedback_led_get(feedback_output_t *out);
```

Имя экземпляра: `"LED"`.

## Файлы

| Файл | Роль |
|------|------|
| `feedback_led.h` | Фабрика |
| `feedback_led.c` | GPIO + генератор |
| `feedback_led_config.h` | Номер пина |
