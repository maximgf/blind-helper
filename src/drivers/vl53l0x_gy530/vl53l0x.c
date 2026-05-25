/**
 * Минимальный драйвер VL53L0X для ESP-IDF (i2c_master).
 * Алгоритм инициализации и измерения основан на ST VL53L0X API / UM2039.
 */

#include "vl53l0x.h"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "VL53L0X"

#define I2C_TIMEOUT_MS 1000

#define decodeVcselPeriod(reg_val) (((reg_val) + 1) << 1)
#define encodeVcselPeriod(period_pclks) (((period_pclks) >> 1) - 1)
#define calcMacroPeriod(vcsel_period_pclks) \
    ((((uint32_t)2304 * (vcsel_period_pclks) * 1655) + 500) / 1000)

enum vl53l0x_reg {
    SYSRANGE_START = 0x00,
    SYSTEM_SEQUENCE_CONFIG = 0x01,
    SYSTEM_RANGE_CONFIG = 0x09,
    SYSTEM_INTERMEASUREMENT_PERIOD = 0x04,
    SYSTEM_INTERRUPT_CONFIG_GPIO = 0x0A,
    GPIO_HV_MUX_ACTIVE_HIGH = 0x84,
    SYSTEM_INTERRUPT_CLEAR = 0x0B,
    RESULT_INTERRUPT_STATUS = 0x13,
    RESULT_RANGE_STATUS = 0x14,
    MSRC_CONFIG_CONTROL = 0x60,
    FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT = 0x44,
    PRE_RANGE_CONFIG_MIN_SNR = 0x27,
    PRE_RANGE_CONFIG_VALID_PHASE_LOW = 0x56,
    PRE_RANGE_CONFIG_VALID_PHASE_HIGH = 0x57,
    PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT = 0x64,
    FINAL_RANGE_CONFIG_MIN_SNR = 0x67,
    FINAL_RANGE_CONFIG_VALID_PHASE_LOW = 0x47,
    FINAL_RANGE_CONFIG_VALID_PHASE_HIGH = 0x48,
    PRE_RANGE_CONFIG_VCSEL_PERIOD = 0x50,
    PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI = 0x51,
    FINAL_RANGE_CONFIG_VCSEL_PERIOD = 0x70,
    FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI = 0x71,
    MSRC_CONFIG_TIMEOUT_MACROP = 0x46,
    IDENTIFICATION_MODEL_ID = 0xC0,
    OSC_CALIBRATE_VAL = 0xF8,
    GLOBAL_CONFIG_VCSEL_WIDTH = 0x32,
    GLOBAL_CONFIG_SPAD_ENABLES_REF_0 = 0xB0,
    GLOBAL_CONFIG_REF_EN_START_SELECT = 0xB6,
    DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD = 0x4E,
    DYNAMIC_SPAD_REF_EN_START_OFFSET = 0x4F,
    VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV = 0x89,
    ALGO_PHASECAL_LIM = 0x30,
    ALGO_PHASECAL_CONFIG_TIMEOUT = 0x30,
};

typedef enum { VCSEL_PERIOD_PRE_RANGE, VCSEL_PERIOD_FINAL_RANGE } vcsel_period_type_t;

typedef struct {
    bool tcc;
    bool msrc;
    bool dss;
    bool pre_range;
    bool final_range;
} sequence_step_enables_t;

typedef struct {
    uint16_t pre_range_vcsel_period_pclks;
    uint16_t final_range_vcsel_period_pclks;
    uint16_t msrc_dss_tcc_mclks;
    uint16_t pre_range_mclks;
    uint16_t final_range_mclks;
    uint32_t msrc_dss_tcc_us;
    uint32_t pre_range_us;
    uint32_t final_range_us;
} sequence_step_timeouts_t;

static esp_err_t write_reg(vl53l0x_t *dev, uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return i2c_master_transmit(dev->i2c, buf, sizeof(buf), I2C_TIMEOUT_MS);
}

