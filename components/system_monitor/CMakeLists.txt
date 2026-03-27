idf_component_register(
    SRCS "src/system_monitor.c"
    INCLUDE_DIRS "include" "${CMAKE_SOURCE_DIR}/main"
    REQUIRES freertos log esp_system logger
)