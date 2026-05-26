# user_message

**Абстракция канала сообщений** к сопровождающему / телефону (SOS и сервисные тексты).

## API

```c
typedef struct user_message {
    const char *name;
    user_message_init_fn init;
    user_message_deinit_fn deinit;
    user_message_send_fn send;
    void *impl;
} user_message_t;
```

| Метод | Назначение |
|-------|------------|
| `init` | Поднять транспорт (NimBLE + NVS или no-op для UART) |
| `send(self, text)` | Доставить событие (лог + BLE notify для SOS) |
| `deinit` | Опционально |

## Реализации

Выбор в [message_registry](../message_registry/README.md):

| Условие | Драйвер | Транспорт |
|---------|---------|-----------|
| `CONFIG_BT_NIMBLE_ENABLED` | `ble_message` | BLE GATT |
| иначе | `console_message` | `ESP_LOG` → UART 115200 |

## Протокол (логический)

- Вход: UTF-8 C-string (для кнопки — константа из `app_config`).
- Выход зависит от драйвера; для продакшена критичен **BLE notify SOS** (см. [ble_message](../drivers/ble_message/README.md)).

## Файлы

| Файл | Роль |
|------|------|
| `user_message.h` | Vtable |
