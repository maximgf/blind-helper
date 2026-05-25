#pragma once

/**
 * @file feedback_led.h
 * @brief Драйвер светодиода как исполнителя feedback_output.
 */

#include "esp_err.h"
#include "feedback_output.h"

esp_err_t feedback_led_get(feedback_output_t *out);
