/**
 * @file    logger_task.c
 * @brief   Persistent logger using SPIFFS.
 *
 * Design decisions:
 *  - All writes are protected by g_log_file_mutex (shared with any
 *    future reader task e.g. OTA log dump).
 *  - Log rotation: when the file exceeds LOG_FILE_MAX_SIZE_BYTES,
 *    it is renamed to system.log.bak and a fresh file is started.
 *  - logger_post() is callable from any task before this task starts;
 *    entries pile up in g_log_queue and are drained once SPIFFS is ready.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_spiffs.h"

#include "config.h"
#include "types.h"
#include "rtos_handles.h"
#include "logger_task.h"

static const char *TAG = "LOGGER";

#define LOG_BACKUP_PATH "/spiffs/system.log.bak"

static const char *level_to_str(log_level_t lvl)
{
    switch (lvl) {
    case LOG_LEVEL_DEBUG:   return "DBG";
    case LOG_LEVEL_INFO:    return "INF";
    case LOG_LEVEL_WARNING: return "WRN";
    case LOG_LEVEL_ERROR:   return "ERR";
    default:                return "???";
    }
}

/* ─────────────────────────────────────────────────────────────────────────────
 * SPIFFS initialisation
 * ─────────────────────────────────────────────────────────────────────────── */
static esp_err_t spiffs_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path              = "/spiffs",
        .partition_label        = NULL,   /* use first spiffs partition */
        .max_files              = 5,
        .format_if_mount_failed = true,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0, used = 0;
    esp_spiffs_info(NULL, &total, &used);
    ESP_LOGI(TAG, "SPIFFS: %u B total, %u B used.", (unsigned)total, (unsigned)used);
    return ESP_OK;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Log rotation
 * ─────────────────────────────────────────────────────────────────────────── */
static void rotate_if_needed(void)
{
    FILE *f = fopen(LOG_FILE_PATH, "r");
    if (!f) return;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);

    if (size >= (long)LOG_FILE_MAX_SIZE_BYTES) {
        ESP_LOGW(TAG, "Log file full (%ld B) — rotating.", size);
        remove(LOG_BACKUP_PATH);
        rename(LOG_FILE_PATH, LOG_BACKUP_PATH);
    }
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Write one entry to SPIFFS
 * ─────────────────────────────────────────────────────────────────────────── */
static void write_entry(const log_entry_t *entry)
{
    if (xSemaphoreTake(g_log_file_mutex, pdMS_TO_TICKS(500)) != pdTRUE) {
        ESP_LOGW(TAG, "Could not acquire log mutex — entry dropped.");
        return;
    }

    rotate_if_needed();

    FILE *f = fopen(LOG_FILE_PATH, "a");
    if (f) {
        fprintf(f, "[%10lu][%s][%-12s] %s\n",
                (unsigned long)entry->timestamp_s,
                level_to_str(entry->level),
                entry->source,
                entry->message);
        fclose(f);
    } else {
        ESP_LOGE(TAG, "Cannot open log file.");
    }

    xSemaphoreGive(g_log_file_mutex);
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Public API — post from any task
 * ─────────────────────────────────────────────────────────────────────────── */
void logger_post(log_level_t level, const char *source, const char *message)
{
    if (g_log_queue == NULL) return;   /* called before queue created */

    log_entry_t entry = {
        .level       = level,
        .timestamp_s = (uint32_t)time(NULL),
    };
    strncpy(entry.source,  source  ? source  : "?", sizeof(entry.source)  - 1);
    strncpy(entry.message, message ? message : "?", sizeof(entry.message) - 1);
    entry.source[sizeof(entry.source)   - 1] = '\0';
    entry.message[sizeof(entry.message) - 1] = '\0';

    /* Non-blocking — drop if queue full to avoid blocking callers */
    xQueueSend(g_log_queue, &entry, 0);
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Task entry point
 * ─────────────────────────────────────────────────────────────────────────── */
void logger_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Task started.");

    bool spiffs_ok = (spiffs_init() == ESP_OK);

    if (!spiffs_ok) {
        ESP_LOGE(TAG, "SPIFFS unavailable — logging to UART only.");
    }

    log_entry_t entry;

    for (;;) {
        /* Block indefinitely until there is something to log */
        if (xQueueReceive(g_log_queue, &entry, portMAX_DELAY) == pdTRUE) {
            /* Always mirror to UART */
            ESP_LOGI(TAG, "[%s][%s] %s",
                     level_to_str(entry.level),
                     entry.source,
                     entry.message);

            /* Persist to SPIFFS only if available */
            if (spiffs_ok) {
                write_entry(&entry);
            }
        }
    }
}