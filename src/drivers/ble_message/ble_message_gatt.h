/**
 * @file ble_message_gatt.h
 * @brief GATT-слой VibroGuide: сервис/характеристики и отправка notify.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "hazard_filter/hazard_filter.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"

/** Лог регистрации сервиса/характеристик при старте NimBLE. */
void ble_message_gatt_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
/** Обработка подписки CCCD (вкл/выкл notify по характеристикам). */
void ble_message_gatt_subscribe_cb(struct ble_gap_event *event);
/** Фиксация активного conn_handle после успешного connect. */
void ble_message_gatt_on_connect(uint16_t conn_handle);
/** Сброс состояния подписок/соединения после disconnect. */
void ble_message_gatt_on_disconnect(void);

/** Регистрация GATT-сервиса VibroGuide (AA00..AA05). */
int ble_message_gatt_init(void);

/** Notify дистанции в сантиметрах (AA01, little-endian uint16). */
int ble_message_gatt_notify_distance(uint16_t distance_cm);
/** Notify SOS-события (AA02). */
int ble_message_gatt_notify_sos(void);
/** Notify байта статуса (AA03). */
int ble_message_gatt_notify_status(uint8_t status);
/** Notify уровня батареи в процентах (AA05). */
int ble_message_gatt_notify_battery(uint8_t percent);

/** Поставить SOS в очередь (вызывать из задачи кнопки). */
bool ble_message_gatt_request_sos(void);

/** Отправить отложенный SOS (вызывать из главного цикла приложения). */
void ble_message_gatt_flush_pending(void);

/** Преобразовать состояние hazard-фильтра в битовую маску статуса для AA03. */
uint8_t ble_message_gatt_status_from_hazard(hazard_state_t hazard, bool valid);
