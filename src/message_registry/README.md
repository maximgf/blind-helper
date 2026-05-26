# message_registry

**Фабрика** активного канала `user_message` в зависимости от конфигурации сборки.

## API

```c
esp_err_t user_message_get_active(user_message_t *out);
```

## Логика выбора

```c
#if CONFIG_BT_NIMBLE_ENABLED
    return ble_message_get(out);
#else
    return console_message_get(out);
#endif
```

Исходники `drivers/ble_message/*.c` **исключаются** из сборки, если NimBLE выключен (`src/CMakeLists.txt`).

## Связь с sdkconfig

`sdkconfig.defaults`:

- `CONFIG_BT_NIMBLE_ENABLED=y`
- `CONFIG_BT_BLUEDROID_ENABLED=n`

## Файлы

| Файл | Роль |
|------|------|
| `message_registry.h` | Объявление |
| `message_registry.c` | Условная компиляция BLE/UART |
