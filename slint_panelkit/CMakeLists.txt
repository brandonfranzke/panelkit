cmake_minimum_required(VERSION 3.10)
project(panelkit LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(BUILD_SDL "Build with SDL2 renderer" ON)
option(BUILD_FBDEV "Build with Linux framebuffer" OFF)

# Source files
set(SOURCES
    src/main.c
    src/ui_elements.c
    src/platform.c
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Find LVGL
include(cmake/lvgl.cmake)

# Platform specific settings
if(BUILD_SDL)
    # SDL2 for development/simulation
    include(cmake/sdl2.cmake)
    target_compile_definitions(${PROJECT_NAME} PRIVATE LV_USE_SDL=1)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2)
elseif(BUILD_FBDEV)
    # Linux framebuffer for embedded targets
    target_compile_definitions(${PROJECT_NAME} PRIVATE LV_USE_FBDEV=1)
endif()

# Link LVGL
target_link_libraries(${PROJECT_NAME} PRIVATE lvgl)

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)