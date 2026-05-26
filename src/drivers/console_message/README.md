# console_message

Реализация [user_message](../../user_message/README.md) через **UART** (монитор PlatformIO, 115200 бод).

## Когда используется

Сборка **без** `CONFIG_BT_NIMBLE_ENABLED` — выбирается в [message_registry](../../message_registry/README.md). Удобно для отладки без телефона.

## Принцип работы

| Метод | Поведение |
|-------|-----------|
| `init` | No-op, `ESP_OK` |
| `send` | `ESP_LOGI("USER_MSG", "%s", text)` |

Тег лога: `USER_MSG`. Сообщение кнопки: `"Нажата кнопка помощи"` из `APP_BUTTON_MESSAGE`.

## Протокол

| Параметр | Значение |
|----------|----------|
| Транспорт | Serial UART |
| Кодировка текста | UTF-8 в строке C |
| Формат строки | Стандартный вывод ESP-IDF log (время, уровень, тег) |
| Бинарных пакетов | Нет |

Нет подтверждения доставки и нет обратного канала.

## API

```c
esp_err_t console_message_get(user_message_t *out);
```

Имя канала: `"Console"`.

## Файлы

| Файл | Роль |
|------|------|
| `console_message.h` | Фабрика |
| `console_message.c` | LOG-реализация |
