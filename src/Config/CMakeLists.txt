add_library(Config
        Renderer.c
)

target_include_directories(Config PUBLIC
        ${COMMON_INCLUDE_DIRS}
)

target_link_libraries(Config PUBLIC
        ${COMMON_LIBRARIES}
)
