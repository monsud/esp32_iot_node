# ESP32 Industrial IoT Node

![ESP32](https://img.shields.io/badge/ESP32--S3-ESP--IDF%20v5.4.3-blue)
![FreeRTOS](https://img.shields.io/badge/FreeRTOS-multicore-green)
![MQTT](https://img.shields.io/badge/MQTT-HiveMQ-orange)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

## Overview

A production-grade embedded firmware for a WiFi-connected environmental
monitoring node. The system continuously acquires sensor data, processes
and serialises it to JSON, and publishes readings to an MQTT broker.
All events and errors are persisted to flash (SPIFFS) and survive reboots.

---

## Architecture

```
┌──────────────────────────────────────────────────────────┐
│                    APPLICATION LAYER                     │
│  sensor_task  →  processing_task  →  mqtt_task          │
│       ↓                  ↓               ↓               │
│  logger_task         system_monitor                      │
└───────────────────────────┬──────────────────────────────┘
                            │  FreeRTOS Queues / Event Groups
┌───────────────────────────▼──────────────────────────────┐
│                   MIDDLEWARE / RTOS LAYER                 │
│   g_sensor_queue  g_mqtt_queue  g_log_queue              │
│   g_system_events (EVT_WIFI_CONNECTED, EVT_MQTT_CONNECTED│
│   g_log_file_mutex                                       │
└───────────────────────────┬──────────────────────────────┘
                            │
┌───────────────────────────▼──────────────────────────────┐
│                     DRIVERS LAYER                        │
│   sensor_driver  (simulated BME280 / real I2C)           │
│   wifi_manager   (esp_wifi + esp_event)                  │
│   esp-mqtt       (TCP/IP broker client)                  │
│   SPIFFS         (non-volatile log storage)              │
└──────────────────────────────────────────────────────────┘
```

### Data flow

```
sensor_driver ──► sensor_task ──[g_sensor_queue]──► processing_task
                                                          │
                                                  [g_mqtt_queue]
                                                          │
                                                     mqtt_task ──► Broker
```

---

## Project Structure

```
esp32_iot_node/
├── CMakeLists.txt
├── partitions.csv
├── sdkconfig.defaults
│
├── main/
│   ├── CMakeLists.txt
│   ├── main.c               
│   ├── config.h              
│   ├── types.h               
│   ├── rtos_handles.h        
│   ├── wifi_manager.{h,c}    
│   └── processing_task.{h,c} 
│
└── components/
    ├── sensor/
    │   ├── include/
    │   │   ├── sensor_task.h
    │   │   └── sensor_driver.h   
    │   └── src/
    │       ├── sensor_task.c    
    │       └── sensor_driver.c   
    │
    ├── mqtt_client/
    │   ├── include/mqtt_task.h
    │   └── src/mqtt_task.c      
    │
    ├── logger/
    │   ├── include/logger_task.h
    │   └── src/logger_task.c     
    │
    └── system_monitor/
        ├── include/system_monitor.h
        └── src/system_monitor.c  
```

---

## RTOS Task Map

| Task            | Priority | Stack  | Period          | Purpose                        |
|-----------------|----------|--------|-----------------|--------------------------------|
| system_monitor  | MAX-1    | 2 KB   | 5 s             | Heap / connectivity health     |
| mqtt_task       | MAX-2    | 8 KB   | event-driven    | Broker publish + reconnect     |
| sensor_task     | MAX-3    | 4 KB   | 2 s             | Sensor acquisition             |
| processing_task | MAX-4    | 4 KB   | event-driven    | JSON serialisation             |
| logger_task     | 2        | 4 KB   | event-driven    | SPIFFS log write               |

All inter-task communication is via **FreeRTOS queues** — tasks never
share memory directly.

---

## Inter-Task Communication

```
                   ┌────────────────────────────────┐
                   │        g_system_events          │
                   │  EVT_WIFI_CONNECTED   (BIT0)    │
                   │  EVT_MQTT_CONNECTED   (BIT1)    │
                   │  EVT_SENSOR_READY     (BIT2)    │
                   │  EVT_SHUTDOWN_REQUEST (BIT7)    │
                   └────────────────────────────────┘

  sensor_task ─[sensor_data_t]──► g_sensor_queue ──► processing_task
  processing_task ─[mqtt_msg_t]──► g_mqtt_queue  ──► mqtt_task
  ANY task ─[log_entry_t]────────► g_log_queue   ──► logger_task
```

---

## Concurrency & Synchronisation

| Primitive          | Protects                         |
|--------------------|----------------------------------|
| `g_log_file_mutex` | SPIFFS file descriptor           |
| FreeRTOS queues    | All cross-task data exchange     |
| `g_system_events`  | Global connectivity state        |

Deadlock prevention: mutexes are **never held while waiting on a queue**.  
Priority inversion: `g_log_file_mutex` uses the FreeRTOS priority-inheritance
mutex (`xSemaphoreCreateMutex`).

---

## Configuration

All tunables live in **`main/config.h`** — never hard-coded in `.c` files.

| Symbol                    | Default              | Description             |
|---------------------------|----------------------|-------------------------|
| `WIFI_SSID`               | sdkconfig            | SSID                    |
| `WIFI_MAX_RETRY`          | 5                    | Reconnect attempts      |
| `MQTT_BROKER_URL`         | sdkconfig            | Broker URI              |
| `MQTT_PUBLISH_RETRY`      | 3                    | Publish attempts        |
| `SENSOR_POLL_INTERVAL_MS` | 2000                 | Acquisition period      |
| `LOG_FILE_MAX_SIZE_BYTES` | 256 KB               | Rotation threshold      |
| `MONITOR_CHECK_INTERVAL_MS`| 5000                | Health-check period     |
| `WATCHDOG_TIMEOUT_S`      | 30                   | HW watchdog timeout     |

---

## Build & Flash

```bash
# Prerequisites: ESP-IDF >= 5.1 sourced
. $IDF_PATH/export.sh

# Configure credentials
idf.py menuconfig
# → Component config → WiFi / MQTT settings

# Build
idf.py build

# Flash + monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

---

## Replacing the Simulated Sensor

The sensor HAL is isolated in `components/sensor/src/sensor_driver.c`.

To use a real BME280:
1. Add `idf_component_manager` dependency: `espressif/bme280`
2. Replace `sensor_driver_init()` with I2C master init + BME280 probe.
3. Replace `sensor_driver_read()` with `bme280_read_forced_mode()`.
4. All other tasks remain unchanged.

---

## Error Handling Strategy

| Condition               | Response                                      |
|-------------------------|-----------------------------------------------|
| Sensor read failure     | Log error, skip sample, continue             |
| Sensor queue full       | Drop oldest, log warning                     |
| WiFi disconnect         | Auto-reconnect with delay, log event         |
| MQTT disconnect         | esp-mqtt auto-reconnects; publish retries    |
| Publish failure         | Retry × 3 with exponential back-off          |
| Low heap                | Log error, publish alert to broker           |
| Task watchdog           | ESP-IDF resets the chip                      |
| SPIFFS full             | Log rotation (rename → .bak)                 |

---

## Log Format (SPIFFS `/spiffs/system.log`)

```
[  1700000000][INF][WIFI        ] WiFi connected.
[  1700000001][INF][SENSOR      ] Task started.
[  1700000003][WRN][PROCESSING  ] No sensor data received.
[  1700000010][ERR][MQTT        ] Publish failed after max retries.
```

---

## Requirements Traceability

| Requirement                        | Implementation                             |
|------------------------------------|--------------------------------------------|
| FR-1  Periodic sensor acquisition  | `sensor_task.c` + `vTaskDelayUntil`        |
| FR-2  Data formatting              | `processing_task.c` + `snprintf` JSON      |
| FR-3  MQTT transmission            | `mqtt_task.c` + esp-mqtt component         |
| FR-4  Network auto-reconnect       | `wifi_manager.c` + `mqtt_task.c`           |
| FR-5  Non-volatile logging         | `logger_task.c` + SPIFFS                   |
| FR-6  Continuous operation         | Watchdog + recovery in all tasks           |
| FR-7  Fault detection & recovery   | `system_monitor.c` + retry logic           |
| FR-8  UART debug output            | ESP-IDF `ESP_LOG*` macros throughout       |
| FR-9  Multi-task RTOS              | FreeRTOS, 5 concurrent tasks               |
| FR-10 Delivery reliability         | `publish_with_retry()` + QoS 1             |

---

## License

MIT — see [LICENSE](LICENSE)
