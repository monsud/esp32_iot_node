idf_component_register(
    SRCS "src/logger_task.c"
    INCLUDE_DIRS "include" "${CMAKE_SOURCE_DIR}/main"
    REQUIRES freertos log esp_system spiffs
)