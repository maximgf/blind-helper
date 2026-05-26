# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

file(GLOB_RECURSE app_sources "${CMAKE_SOURCE_DIR}/src/*.c")

if(NOT CONFIG_BT_NIMBLE_ENABLED)
    list(FILTER app_sources EXCLUDE REGEX ".*/drivers/ble_message/.*")
endif()

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/src"
    REQUIRES bt driver esp_rom esp_timer freertos nvs_flash
)