static esp_err_t write_reg16(vl53l0x_t *dev, uint8_t reg, uint16_t value)
{
    uint8_t buf[3] = {reg, (uint8_t)(value >> 8), (uint8_t)value};
    return i2c_master_transmit(dev->i2c, buf, sizeof(buf), I2C_TIMEOUT_MS);
}

static esp_err_t write_multi(vl53l0x_t *dev, uint8_t reg, const uint8_t *src, uint8_t count)
{
    uint8_t buf[7];
    if (count > sizeof(buf) - 1) {
        return ESP_ERR_INVALID_SIZE;
    }
    buf[0] = reg;
    for (uint8_t i = 0; i < count; i++) {
        buf[i + 1] = src[i];
    }
    return i2c_master_transmit(dev->i2c, buf, count + 1, I2C_TIMEOUT_MS);
}

static esp_err_t read_reg(vl53l0x_t *dev, uint8_t reg, uint8_t *value)
{
    return i2c_master_transmit_receive(dev->i2c, &reg, 1, value, 1, I2C_TIMEOUT_MS);
}

static esp_err_t read_reg16(vl53l0x_t *dev, uint8_t reg, uint16_t *value)
{
    uint8_t buf[2];
    esp_err_t err = i2c_master_transmit_receive(dev->i2c, &reg, 1, buf, 2, I2C_TIMEOUT_MS);
    if (err == ESP_OK) {
        *value = ((uint16_t)buf[0] << 8) | buf[1];
    }
    return err;
}

static esp_err_t read_multi(vl53l0x_t *dev, uint8_t reg, uint8_t *dst, uint8_t count)
{
    return i2c_master_transmit_receive(dev->i2c, &reg, 1, dst, count, I2C_TIMEOUT_MS);
}

static bool timeout_expired(vl53l0x_t *dev, TickType_t start)
{
    if (dev->io_timeout_ms == 0) {
        return false;
    }
    return (xTaskGetTickCount() - start) > pdMS_TO_TICKS(dev->io_timeout_ms);
}

static uint16_t decode_timeout(uint16_t reg_val)
{
    return (uint16_t)((reg_val & 0x00FF) << (uint16_t)((reg_val & 0xFF00) >> 8)) + 1;
}

static uint16_t encode_timeout(uint32_t timeout_mclks)
{
    uint32_t ls_byte = 0;
    uint16_t ms_byte = 0;

    if (timeout_mclks == 0) {
        return 0;
    }
    ls_byte = timeout_mclks - 1;
    while ((ls_byte & 0xFFFFFF00) > 0) {
        ls_byte >>= 1;
        ms_byte++;
    }
    return (ms_byte << 8) | (ls_byte & 0xFF);
}

static uint32_t timeout_mclks_to_us(uint16_t timeout_period_mclks, uint8_t vcsel_period_pclks)
{
    uint32_t macro_period_ns = calcMacroPeriod(vcsel_period_pclks);
    return ((timeout_period_mclks * macro_period_ns) + 500) / 1000;
}

static uint32_t timeout_us_to_mclks(uint32_t timeout_period_us, uint8_t vcsel_period_pclks)
{
    uint32_t macro_period_ns = calcMacroPeriod(vcsel_period_pclks);
    return (((timeout_period_us * 1000) + (macro_period_ns / 2)) / macro_period_ns);
}

static esp_err_t get_vcsel_pulse_period(vl53l0x_t *dev, vcsel_period_type_t type, uint8_t *period)
{
    uint8_t reg = (type == VCSEL_PERIOD_PRE_RANGE) ? PRE_RANGE_CONFIG_VCSEL_PERIOD
                                                   : FINAL_RANGE_CONFIG_VCSEL_PERIOD;
    uint8_t val;
    ESP_RETURN_ON_ERROR(read_reg(dev, reg, &val), TAG, "read vcsel");
    *period = decodeVcselPeriod(val);
    return ESP_OK;
}

