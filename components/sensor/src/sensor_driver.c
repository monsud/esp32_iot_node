/**
 * @file    sensor_driver.c
 * @brief   Environmental sensor driver (simulated BME280).
 *
 * Production code: replace the simulated readings with actual
 * I2C transactions (esp_driver_i2c / bme280 component).
 */

#include <math.h>
#include <time.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_random.h"

#include "config.h"
#include "types.h"
#include "sensor_driver.h"

static const char *TAG = "SENSOR_DRV";

/* Simple sine-wave simulation seed */
static uint32_t s_tick = 0;

/* ─────────────────────────────────────────────────────────────────────────────
 * Public API
 * ─────────────────────────────────────────────────────────────────────────── */
esp_err_t sensor_driver_init(void)
{
    /*
     * Real init: configure I2C master, probe device address, read chip-id.
     * Here we just log the simulation mode.
     */
    ESP_LOGI(TAG, "Sensor driver initialised (SIMULATED mode).");
    s_tick = 0;
    return ESP_OK;
}

esp_err_t sensor_driver_read(sensor_data_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Deterministic simulation:
     *   temperature oscillates 20–30 °C  (sine, period ~60 samples)
     *   humidity    oscillates 40–60 %RH (cosine)
     *   pressure    stays near 1013 hPa  (small random walk)
     */
    float angle = (float)s_tick * (2.0f * M_PI / 60.0f);
    s_tick++;

    out->temperature_c = 25.0f + 5.0f  * sinf(angle);
    out->humidity_pct  = 50.0f + 10.0f * cosf(angle);
    out->pressure_hpa  = 1013.0f + ((float)(esp_random() % 100) - 50.0f) / 50.0f;
    out->timestamp_s   = (uint32_t)time(NULL);

    /* Validate physical range */
    if (out->temperature_c < SENSOR_TEMP_MIN_C ||
        out->temperature_c > SENSOR_TEMP_MAX_C ||
        out->humidity_pct  < SENSOR_HUM_MIN_PCT ||
        out->humidity_pct  > SENSOR_HUM_MAX_PCT) {

        ESP_LOGE(TAG, "Sensor data out of range — discarding.");
        out->valid = false;
        return ESP_ERR_INVALID_RESPONSE;
    }

    out->valid = true;
    return ESP_OK;
}
