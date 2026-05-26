# ble_message

Стек **Apache NimBLE** на ESP32: GAP (реклама, соединение) + GATT-сервис **VibroGuide**, совместимый с Android (`src_android/Constants.kt`).

Реализует [user_message](../../user_message/README.md) и телеметрию из [distance_feedback](../../distance_feedback/README.md).

## Структура модуля

| Файл | Слой |
|------|------|
| `ble_message.c` | Init NVS, запуск host, `user_message` (SOS queue) |
| `ble_message_gap.c` | Реклама, connect/disconnect |
| `ble_message_gatt.c` | Сервис AA00–AA05, notify/read/write |
| `ble_message_config.h` | Имя устройства, константы |

## Стек и задачи

```text
app_main
  ├─ ble_message_init() → nimble_port_freertos_init(nimble_host_task)
  ├─ distance_feedback_run() → gatt flush + notify
  └─ button_notify_task → ble_message_gatt_request_sos()

nimble_host_task → nimble_port_run()
on_stack_sync → ble_message_gap_adv_start()
```

SOS **не** вызывает `ble_gatts_notify` из задачи кнопки — только `s_sos_pending`, flush в главном цикле.

---

## GAP (реклама и соединение)

### Имя и UUID в эфире

| Поле ADV | Значение |
|----------|----------|
| Local name | `VibroGuide` (`BLE_MSG_DEVICE_NAME`) |
| Flags | General discoverable, BR/EDR not supported |
| Service UUID 16-bit | `0xAA00` (полный 128-bit в GATT) |
| Интервал рекламы | 200–220 ms |

### События

| Событие | Действие |
|---------|----------|
| CONNECT OK | `ble_message_gatt_on_connect`, реклама останавливается |
| CONNECT fail / DISCONNECT | Сброс GATT, снова `start_advertising()` |
| SUBSCRIBE | Обновление флагов CCCD для notify |

Адрес: auto (public/random), логируется при старте рекламы.

---

## GATT: сервис VibroGuide

Базовый UUID (Bluetooth base):  
`0000XXXX-0000-1000-8000-00805f9b34fb` — подставляется 16-bit `XXXX`.

### Сводная таблица характеристик

| UUID16 | Имя | Свойства | Размер payload | Направление | Периодичность |
|--------|-----|----------|----------------|-------------|---------------|
| `AA00` | Service | Primary | — | — | — |
| `AA01` | Distance | Read, Notify | 2 байта | Peripheral → Central | ~500 ms + по read |
| `AA02` | SOS | Read, Notify | 1 байт | Peripheral → Central | По кнопке |
| `AA03` | Status | Read, Notify | 1 байт | Peripheral → Central | ~500 ms |
| `AA04` | Config | Write | ≥ 8 байт | Central → Peripheral | По запросу app |
| `AA05` | Battery | Read, Notify | 1 байт | Peripheral → Central | При connect + по read |

### AA01 — Distance

| Поле | Формат |
|------|--------|
| Единицы | Сантиметры (целое) |
| Порядок байт | **Little-endian** `uint16` |
| Источник | `(mm + 5) / 10` из ToF |
| Notify | Только при валидном замере и включённом CCCD |

Пример: 123 cm → `0x7B 0x00`.

### AA02 — SOS

| Поле | Формат |
|------|--------|
| Значение | Счётчик `s_last_sos++` (1…255, не 0) |
| Смысл для Android | Любое изменение + notify → `handleSosEvent()` → MQTT |

Запрос: `ble_message_gatt_request_sos()` (нужно активное соединение).  
Отправка: `ble_message_gatt_flush_pending()` в цикле `distance_feedback`.

При сбое notify с отключённым флагом CCCD делается повтор с `notify_enabled=true` (обход гонки подписки).

### AA03 — Status (битовая маска)

Формируется в `ble_message_gatt_status_from_hazard()`:

| Бит | Маска | Условие |
|-----|-------|---------|
| 0 | `0x01` | Валидный замер дистанции |
| 1 | `0x02` | `hazard == HAZARD_APPROACHING` |
| 2 | `0x04` | `hazard_filter_should_alert(hazard)` |

### AA04 — Config (write)

| Смещение | Тип | Поле (Android / app) |
|----------|-----|----------------------|
| 0–1 | uint16 LE | `safe_cm` → `zone_max` |
| 2–3 | uint16 LE | `warn_cm` → zone3 |
| 4–5 | uint16 LE | `near_cm` → zone2 |
| 6–7 | uint16 LE | `critical_cm` → zone1 |

Минимальная длина: `BLE_MSG_CONFIG_MIN_LEN` (8).  
Вызов: `app_config_set_vibro_thresholds_cm()`.

### AA05 — Battery

| Поле | Формат |
|------|--------|
| Значение | `uint8` 0–100 % |
| По умолчанию | `100` (`BLE_MSG_BATTERY_DEFAULT_PERCENT`) |
| Измерение АКБ | Пока заглушка; notify при connect |

---

## ATT / NimBLE (поведение)

| Механизм | Использование |
|----------|----------------|
| Notify | `ble_gatts_notify_custom` + mbuf |
| CCCD | `BLE_GAP_EVENT_SUBSCRIBE` → флаги `s_*_notify` |
| Read | Последнее кэшированное значение (`s_last_*`) |
| Write Config | `BLE_GATT_ACCESS_OP_WRITE_CHR` |

Без соединения или без CCCD notify возвращает предупреждение в лог, не падает.

---

## user_message API

```c
esp_err_t ble_message_get(user_message_t *out);
```

- `init`: NVS + `nimble_port_init`, GAP/GATT, host task.
- `send`: лог текста + `ble_message_gatt_request_sos()`; `ESP_ERR_INVALID_STATE` если нет BLE link.

---

## Сборка

При `CONFIG_BT_NIMBLE_ENABLED=n` каталог **не компилируется**; см. `src/CMakeLists.txt`, зависимость `bt` в component.

---

## Совместимость с Android

| ESP32 | Android (`VibroGuideService.kt`) |
|-------|----------------------------------|
| Имя `VibroGuide` | Сканирование / подключение GATT |
| UUID AA01–AA05 | `Constants.kt` |
| LE distance | `onCharacteristicChanged` → UI + notification |
| SOS notify | MQTT `publishSos` |
| CONFIG write | `writeVibroConfig(ByteArray)` |

Подписка на notify: CCCD по одной характеристике за раз (ограничение Android).

## Файлы

| Файл | Роль |
|------|------|
| `ble_message.h` / `.c` | Фасад user_message + NimBLE host |
| `ble_message_gap.h` / `.c` | Advertising |
| `ble_message_gatt.h` / `.c` | GATT сервис |
| `ble_message_config.h` | Имя и константы |
