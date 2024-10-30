cmake_minimum_required(VERSION 3.28)
project(SiFe C)

set(CMAKE_C_STANDARD 23)

set(MICROUI_DIR "${CMAKE_SOURCE_DIR}/dependencies/MicroUI")

add_executable(SiFe main.c)

target_include_directories(SiFe PRIVATE ${MICROUI_DIR})
