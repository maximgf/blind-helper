#pragma once

#include "distance_sensor.h"

/** Блокирующий цикл опроса датчика и вывода в UART. */
void distance_app_run(distance_sensor_t *sensor);
