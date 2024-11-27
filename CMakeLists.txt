cmake_minimum_required(VERSION 3.28)
project(SiFe C)

set(CMAKE_C_STANDARD 23)

# Define directory paths
set(MICROUI_DIR "${CMAKE_SOURCE_DIR}/dependencies/MicroUI")
set(SDL_DIR "${CMAKE_SOURCE_DIR}/dependencies/SDL-release-2.30.8")

# Add SDL2
add_subdirectory(${SDL_DIR})

# Define source files
set(MICROUI_SOURCES "${MICROUI_DIR}/microui.c" renderer.c)
set(PROJECT_SOURCES
        ui_state.c
        menu.c
        logger.c
)

# Create executable
add_executable(SiFe
        main.c
        ${MICROUI_SOURCES}
        ${PROJECT_SOURCES}
)

# Link libraries
target_link_libraries(SiFe PRIVATE SDL2::SDL2-static OpenGL32)

# Include directories
target_include_directories(SiFe PRIVATE
        ${MICROUI_DIR}
        ${SDL_DIR}/include
        ${CMAKE_SOURCE_DIR}
)