static void get_sequence_step_enables(vl53l0x_t *dev, sequence_step_enables_t *enables)
{
    uint8_t sequence_config;
    if (read_reg(dev, SYSTEM_SEQUENCE_CONFIG, &sequence_config) != ESP_OK) {
        return;
    }
    enables->tcc = (sequence_config >> 4) & 0x1;
    enables->dss = (sequence_config >> 3) & 0x1;
    enables->msrc = (sequence_config >> 2) & 0x1;
    enables->pre_range = (sequence_config >> 6) & 0x1;
    enables->final_range = (sequence_config >> 7) & 0x1;
}

static esp_err_t get_sequence_step_timeouts(vl53l0x_t *dev, const sequence_step_enables_t *enables,
                                            sequence_step_timeouts_t *timeouts)
{
    uint8_t period;
    uint16_t reg16;

    ESP_RETURN_ON_ERROR(get_vcsel_pulse_period(dev, VCSEL_PERIOD_PRE_RANGE, &period), TAG,
                        "pre vcsel");
    timeouts->pre_range_vcsel_period_pclks = period;

    ESP_RETURN_ON_ERROR(read_reg(dev, MSRC_CONFIG_TIMEOUT_MACROP, &period), TAG, "msrc");
    timeouts->msrc_dss_tcc_mclks = period + 1;
    timeouts->msrc_dss_tcc_us =
        timeout_mclks_to_us(timeouts->msrc_dss_tcc_mclks, timeouts->pre_range_vcsel_period_pclks);

    ESP_RETURN_ON_ERROR(read_reg16(dev, PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI, &reg16), TAG,
                        "pre timeout");
    timeouts->pre_range_mclks = decode_timeout(reg16);
    timeouts->pre_range_us =
        timeout_mclks_to_us(timeouts->pre_range_mclks, timeouts->pre_range_vcsel_period_pclks);

    ESP_RETURN_ON_ERROR(get_vcsel_pulse_period(dev, VCSEL_PERIOD_FINAL_RANGE, &period), TAG,
                        "final vcsel");
    timeouts->final_range_vcsel_period_pclks = period;

    ESP_RETURN_ON_ERROR(read_reg16(dev, FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI, &reg16), TAG,
                        "final timeout");
    timeouts->final_range_mclks = decode_timeout(reg16);
    if (enables->pre_range) {
        timeouts->final_range_mclks -= timeouts->pre_range_mclks;
    }
    timeouts->final_range_us = timeout_mclks_to_us(timeouts->final_range_mclks,
                                                    timeouts->final_range_vcsel_period_pclks);
    return ESP_OK;
}

static esp_err_t set_signal_rate_limit(vl53l0x_t *dev, float limit_mcps)
{
    if (limit_mcps < 0.0f || limit_mcps > 511.99f) {
        return ESP_ERR_INVALID_ARG;
    }
    return write_reg16(dev, FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT,
                       (uint16_t)(limit_mcps * (1 << 7)));
}

