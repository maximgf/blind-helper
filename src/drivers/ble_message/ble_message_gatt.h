#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "hazard_filter.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"

void ble_message_gatt_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
void ble_message_gatt_subscribe_cb(struct ble_gap_event *event);
void ble_message_gatt_on_connect(uint16_t conn_handle);
void ble_message_gatt_on_disconnect(void);

int ble_message_gatt_init(void);

int ble_message_gatt_notify_distance(uint16_t distance_cm);
int ble_message_gatt_notify_sos(void);
int ble_message_gatt_notify_status(uint8_t status);
int ble_message_gatt_notify_battery(uint8_t percent);

/** Поставить SOS в очередь (вызывать из задачи кнопки). */
bool ble_message_gatt_request_sos(void);

/** Отправить отложенный SOS (вызывать из главного цикла приложения). */
void ble_message_gatt_flush_pending(void);

uint8_t ble_message_gatt_status_from_hazard(hazard_state_t hazard, bool valid);
