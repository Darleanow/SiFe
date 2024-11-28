add_library(Systems
        Logger.c
)

target_include_directories(Systems PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)