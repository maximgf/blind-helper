# distance_sensor

**Абстракция** лазерного (или иного) дальномера: прикладной код работает только с миллиметрами вдоль луча, без привязки к чипу.

## Принцип работы

Паттерн «виртуальная таблица» (vtable): структура `distance_sensor_t` содержит указатели `init`, `deinit`, `read_mm` и opaque `impl` для состояния драйвера.

```c
typedef struct distance_sensor {
    const char *name;
    distance_sensor_init_fn init;
    distance_sensor_deinit_fn deinit;
    distance_sensor_read_mm_fn read_mm;
    void *impl;
} distance_sensor_t;
```

Inline-обёртки проверяют `NULL` и делегируют в драйвер.

## Протокол данных (логический)

| Поле / константа | Тип | Описание |
|------------------|-----|----------|
| Выход `read_mm` | `uint16_t` | Дистанция в **мм** вдоль оси модуля |
| `DISTANCE_MM_INVALID` | `65535` | Нет достоверного замера (вне диапазона, блик, сбой шины) |
| Код возврата | `esp_err_t` | `ESP_OK` — значение записано; иначе ошибка транспорта/датчика |

Физический протокол (I2C, регистры VL53L0X) инкапсулирован в `drivers/vl53l0x_gy530` — см. [его README](../drivers/vl53l0x_gy530/README.md).

## Жизненный цикл

1. `distance_sensor_get_active()` — из [sensor_registry](../sensor_registry/README.md).
2. `distance_sensor_init()` — шина, калибровка чипа.
3. Цикл: `distance_sensor_read_mm(&mm)`.
4. `distance_sensor_deinit()` — опционально (текущий драйвер не реализует).

## Потребители

- `hazard_filter` — динамика дистанции.
- `distance_feedback` — опрос и телеметрия.

## Расширение

Новый датчик: реализовать три callback'а, экспортировать `xxx_get(distance_sensor_t *out)`, подключить в `sensor_registry.c`.

## Файлы

| Файл | Роль |
|------|------|
| `distance_sensor.h` | API и inline-helpers |
