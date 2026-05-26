# sensor_registry

**Фабрика** активного дальномера: единственное место выбора конкретного железа для всего приложения.

## Принцип работы

```c
esp_err_t distance_sensor_get_active(distance_sensor_t *out);
```

Текущая реализация делегирует в `vl53l0x_gy530_get()` — модуль GY-530 на VL53L0X.

## Зачем отдельный модуль

- `main.c` и `distance_feedback` зависят только от `distance_sensor_t`.
- Смена ToF → ультразвук или другой I2C-чип: правка **только** `sensor_registry.c` (+ новый драйвер в `drivers/`).

## Зависимости

```
sensor_registry.c → drivers/vl53l0x_gy530/vl53l0x_gy530.h
                  → distance_sensor/distance_sensor.h
```

## Файлы

| Файл | Роль |
|------|------|
| `sensor_registry.h` | Объявление API |
| `sensor_registry.c` | Привязка к `vl53l0x_gy530` |
