/**
 * @file vl53l0x_gy530.c
 * @brief Дальномер «в луче» до ~2 м: время пролёта лазера → миллиметры.
 *
 * Один замер — одно расстояние по оси модуля (куда смотрит GY-530 на корпусе).
 */

#include "vl53l0x_gy530.h"

#include "vl53l0x_gy530_config.h"
#include "vl53l0x.h"

#include "distance_sensor.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

static const char *TAG = "VL53L0X_GY530";

typedef struct {
    vl53l0x_t vl53;
    i2c_master_bus_handle_t bus;
    i2c_master_dev_handle_t i2c_dev;
} vl53l0x_gy530_ctx_t;

static vl53l0x_gy530_ctx_t s_ctx;

/** Пробуждение чипа по XSHUT и поднятие I2C-шины. */
static esp_err_t bus_init(vl53l0x_gy530_ctx_t *ctx)
{
    esp_err_t ret;

    gpio_reset_pin(VL53L0X_GY530_XSHUT_GPIO);
    gpio_set_direction(VL53L0X_GY530_XSHUT_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(VL53L0X_GY530_XSHUT_GPIO, 0);
    esp_rom_delay_us(VL53L0X_GY530_XSHUT_LOW_US);
    gpio_set_level(VL53L0X_GY530_XSHUT_GPIO, 1);
    esp_rom_delay_us(VL53L0X_GY530_XSHUT_BOOT_US);

    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = VL53L0X_GY530_I2C_PORT,
        .scl_io_num = VL53L0X_GY530_I2C_SCL_GPIO,
        .sda_io_num = VL53L0X_GY530_I2C_SDA_GPIO,
        .glitch_ignore_cnt = VL53L0X_GY530_I2C_GLITCH_IGNORE_CNT,
        .flags.enable_internal_pullup = VL53L0X_GY530_I2C_INTERNAL_PULLUP,
    };

    ret = i2c_new_master_bus(&bus_config, &ctx->bus);
    if (ret != ESP_OK) {
        return ret;
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = VL53L0X_GY530_I2C_ADDR_7BIT,
        .scl_speed_hz = VL53L0X_GY530_I2C_FREQ_HZ,
    };

    ret = i2c_master_bus_add_device(ctx->bus, &dev_config, &ctx->i2c_dev);
    if (ret != ESP_OK) {
        i2c_del_master_bus(ctx->bus);
        ctx->bus = NULL;
        return ret;
    }

    ret = i2c_master_probe(ctx->bus, VL53L0X_GY530_I2C_ADDR_7BIT, VL53L0X_GY530_I2C_PROBE_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "not detected on I2C 0x%02X: %s", VL53L0X_GY530_I2C_ADDR_7BIT, esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "detected on I2C bus");
    }

    return ESP_OK;
}

static esp_err_t vl53l0x_gy530_init(distance_sensor_t *self)
{
    vl53l0x_gy530_ctx_t *ctx = self->impl;

    ESP_LOGI(TAG, "init (SDA=%d SCL=%d XSHUT=%d)", VL53L0X_GY530_I2C_SDA_GPIO, VL53L0X_GY530_I2C_SCL_GPIO,
             VL53L0X_GY530_XSHUT_GPIO);

    ESP_RETURN_ON_ERROR(bus_init(ctx), TAG, "bus");

    ctx->vl53.i2c = ctx->i2c_dev;
    ctx->vl53.io_timeout_ms = VL53L0X_GY530_IO_TIMEOUT_MS;

    return vl53l0x_init(&ctx->vl53, VL53L0X_GY530_IO_2V8_MODE);
}

/** Одиночный снимок дистанции (мм) вдоль луча — для hazard_filter и зон мигания. */
static esp_err_t vl53l0x_gy530_read_mm(distance_sensor_t *self, uint16_t *mm_out)
{
    vl53l0x_gy530_ctx_t *ctx = self->impl;
    uint16_t mm = vl53l0x_read_range_single_mm(&ctx->vl53);

    *mm_out = mm;
    return (mm == DISTANCE_MM_INVALID) ? ESP_ERR_INVALID_RESPONSE : ESP_OK;
}

static const distance_sensor_t s_sensor = {
    .name = "GY-530 (VL53L0X)",
    .init = vl53l0x_gy530_init,
    .deinit = NULL,
    .read_mm = vl53l0x_gy530_read_mm,
    .impl = &s_ctx,
};

esp_err_t vl53l0x_gy530_get(distance_sensor_t *out)
{
    if (!out) {
        return ESP_ERR_INVALID_ARG;
    }
    *out = s_sensor;
    return ESP_OK;
}
