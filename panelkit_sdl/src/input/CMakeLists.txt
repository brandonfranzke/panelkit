# Input subsystem library

set(INPUT_SOURCES
    input_handler.c
    input_source_sdl.c
    input_source_mock.c
    input_debug.c
)

# Only include evdev source on Linux
if(UNIX AND NOT APPLE)
    list(APPEND INPUT_SOURCES input_source_evdev.c)
endif()

# Create input library
add_library(panelkit_input STATIC ${INPUT_SOURCES})

# Link against required libraries
target_link_libraries(panelkit_input
    ${SDL2_LIBRARIES}
    pthread
)

# Include directories
target_include_directories(panelkit_input PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# SDL include directories (handle macOS homebrew case)
if(APPLE AND NOT EMBEDDED_BUILD)
    target_include_directories(panelkit_input PUBLIC
        ${SDL2_INCLUDE_DIRS}/SDL2
        ${SDL2_TTF_INCLUDE_DIRS}/SDL2
    )
else()
    target_include_directories(panelkit_input PUBLIC
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
    )
endif()

# Add to parent target list
set(PANELKIT_INPUT_LIB panelkit_input PARENT_SCOPE)