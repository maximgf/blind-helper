#pragma once

/**
 * @file feedback_led_config.h
 * @brief Пин индикатора на плате (не пересекается с I2C/XSHUT дальномера).
 */

#include "driver/gpio.h"

#define FEEDBACK_LED_GPIO GPIO_NUM_6
