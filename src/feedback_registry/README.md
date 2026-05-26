# feedback_registry

**Фабрика** активного канала обратной связи (индикатор).

## API

```c
esp_err_t feedback_output_get_active(feedback_output_t *out);
```

Реализация: `feedback_led_get()` → GPIO LED на пине 6.

## Назначение

Единственная точка замены «лампочка ↔ вибромотор» без правок `distance_feedback` и `main.c`.

## Зависимости

```
feedback_registry.c → drivers/feedback_led/feedback_led.h
                    → feedback_output/feedback_output.h
```

## Файлы

| Файл | Роль |
|------|------|
| `feedback_registry.h` | Объявление |
| `feedback_registry.c` | Привязка к LED |