static esp_err_t set_measurement_timing_budget(vl53l0x_t *dev, uint32_t budget_us)
{
    const uint16_t start_overhead = 1910;
    const uint16_t end_overhead = 960;
    const uint16_t msrc_overhead = 660;
    const uint16_t tcc_overhead = 590;
    const uint16_t dss_overhead = 690;
    const uint16_t pre_range_overhead = 660;
    const uint16_t final_range_overhead = 550;

    sequence_step_enables_t enables;
    sequence_step_timeouts_t timeouts;
    uint32_t used_budget_us = start_overhead + end_overhead;

    get_sequence_step_enables(dev, &enables);
    ESP_RETURN_ON_ERROR(get_sequence_step_timeouts(dev, &enables, &timeouts), TAG, "timeouts");

    if (enables.tcc) {
        used_budget_us += timeouts.msrc_dss_tcc_us + tcc_overhead;
    }
    if (enables.dss) {
        used_budget_us += 2 * (timeouts.msrc_dss_tcc_us + dss_overhead);
    } else if (enables.msrc) {
        used_budget_us += timeouts.msrc_dss_tcc_us + msrc_overhead;
    }
    if (enables.pre_range) {
        used_budget_us += timeouts.pre_range_us + pre_range_overhead;
    }
    if (!enables.final_range) {
        dev->measurement_timing_budget_us = budget_us;
        return ESP_OK;
    }

    used_budget_us += final_range_overhead;
    if (used_budget_us > budget_us) {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t final_range_timeout_us = budget_us - used_budget_us;
    uint32_t final_range_timeout_mclks =
        timeout_us_to_mclks(final_range_timeout_us, timeouts.final_range_vcsel_period_pclks);
    if (enables.pre_range) {
        final_range_timeout_mclks += timeouts.pre_range_mclks;
    }

    ESP_RETURN_ON_ERROR(
        write_reg16(dev, FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI, encode_timeout(final_range_timeout_mclks)),
        TAG, "final timeout write");

    dev->measurement_timing_budget_us = budget_us;
    return ESP_OK;
}

static esp_err_t get_measurement_timing_budget(vl53l0x_t *dev, uint32_t *budget_us)
{
    const uint16_t start_overhead = 1910;
    const uint16_t end_overhead = 960;
    const uint16_t msrc_overhead = 660;
    const uint16_t tcc_overhead = 590;
    const uint16_t dss_overhead = 690;
    const uint16_t pre_range_overhead = 660;
    const uint16_t final_range_overhead = 550;

    sequence_step_enables_t enables;
    sequence_step_timeouts_t timeouts;
    uint32_t budget = start_overhead + end_overhead;

    get_sequence_step_enables(dev, &enables);
    ESP_RETURN_ON_ERROR(get_sequence_step_timeouts(dev, &enables, &timeouts), TAG, "timeouts");

    if (enables.tcc) {
        budget += timeouts.msrc_dss_tcc_us + tcc_overhead;
    }
    if (enables.dss) {
        budget += 2 * (timeouts.msrc_dss_tcc_us + dss_overhead);
    } else if (enables.msrc) {
        budget += timeouts.msrc_dss_tcc_us + msrc_overhead;
    }
    if (enables.pre_range) {
        budget += timeouts.pre_range_us + pre_range_overhead;
    }
    if (enables.final_range) {
        budget += timeouts.final_range_us + final_range_overhead;
    }

    dev->measurement_timing_budget_us = budget;
    *budget_us = budget;
    return ESP_OK;
}

static bool perform_single_ref_calibration(vl53l0x_t *dev, uint8_t vhv_init_byte)
{
    uint8_t status;
    TickType_t start = xTaskGetTickCount();

    if (write_reg(dev, SYSRANGE_START, 0x01 | vhv_init_byte) != ESP_OK) {
        return false;
    }

    do {
        if (read_reg(dev, RESULT_INTERRUPT_STATUS, &status) != ESP_OK) {
            return false;
        }
        if ((status & 0x07) != 0) {
            break;
        }
    } while (!timeout_expired(dev, start));

    if ((status & 0x07) == 0) {
        return false;
    }

    if (write_reg(dev, SYSTEM_INTERRUPT_CLEAR, 0x01) != ESP_OK) {
        return false;
    }
    return write_reg(dev, SYSRANGE_START, 0x00) == ESP_OK;
}

static bool get_spad_info(vl53l0x_t *dev, uint8_t *count, bool *type_is_aperture)
{
    uint8_t tmp;
    TickType_t start = xTaskGetTickCount();

    if (write_reg(dev, 0x80, 0x01) != ESP_OK) {
        return false;
    }
    write_reg(dev, 0xFF, 0x01);
    write_reg(dev, 0x00, 0x00);
    write_reg(dev, 0xFF, 0x06);
    read_reg(dev, 0x83, &tmp);
    write_reg(dev, 0x83, tmp | 0x04);
    write_reg(dev, 0xFF, 0x07);
    write_reg(dev, 0x81, 0x01);
    write_reg(dev, 0x80, 0x01);
    write_reg(dev, 0x94, 0x6b);
    write_reg(dev, 0x83, 0x00);

    do {
        if (read_reg(dev, 0x83, &tmp) != ESP_OK) {
            return false;
        }
        if (tmp != 0x00) {
            break;
        }
    } while (!timeout_expired(dev, start));

    if (tmp == 0x00) {
        return false;
    }

    write_reg(dev, 0x83, 0x01);
    read_reg(dev, 0x92, &tmp);
    *count = tmp & 0x7f;
    *type_is_aperture = (tmp >> 7) & 0x01;

    write_reg(dev, 0x81, 0x00);
    write_reg(dev, 0xFF, 0x06);
    read_reg(dev, 0x83, &tmp);
    write_reg(dev, 0x83, tmp & ~0x04);
    write_reg(dev, 0xFF, 0x01);
    write_reg(dev, 0x00, 0x01);
    write_reg(dev, 0xFF, 0x00);
    write_reg(dev, 0x80, 0x00);
    return true;
}

static esp_err_t load_tuning_settings(vl53l0x_t *dev)
{
    write_reg(dev, 0xFF, 0x01);
    write_reg(dev, 0x00, 0x00);
    write_reg(dev, 0xFF, 0x00);
    write_reg(dev, 0x09, 0x00);
    write_reg(dev, 0x10, 0x00);
    write_reg(dev, 0x11, 0x00);
    write_reg(dev, 0x24, 0x01);
    write_reg(dev, 0x25, 0xFF);
    write_reg(dev, 0x75, 0x00);
    write_reg(dev, 0xFF, 0x01);
    write_reg(dev, 0x4E, 0x2C);
    write_reg(dev, 0x48, 0x00);
    write_reg(dev, 0x30, 0x20);
    write_reg(dev, 0xFF, 0x00);
    write_reg(dev, 0x30, 0x09);
    write_reg(dev, 0x54, 0x00);
    write_reg(dev, 0x31, 0x04);
    write_reg(dev, 0x32, 0x03);
    write_reg(dev, 0x40, 0x83);
    write_reg(dev, 0x46, 0x25);
    write_reg(dev, 0x60, 0x00);
    write_reg(dev, 0x27, 0x00);
    write_reg(dev, 0x50, 0x06);
    write_reg(dev, 0x51, 0x00);
    write_reg(dev, 0x52, 0x96);
    write_reg(dev, 0x56, 0x08);
    write_reg(dev, 0x57, 0x30);
    write_reg(dev, 0x61, 0x00);
    write_reg(dev, 0x62, 0x00);
    write_reg(dev, 0x64, 0x00);
    write_reg(dev, 0x65, 0x00);
    write_reg(dev, 0x66, 0xA0);
    write_reg(dev, 0xFF, 0x01);
    write_reg(dev, 0x22, 0x32);
    write_reg(dev, 0x47, 0x14);
    write_reg(dev, 0x49, 0xFF);
    write_reg(dev, 0x4A, 0x00);
    write_reg(dev, 0xFF, 0x00);
    write_reg(dev, 0x7A, 0x0A);
    write_reg(dev, 0x7B, 0x00);
    write_reg(dev, 0x78, 0x21);
    write_reg(dev, 0xFF, 0x01);
    write_reg(dev, 0x23, 0x34);
    write_reg(dev, 0x42, 0x00);
    write_reg(dev, 0x44, 0xFF);
    write_reg(dev, 0x45, 0x26);
    write_reg(dev, 0x46, 0x05);
    write_reg(dev, 0x40, 0x40);
    write_reg(dev, 0x0E, 0x06);
    write_reg(dev, 0x20, 0x1A);
    write_reg(dev, 0x43, 0x40);
    write_reg(dev, 0xFF, 0x00);
    write_reg(dev, 0x34, 0x03);
    write_reg(dev, 0x35, 0x44);
    write_reg(dev, 0xFF, 0x01);
    write_reg(dev, 0x31, 0x04);
    write_reg(dev, 0x4B, 0x09);
    write_reg(dev, 0x4C, 0x05);
    write_reg(dev, 0x4D, 0x04);
    write_reg(dev, 0xFF, 0x00);
    write_reg(dev, 0x44, 0x00);
    write_reg(dev, 0x45, 0x20);
    write_reg(dev, 0x47, 0x08);
    write_reg(dev, 0x48, 0x28);
    write_reg(dev, 0x67, 0x00);
    write_reg(dev, 0x70, 0x04);
    write_reg(dev, 0x71, 0x01);
    write_reg(dev, 0x72, 0xFE);
    write_reg(dev, 0x76, 0x00);
    write_reg(dev, 0x77, 0x00);
    write_reg(dev, 0xFF, 0x01);
    write_reg(dev, 0x0D, 0x01);
    write_reg(dev, 0xFF, 0x00);
    write_reg(dev, 0x80, 0x01);
    write_reg(dev, 0x01, 0xF8);
    write_reg(dev, 0xFF, 0x01);
    write_reg(dev, 0x8E, 0x01);
    write_reg(dev, 0x00, 0x01);
    write_reg(dev, 0xFF, 0x00);
    write_reg(dev, 0x80, 0x00);
    return ESP_OK;
}

esp_err_t vl53l0x_init(vl53l0x_t *dev, bool io_2v8)
{
    uint8_t model_id;
    uint8_t spad_count;
    bool spad_type_is_aperture;
    uint8_t ref_spad_map[6];
    uint32_t budget_us;

    ESP_RETURN_ON_FALSE(dev && dev->i2c, ESP_ERR_INVALID_ARG, TAG, "invalid dev");

    if (dev->io_timeout_ms == 0) {
        dev->io_timeout_ms = 500;
    }

    ESP_RETURN_ON_ERROR(read_reg(dev, IDENTIFICATION_MODEL_ID, &model_id), TAG, "model id");
    if (model_id != 0xEE) {
        ESP_LOGE(TAG, "unexpected model ID 0x%02X (expected 0xEE)", model_id);
        return ESP_ERR_NOT_FOUND;
    }

    if (io_2v8) {
        uint8_t reg;
        ESP_RETURN_ON_ERROR(read_reg(dev, VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV, &reg), TAG, "vhv");
        ESP_RETURN_ON_ERROR(write_reg(dev, VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV, reg | 0x01), TAG,
                            "vhv write");
    }

    write_reg(dev, 0x88, 0x00);
    write_reg(dev, 0x80, 0x01);
    write_reg(dev, 0xFF, 0x01);
    write_reg(dev, 0x00, 0x00);
    ESP_RETURN_ON_ERROR(read_reg(dev, 0x91, &dev->stop_variable), TAG, "stop var");
    write_reg(dev, 0x00, 0x01);
    write_reg(dev, 0xFF, 0x00);
    write_reg(dev, 0x80, 0x00);

    uint8_t msrc;
    ESP_RETURN_ON_ERROR(read_reg(dev, MSRC_CONFIG_CONTROL, &msrc), TAG, "msrc");
    ESP_RETURN_ON_ERROR(write_reg(dev, MSRC_CONFIG_CONTROL, msrc | 0x12), TAG, "msrc write");
    ESP_RETURN_ON_ERROR(set_signal_rate_limit(dev, 0.25f), TAG, "signal rate");
    ESP_RETURN_ON_ERROR(write_reg(dev, SYSTEM_SEQUENCE_CONFIG, 0xFF), TAG, "seq config");

    if (!get_spad_info(dev, &spad_count, &spad_type_is_aperture)) {
        ESP_LOGE(TAG, "SPAD info failed");
        return ESP_FAIL;
    }

    ESP_RETURN_ON_ERROR(read_multi(dev, GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6), TAG,
                        "spad map");

    write_reg(dev, 0xFF, 0x01);
    write_reg(dev, DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00);
    write_reg(dev, DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C);
    write_reg(dev, 0xFF, 0x00);
    write_reg(dev, GLOBAL_CONFIG_REF_EN_START_SELECT, 0xB4);

    uint8_t first_spad = spad_type_is_aperture ? 12 : 0;
    uint8_t spads_enabled = 0;
    for (uint8_t i = 0; i < 48; i++) {
        if (i < first_spad || spads_enabled == spad_count) {
            ref_spad_map[i / 8] &= (uint8_t) ~(1 << (i % 8));
        } else if ((ref_spad_map[i / 8] >> (i % 8)) & 0x1) {
            spads_enabled++;
        }
    }
    ESP_RETURN_ON_ERROR(write_multi(dev, GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6), TAG,
                        "spad write");

    ESP_RETURN_ON_ERROR(load_tuning_settings(dev), TAG, "tuning");

    uint8_t gpio_mux;
    ESP_RETURN_ON_ERROR(read_reg(dev, GPIO_HV_MUX_ACTIVE_HIGH, &gpio_mux), TAG, "gpio mux");
    write_reg(dev, SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);
    write_reg(dev, GPIO_HV_MUX_ACTIVE_HIGH, gpio_mux & ~0x10);
    write_reg(dev, SYSTEM_INTERRUPT_CLEAR, 0x01);

    ESP_RETURN_ON_ERROR(get_measurement_timing_budget(dev, &budget_us), TAG, "get budget");
    ESP_RETURN_ON_ERROR(set_measurement_timing_budget(dev, budget_us), TAG, "set budget");
    ESP_RETURN_ON_ERROR(write_reg(dev, SYSTEM_SEQUENCE_CONFIG, 0xE8), TAG, "seq e8");

    write_reg(dev, SYSTEM_SEQUENCE_CONFIG, 0x01);
    if (!perform_single_ref_calibration(dev, 0x40)) {
        return ESP_FAIL;
    }
    write_reg(dev, SYSTEM_SEQUENCE_CONFIG, 0x02);
    if (!perform_single_ref_calibration(dev, 0x00)) {
        return ESP_FAIL;
    }
    write_reg(dev, SYSTEM_SEQUENCE_CONFIG, 0xE8);

    ESP_LOGI(TAG, "initialized (timing budget %lu us)", (unsigned long)budget_us);
    return ESP_OK;
}

static uint16_t read_range_continuous_mm(vl53l0x_t *dev)
{
    uint8_t status;
    uint16_t range = 65535;
    TickType_t start = xTaskGetTickCount();

    do {
        if (read_reg(dev, RESULT_INTERRUPT_STATUS, &status) != ESP_OK) {
            return 65535;
        }
        if ((status & 0x07) != 0) {
            break;
        }
    } while (!timeout_expired(dev, start));

    if ((status & 0x07) == 0) {
        return 65535;
    }

    if (read_reg16(dev, RESULT_RANGE_STATUS + 10, &range) != ESP_OK) {
        return 65535;
    }
    write_reg(dev, SYSTEM_INTERRUPT_CLEAR, 0x01);
    return range;
}

uint16_t vl53l0x_read_range_single_mm(vl53l0x_t *dev)
{
    uint8_t start;
    TickType_t tick_start = xTaskGetTickCount();

    write_reg(dev, 0x80, 0x01);
    write_reg(dev, 0xFF, 0x01);
    write_reg(dev, 0x00, 0x00);
    write_reg(dev, 0x91, dev->stop_variable);
    write_reg(dev, 0x00, 0x01);
    write_reg(dev, 0xFF, 0x00);
    write_reg(dev, 0x80, 0x00);
    write_reg(dev, SYSRANGE_START, 0x01);

    do {
        if (read_reg(dev, SYSRANGE_START, &start) != ESP_OK) {
            return 65535;
        }
        if ((start & 0x01) == 0) {
            break;
        }
    } while (!timeout_expired(dev, tick_start));

    if (start & 0x01) {
        return 65535;
    }

    return read_range_continuous_mm(dev);
}
