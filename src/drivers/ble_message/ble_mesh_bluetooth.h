#pragma once

/**
 * @file ble_mesh_bluetooth.h
 * @brief Инициализация BLE-контроллера (адаптация esp-idf ble_mesh_example_init).
 */

#include <stdint.h>

#include "esp_err.h"

void ble_mesh_get_dev_uuid(uint8_t *dev_uuid);

esp_err_t bluetooth_init(void);
