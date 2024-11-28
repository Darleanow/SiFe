add_library(Core INTERFACE)

add_subdirectory(GUI/Components)
add_subdirectory(GUI)
add_subdirectory(Systems)
add_subdirectory(Config)

target_link_libraries(Core INTERFACE Config Systems GUI MicroUI ${COMMON_LIBRARIES})

target_include_directories(Core INTERFACE ${COMMON_INCLUDE_DIRS})
