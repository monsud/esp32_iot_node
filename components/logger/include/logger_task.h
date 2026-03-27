/**
 * @file    logger_task.h
 * @brief   Persistent logger — writes log_entry_t to SPIFFS.
 *
 * All tasks post log entries via logger_post() which is
 * ISR-safe (uses xQueueSendFromISR internally when needed).
 * The logger task drains the queue and writes to flash.
 */

#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  FreeRTOS task entry point for the logger.
 */
void logger_task(void *pvParameters);

/**
 * @brief  Non-blocking helper used by all other tasks to post a log entry.
 *         Safe to call before the logger task starts (entries buffered in queue).
 *
 * @param level   Severity level.
 * @param source  Short string identifying the caller (module name).
 * @param message Human-readable message (truncated to LOG_MSG_MAX_LEN).
 */
void logger_post(log_level_t level, const char *source, const char *message);

#ifdef __cplusplus
}
#endif
