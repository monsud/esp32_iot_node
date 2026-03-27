cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/components/sensor
    ${CMAKE_CURRENT_SOURCE_DIR}/components/mqtt_client
    ${CMAKE_CURRENT_SOURCE_DIR}/components/logger
    ${CMAKE_CURRENT_SOURCE_DIR}/components/system_monitor
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_iot_node)
