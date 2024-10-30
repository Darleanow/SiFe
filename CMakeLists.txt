cmake_minimum_required(VERSION 3.28)
project(SiFe C)

set(CMAKE_C_STANDARD 23)

set(MICROUI_DIR "${CMAKE_SOURCE_DIR}/dependencies/MicroUI")
set(SDL_DIR "${CMAKE_SOURCE_DIR}/dependencies/SDL-release-2.30.8")

add_subdirectory(${SDL_DIR})

set(MICROUI_SOURCES "${MICROUI_DIR}/microui.c" renderer.c)

add_executable(SiFe main.c ${MICROUI_SOURCES})

target_link_libraries(SiFe PRIVATE SDL2::SDL2-static OpenGL32)
target_include_directories(SiFe PRIVATE  renderer.h)
target_include_directories(SiFe PRIVATE ${MICROUI_DIR} ${SDL_DIR}/include ${CMAKE_SOURCE_DIR})
