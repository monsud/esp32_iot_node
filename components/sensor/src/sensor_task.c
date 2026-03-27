/**
 * @file    sensor_task.c
 * @brief   Periodic sensor acquisition task.
 *
 * Responsibilities:
 *  - Initialise the sensor driver.
 *  - Poll the sensor at SENSOR_POLL_INTERVAL_MS.
 *  - Validate data (delegated to driver).
 *  - Post valid samples to g_sensor_queue (non-blocking drop on full).
 *  - Log errors to g_log_queue.
 *  - Subscribe to the task watchdog.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_task_wdt.h"

#include "config.h"
#include "types.h"
#include "rtos_handles.h"
#include "sensor_driver.h"
#include "sensor_task.h"
#include "logger_task.h"

static const char *TAG = "SENSOR";

void sensor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Task started.");

    /* Initialise hardware driver */
    if (sensor_driver_init() != ESP_OK) {
        ESP_LOGE(TAG, "Driver init failed — task suspended.");
        logger_post(LOG_LEVEL_ERROR, TAG, "Sensor driver init failed.");
        vTaskSuspend(NULL);
    }

    /* Subscribe to watchdog */
    esp_task_wdt_add(NULL);

    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        /* Pet the watchdog */
        esp_task_wdt_reset();

        sensor_data_t sample;
        esp_err_t err = sensor_driver_read(&sample);

        if (err == ESP_OK && sample.valid) {
            /* Non-blocking send — drop oldest if queue is full */
            if (xQueueSend(g_sensor_queue, &sample, 0) != pdTRUE) {
                ESP_LOGW(TAG, "Sensor queue full — sample dropped.");
                logger_post(LOG_LEVEL_WARNING, TAG,
                            "Sensor queue full — sample dropped.");
            }
        } else {
            ESP_LOGE(TAG, "Sensor read error: %s", esp_err_to_name(err));
            logger_post(LOG_LEVEL_ERROR, TAG, "Sensor read error.");
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SENSOR_POLL_INTERVAL_MS));
    }
    /* unreachable */
}
