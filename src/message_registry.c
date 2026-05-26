/**
 * @file message_registry.c
 * @brief Фабрика активного канала сообщений.
 */

#include "message_registry.h"

#include "sdkconfig.h"

#if CONFIG_BT_NIMBLE_ENABLED
#include "drivers/ble_message/ble_message.h"
#else
#include "drivers/console_message/console_message.h"
#endif

esp_err_t user_message_get_active(user_message_t *out)
{
#if CONFIG_BT_NIMBLE_ENABLED
    return ble_message_get(out);
#else
    return console_message_get(out);
#endif
}
