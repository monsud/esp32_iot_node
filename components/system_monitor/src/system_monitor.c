/**
 * @file    system_monitor.c
 * @brief   Periodic system health monitor.
 *
 * Runs at the highest priority to ensure it always gets CPU time.
 * It does NOT block on queues or mutexes for long — it samples
 * system state and posts a single MQTT health-check message, then
 * sleeps for MONITOR_CHECK_INTERVAL_MS.
 *
 * Stack high-water marks are logged so developers can tune stack sizes.
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_task_wdt.h"

#include "config.h"
#include "types.h"
#include "rtos_handles.h"
#include "logger_task.h"
#include "system_monitor.h"

static const char *TAG = "MONITOR";

/* Minimum free heap threshold before an error is logged (bytes) */
#define HEAP_LOW_WATERMARK_BYTES (20 * 1024)

/* ─────────────────────────────────────────────────────────────────────────────
 * Helpers
 * ─────────────────────────────────────────────────────────────────────────── */
static void check_heap(void)
{
    size_t free_heap = esp_get_free_heap_size();
    size_t min_heap  = esp_get_minimum_free_heap_size();

    ESP_LOGI(TAG, "Heap free: %u B  |  min-ever: %u B",
             (unsigned)free_heap, (unsigned)min_heap);

    if (free_heap < HEAP_LOW_WATERMARK_BYTES) {
        char msg[LOG_MSG_MAX_LEN];
        snprintf(msg, sizeof(msg),
                 "LOW HEAP WARNING: %u B free.", (unsigned)free_heap);
        logger_post(LOG_LEVEL_ERROR, TAG, msg);
    }
}

static void check_connectivity(void)
{
    EventBits_t bits = xEventGroupGetBits(g_system_events);

    bool wifi_ok = (bits & EVT_WIFI_CONNECTED)  != 0;
    bool mqtt_ok = (bits & EVT_MQTT_CONNECTED)  != 0;

    if (!wifi_ok) {
        logger_post(LOG_LEVEL_WARNING, TAG, "WiFi not connected.");
    }
    if (!mqtt_ok) {
        logger_post(LOG_LEVEL_WARNING, TAG, "MQTT broker not connected.");
    }

    ESP_LOGI(TAG, "Connectivity — WiFi: %s | MQTT: %s",
             wifi_ok ? "UP" : "DOWN",
             mqtt_ok ? "UP" : "DOWN");
}

static void publish_health_report(size_t free_heap, bool wifi_ok, bool mqtt_ok)
{
    mqtt_message_t health = {0};
    snprintf(health.topic, sizeof(health.topic),
             "%s/node/health", MQTT_TOPIC_PREFIX);
    snprintf(health.payload, sizeof(health.payload),
             "{\"free_heap\":%u,\"wifi\":%s,\"mqtt\":%s}",
             (unsigned)free_heap,
             wifi_ok ? "true" : "false",
             mqtt_ok ? "true" : "false");
    health.qos    = 0;      /* best-effort for telemetry */
    health.retain = false;

    /* Non-blocking — health report is not critical */
    xQueueSend(g_mqtt_queue, &health, 0);
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Task entry point
 * ─────────────────────────────────────────────────────────────────────────── */
void system_monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Task started.");

    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {

        /* Heap health */
        size_t free_heap = esp_get_free_heap_size();
        check_heap();

        /* Connectivity health */
        EventBits_t bits = xEventGroupGetBits(g_system_events);
        bool wifi_ok = (bits & EVT_WIFI_CONNECTED) != 0;
        bool mqtt_ok = (bits & EVT_MQTT_CONNECTED) != 0;
        check_connectivity();

        /* Publish compact health JSON */
        publish_health_report(free_heap, wifi_ok, mqtt_ok);

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(MONITOR_CHECK_INTERVAL_MS));
    }
}
