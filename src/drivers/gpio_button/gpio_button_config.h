#pragma once

/**
 * @file gpio_button_config.h
 * @brief Пин кнопки на плате (не пересекается с I2C, XSHUT, LED).
 */

#include "driver/gpio.h"

#define GPIO_BUTTON_GPIO            GPIO_NUM_8
/** true — нажатие тянет линию к GND (типичная схема с pull-up). */
#define GPIO_BUTTON_ACTIVE_LOW      true
#define GPIO_BUTTON_DEBOUNCE_MS     30
