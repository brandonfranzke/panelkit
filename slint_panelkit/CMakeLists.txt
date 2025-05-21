cmake_minimum_required(VERSION 3.10)
project(panelkit LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Source files
set(SOURCES
    src/main.c
)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Find packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# Add SDL2
include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})

# Add LVGL (clone it during configuration)
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/lvgl")
    execute_process(
        COMMAND git clone -b v9.2.2 --depth 1 https://github.com/lvgl/lvgl.git
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()

# Add LVGL as a subdirectory
add_subdirectory(${CMAKE_SOURCE_DIR}/lvgl EXCLUDE_FROM_ALL)

# Link LVGL
target_link_libraries(${PROJECT_NAME} lvgl)

# Enable SDL2 driver in LVGL
target_compile_definitions(${PROJECT_NAME} PRIVATE LV_USE_SDL=1)

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)