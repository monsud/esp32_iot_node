idf_component_register(
    SRCS "src/sensor_task.c" "src/sensor_driver.c"
    INCLUDE_DIRS "include" "${CMAKE_SOURCE_DIR}/main"
    REQUIRES freertos log esp_system logger
)