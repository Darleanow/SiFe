add_library(Config
        Renderer.c
)

target_include_directories(Config PUBLIC
        ${COMMON_INCLUDE_DIRS}  # Include SDL2 and other common directories
)

target_link_libraries(Config PUBLIC
        ${COMMON_LIBRARIES}  # Link SDL2 library here
)
