# button_notify

Фоновая задача: опрос кнопки и отправка SOS через [user_message](../user_message/README.md).

## Принцип работы

1. `button_notify_start(button, message)` копирует контекст и создаёт задачу FreeRTOS `btn_notify` (стек 3072, приоритет 5).
2. Цикл с периодом `APP_BUTTON_POLL_MS` (10 ms):
   - `button_input_poll_pressed()` → при `true` вызывается `user_message_send(&message, APP_BUTTON_MESSAGE)`.

## Потоки и BLE

Кнопка работает **не** в контексте NimBLE host. При BLE:

- `ble_message_send()` только ставит флаг `sos_pending` (`ble_message_gatt_request_sos()`).
- Фактический GATT notify выполняется в `distance_feedback_run` → `ble_message_gatt_flush_pending()`.

Так избегаются вызовы стека BLE из чужой задачи.

## Текст сообщения

`APP_BUTTON_MESSAGE` (`"Нажата кнопка помощи"`) пишется в UART-лог. Для телефона значим **notify** характеристики AA02, а не строка.

## API

```c
esp_err_t button_notify_start(button_input_t *button, user_message_t *message);
```

Ошибки: `ESP_ERR_INVALID_ARG`, `ESP_ERR_NO_MEM` (не создалась задача).

## Зависимости

- `app_config` — интервал и текст.
- `button_input`, `user_message`.

## Файлы

| Файл | Роль |
|------|------|
| `button_notify.h` | API |
| `button_notify.c` | Задача `button_notify_task` |
