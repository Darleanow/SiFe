cmake_minimum_required(VERSION 3.28)
project(SiFe C)

set(CMAKE_C_STANDARD 23)

# Define directories
set(MICROUI_DIR "${CMAKE_SOURCE_DIR}/dependencies/MicroUI")
set(SDL_DIR "${CMAKE_SOURCE_DIR}/dependencies/SDL-release-2.30.8")

# Add SDL2 as a subdirectory
add_subdirectory(${SDL_DIR})

# Shared include directories and link libraries
set(COMMON_INCLUDE_DIRS
        ${MICROUI_DIR}
        ${SDL_DIR}/include
        ${CMAKE_SOURCE_DIR}
)

message(STATUS "COMMON_INCLUDE_DIRS: ${COMMON_INCLUDE_DIRS}")


set(COMMON_LIBRARIES
        SDL2::SDL2-static
)

if(WIN32)
    list(APPEND COMMON_LIBRARIES OpenGL32)
elseif(APPLE)
    list(APPEND COMMON_LIBRARIES "-framework OpenGL")
else()
    list(APPEND COMMON_LIBRARIES GL)
endif()

# Define MicroUI as its own library
add_library(MicroUI STATIC
        ${MICROUI_DIR}/microui.c
)

target_include_directories(MicroUI PUBLIC ${MICROUI_DIR})

# Add executable
add_executable(SiFe main.c)

# Add src subdirectory
add_subdirectory(src)

# Link libraries to SiFe
target_link_libraries(SiFe PRIVATE Core MicroUI ${COMMON_LIBRARIES})
target_include_directories(SiFe PRIVATE ${COMMON_INCLUDE_DIRS})
