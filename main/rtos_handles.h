/**
 * @file    rtos_handles.h
 * @brief   Global RTOS object handles (queues, event groups, mutexes).
 *
 * Declared extern here, defined once in main.c.
 * Components include this header to access shared IPC primitives.
 */

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─────────────────────────────────────────────────────────────────────────────
 * Queues
 * ─────────────────────────────────────────────────────────────────────────── */
extern QueueHandle_t g_sensor_queue;      /**< sensor_data_t  : Sensor → Processing */
extern QueueHandle_t g_mqtt_queue;        /**< mqtt_message_t : Processing → MQTT    */
extern QueueHandle_t g_log_queue;         /**< log_entry_t    : All tasks → Logger   */

/* ─────────────────────────────────────────────────────────────────────────────
 * Event Groups  (bit definitions)
 * ─────────────────────────────────────────────────────────────────────────── */
extern EventGroupHandle_t g_system_events;

#define EVT_WIFI_CONNECTED     BIT0
#define EVT_MQTT_CONNECTED     BIT1
#define EVT_SENSOR_READY       BIT2
#define EVT_SHUTDOWN_REQUEST   BIT7

/* ─────────────────────────────────────────────────────────────────────────────
 * Mutexes / Semaphores
 * ─────────────────────────────────────────────────────────────────────────── */
extern SemaphoreHandle_t g_log_file_mutex;   /**< Protects SPIFFS file access */

#ifdef __cplusplus
}
#endif
