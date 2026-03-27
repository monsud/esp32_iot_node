/**
 * @file    main.c
 * @brief   Application entry point — initialises hardware, RTOS primitives
 *          and launches all tasks.
 *
 * Layered startup sequence:
 *   1. NVS / SPIFFS
 *   2. WiFi (blocking until first connection)
 *   3. RTOS primitives (queues, events, mutexes)
 *   4. Task creation
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_task_wdt.h"

#include "config.h"
#include "types.h"
#include "rtos_handles.h"

/* Component APIs */
#include "wifi_manager.h"
#include "sensor_task.h"
#include "processing_task.h"
#include "mqtt_task.h"
#include "logger_task.h"
#include "system_monitor.h"

static const char *TAG = "MAIN";

/* ─────────────────────────────────────────────────────────────────────────────
 * Global RTOS handle definitions (declared extern in rtos_handles.h)
 * ─────────────────────────────────────────────────────────────────────────── */
QueueHandle_t      g_sensor_queue   = NULL;
QueueHandle_t      g_mqtt_queue     = NULL;
QueueHandle_t      g_log_queue      = NULL;
EventGroupHandle_t g_system_events  = NULL;
SemaphoreHandle_t  g_log_file_mutex = NULL;

/* ─────────────────────────────────────────────────────────────────────────────
 * Static helpers
 * ─────────────────────────────────────────────────────────────────────────── */
static esp_err_t nvs_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

static void rtos_primitives_create(void)
{
    g_sensor_queue = xQueueCreate(QUEUE_SENSOR_DEPTH, sizeof(sensor_data_t));
    g_mqtt_queue   = xQueueCreate(QUEUE_MQTT_DEPTH,   sizeof(mqtt_message_t));
    g_log_queue    = xQueueCreate(QUEUE_LOG_DEPTH,     sizeof(log_entry_t));

    configASSERT(g_sensor_queue);
    configASSERT(g_mqtt_queue);
    configASSERT(g_log_queue);

    g_system_events  = xEventGroupCreate();
    g_log_file_mutex = xSemaphoreCreateMutex();

    configASSERT(g_system_events);
    configASSERT(g_log_file_mutex);

    ESP_LOGI(TAG, "RTOS primitives created.");
}

static void tasks_start(void)
{
    BaseType_t rc;

    rc = xTaskCreate(logger_task,    "logger",    STACK_LOGGER,
                     NULL, PRIORITY_LOGGER,     NULL);
    configASSERT(rc == pdPASS);

    rc = xTaskCreate(sensor_task,    "sensor",    STACK_SENSOR,
                     NULL, PRIORITY_SENSOR,     NULL);
    configASSERT(rc == pdPASS);

    rc = xTaskCreate(processing_task, "processing", STACK_PROCESSING,
                     NULL, PRIORITY_PROCESSING, NULL);
    configASSERT(rc == pdPASS);

    rc = xTaskCreate(mqtt_task,      "mqtt",      STACK_MQTT,
                     NULL, PRIORITY_MQTT,       NULL);
    configASSERT(rc == pdPASS);

    rc = xTaskCreate(system_monitor_task, "monitor", STACK_MONITOR,
                     NULL, PRIORITY_MONITOR,    NULL);
    configASSERT(rc == pdPASS);

    ESP_LOGI(TAG, "All tasks started.");
}

/* ─────────────────────────────────────────────────────────────────────────────
 * app_main
 * ─────────────────────────────────────────────────────────────────────────── */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Industrial IoT Node — Booting ===");

    /* 1. Non-volatile storage */
    ESP_ERROR_CHECK(nvs_init());
    ESP_LOGI(TAG, "NVS initialised.");

    /* 2. RTOS primitives — must exist before any task is created */
    rtos_primitives_create();

    /* 3. Logger task first so subsequent init steps can log to SPIFFS */
    BaseType_t rc = xTaskCreate(logger_task, "logger", STACK_LOGGER,
                                NULL, PRIORITY_LOGGER, NULL);
    configASSERT(rc == pdPASS);

    /* 4. WiFi (blocking, uses event group internally) */
    wifi_manager_init();

    /* 5. Remaining tasks */
    BaseType_t rc2;
    rc2 = xTaskCreate(sensor_task,        "sensor",     STACK_SENSOR,
                      NULL, PRIORITY_SENSOR,     NULL);
    configASSERT(rc2 == pdPASS);

    rc2 = xTaskCreate(processing_task,    "processing", STACK_PROCESSING,
                      NULL, PRIORITY_PROCESSING, NULL);
    configASSERT(rc2 == pdPASS);

    rc2 = xTaskCreate(mqtt_task,          "mqtt",       STACK_MQTT,
                      NULL, PRIORITY_MQTT,       NULL);
    configASSERT(rc2 == pdPASS);

    rc2 = xTaskCreate(system_monitor_task,"monitor",    STACK_MONITOR,
                      NULL, PRIORITY_MONITOR,    NULL);
    configASSERT(rc2 == pdPASS);

    ESP_LOGI(TAG, "Boot complete — entering idle.");
    /* app_main may return; the scheduler keeps running. */
}
