#pragma once

/**
 * @file ble_message_config.h
 * @brief GATT VibroGuide — совместим с src_android/Constants.kt.
 */

/** Имя в BLE-рекламе (видно при сканировании в приложении). */
#define BLE_MSG_DEVICE_NAME             "VibroGuide"

/** Минимальная длина записи CONFIG (4× uint16 LE, см). */
#define BLE_MSG_CONFIG_MIN_LEN          8

/** Значение notify SOS (любой байт — приложение реагирует на факт notify). */
#define BLE_MSG_SOS_NOTIFY_VALUE        0x01

/** Уровень батареи по умолчанию, если нет измерения АКБ. */
#define BLE_MSG_BATTERY_DEFAULT_PERCENT 100
