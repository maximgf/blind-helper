# button_registry

**Фабрика** активной кнопки.

## API

```c
esp_err_t button_input_get_active(button_input_t *out);
```

→ `gpio_button_get()` (GPIO 8, active-low).

## Назначение

Изоляция `button_notify` / `main.c` от конкретного пина и схемы (матрица, расширитель I/O и т.д.).

## Файлы

| Файл | Роль |
|------|------|
| `button_registry.h` | Объявление |
| `button_registry.c` | Делегирование в `gpio_button` |
