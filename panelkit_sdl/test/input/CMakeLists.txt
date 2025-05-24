# Input handler test program

add_executable(test_input_handler test_input_handler.c)

# Link against input handler and dependencies
target_link_libraries(test_input_handler
    panelkit_input
    panelkit_core
    ${SDL2_LIBRARIES}
    pthread
)

# Include directories
target_include_directories(test_input_handler PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIRS}
)