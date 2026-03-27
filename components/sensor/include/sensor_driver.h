/**
 * @file    sensor_driver.h
 * @brief   Hardware Abstraction Layer for the environmental sensor.
 *
 * Real implementation reads an I2C sensor (e.g. BME280).
 * A compile-time flag SENSOR_SIMULATE switches to a deterministic
 * simulator so the firmware can be tested without hardware.
 */

#pragma once

#include "esp_err.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initialise the sensor hardware (or simulator).
 * @return ESP_OK on success, ESP_FAIL otherwise.
 */
esp_err_t sensor_driver_init(void);

/**
 * @brief  Read one sample from the sensor.
 * @param  out  Pointer to caller-provided structure; filled on ESP_OK.
 * @return ESP_OK on success, ESP_ERR_INVALID_RESPONSE if data is out
 *         of physical range.
 */
esp_err_t sensor_driver_read(sensor_data_t *out);

#ifdef __cplusplus
}
#endif
