/**
 * @file    sensor_task.h
 * @brief   FreeRTOS task — periodic sensor acquisition.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Sensor acquisition task entry point.
 *         Reads sensor at SENSOR_POLL_INTERVAL_MS, validates data,
 *         and posts sensor_data_t to g_sensor_queue.
 */
void sensor_task(void *pvParameters);

#ifdef __cplusplus
}
#endif
