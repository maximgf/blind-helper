/**
 * @file message_registry.c
 * @brief Фабрика активного канала сообщений.
 */

#include "message_registry.h"

#include "drivers/console_message/console_message.h"

esp_err_t user_message_get_active(user_message_t *out)
{
    return console_message_get(out);
}
