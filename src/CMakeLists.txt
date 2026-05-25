# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

file(GLOB_RECURSE app_sources "${CMAKE_SOURCE_DIR}/src/*.c")

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/src"
    REQUIRES driver esp_rom esp_timer freertos
)
