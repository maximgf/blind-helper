#pragma once

/**
 * @file feedback_registry.h
 * @brief Какой физический индикатор сейчас в устройстве (LED или вибромотор).
 *
 * Единственное место замены «лампочка → вибро» для всего приложения.
 */

#include "esp_err.h"
#include "feedback_output/feedback_output.h"

esp_err_t feedback_output_get_active(feedback_output_t *out);
