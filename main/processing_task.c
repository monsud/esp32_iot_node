/**
 * @file    processing_task.c
 * @brief   Data-processing task — formats sensor_data_t into mqtt_message_t.
 *
 * Sits between sensor_task and mqtt_task, decoupling acquisition from
 * publication.  Performs:
 *   - JSON serialisation (no heap — uses snprintf into fixed buffers)
 *   - Topic construction
 *   - Basic sanity re-check
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_task_wdt.h"

#include "config.h"
#include "types.h"
#include "rtos_handles.h"
#include "logger_task.h"

static const char *TAG = "PROCESSING";

/* ─────────────────────────────────────────────────────────────────────────────
 * Internal helpers
 * ─────────────────────────────────────────────────────────────────────────── */
static void build_message(const sensor_data_t *in, mqtt_message_t *out)
{
    /* Topic: <prefix>/sensor/data */
    snprintf(out->topic, sizeof(out->topic),
             "%s/sensor/data", MQTT_TOPIC_PREFIX);

    /* Minimal JSON payload — no heap allocation */
    snprintf(out->payload, sizeof(out->payload),
             "{"
             "\"ts\":%lu,"
             "\"temp\":%.2f,"
             "\"hum\":%.2f,"
             "\"pres\":%.2f"
             "}",
             (unsigned long)in->timestamp_s,
             in->temperature_c,
             in->humidity_pct,
             in->pressure_hpa);

    out->qos    = MQTT_QOS;
    out->retain = false;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Task entry point
 * ─────────────────────────────────────────────────────────────────────────── */
void processing_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Task started.");
    esp_task_wdt_add(NULL);

    sensor_data_t   in;
    mqtt_message_t  out;

    for (;;) {
        esp_task_wdt_reset();

        /* Block until a sample arrives (or watchdog fires) */
        if (xQueueReceive(g_sensor_queue, &in,
                          pdMS_TO_TICKS(WATCHDOG_TIMEOUT_S * 1000 / 2))
            != pdTRUE) {
            /* Nothing received within half the watchdog window — log and loop */
            ESP_LOGW(TAG, "No sensor data received.");
            logger_post(LOG_LEVEL_WARNING, TAG, "No sensor data received.");
            continue;
        }

        if (!in.valid) {
            logger_post(LOG_LEVEL_WARNING, TAG, "Invalid sample — skipping.");
            continue;
        }

        build_message(&in, &out);

        if (xQueueSend(g_mqtt_queue, &out, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGW(TAG, "MQTT queue full — message dropped.");
            logger_post(LOG_LEVEL_WARNING, TAG,
                        "MQTT queue full — message dropped.");
        }
    }
}
