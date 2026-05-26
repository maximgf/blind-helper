# gpio_button

Драйвер кнопки на GPIO как реализация [button_input](../../button_input/README.md).

## Аппаратная конфигурация

| Параметр | Значение |
|----------|----------|
| GPIO | `GPIO_NUM_8` |
| Активный уровень | Active-low (нажатие → GND) |
| Pull | Internal pull-up |
| Debounce | 30 ms |

Файл: `gpio_button_config.h`.

## Конечный автомат

```text
IDLE ──(pressed)──► DEBOUNCE ──(≥30ms pressed)──► HELD ──(released)──► IDLE
         ▲              │ release early
         └──────────────┘
```

- `poll_pressed()` возвращает `true` **один раз** при переходе DEBOUNCE → HELD.
- Удержание без отпускания не генерирует повторных событий до возврата в IDLE.

## Протокол (физический)

| Слой | Описание |
|------|----------|
| Электрический | Цифровой вход, опрос `gpio_get_level` |
| Логический | `button_input_poll_pressed()` → bool |
| Прикладной | [button_notify](../../button_notify/README.md) → `user_message_send` |

Прерывания GPIO **не** используются — только периодический опрос.

## API

```c
esp_err_t gpio_button_get(button_input_t *out);
```

Возвращает статический `s_input` с именем `"GPIO button"`.

## Файлы

| Файл | Роль |
|------|------|
| `gpio_button.h` | Экспорт фабрики |
| `gpio_button.c` | FSM и GPIO config |
| `gpio_button_config.h` | Пин и тайминги |
