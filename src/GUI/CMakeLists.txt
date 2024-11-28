add_library(GUI
        UiState.c
)

target_link_libraries(GUI PRIVATE Components)

target_include_directories(GUI PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
)
