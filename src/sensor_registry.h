#pragma once

/**
 * @file sensor_registry.h
 * @brief Выбор активного драйвера датчика дистанции.
 *
 * ## Как подключить свой датчик
 *
 * 1. Создайте каталог `src/drivers/<ваш_датчик>/`.
 * 2. Реализуйте @ref distance_sensor_t (поля @c init , @c read_mm , при необходимости @c deinit ).
 * 3. Добавьте `*.c` в сборку (каталог `src/` подхватывается CMake автоматически).
 * 4. В `sensor_registry.c` замените вызов фабрики на свою, например:
 *    @code
 *    return my_sensor_get(&out);
 *    @endcode
 * 5. Параметры пинов/шины держите в `drivers/<ваш_датчик>/<имя>_config.h`,
 *    не в прикладном коде.
 *
 * Файлы `distance_app.c` и `main.c` менять не нужно.
 */

#include "distance_sensor.h"
#include "esp_err.h"

/** Возвращает дескриптор единственного активного датчика (без инициализации железа). */
esp_err_t distance_sensor_get_active(distance_sensor_t *out);
