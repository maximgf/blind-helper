#pragma once

#include "driver/gpio.h"
#include "driver/i2c_types.h"

/** Параметры модуля GY-530 (VL53L0X) — только этот драйвер их использует. */
#define VL53L0X_GY530_I2C_PORT              I2C_NUM_0
#define VL53L0X_GY530_I2C_SDA_GPIO          GPIO_NUM_4
#define VL53L0X_GY530_I2C_SCL_GPIO          GPIO_NUM_5
#define VL53L0X_GY530_XSHUT_GPIO            GPIO_NUM_7

#define VL53L0X_GY530_I2C_ADDR_7BIT         0x29
#define VL53L0X_GY530_I2C_FREQ_HZ           100000
#define VL53L0X_GY530_I2C_GLITCH_IGNORE_CNT 7
#define VL53L0X_GY530_I2C_INTERNAL_PULLUP   false
#define VL53L0X_GY530_I2C_PROBE_TIMEOUT_MS  1000

#define VL53L0X_GY530_XSHUT_LOW_US          100
#define VL53L0X_GY530_XSHUT_BOOT_US         2000

#define VL53L0X_GY530_IO_TIMEOUT_MS         500
#define VL53L0X_GY530_IO_2V8_MODE           true
