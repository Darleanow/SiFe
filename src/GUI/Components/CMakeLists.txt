add_library(Components
        Menu.c
)

target_include_directories(Components PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${COMMON_INCLUDE_DIRS}
)

target_link_libraries(Components PUBLIC
        ${COMMON_LIBRARIES}
)
