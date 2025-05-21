# LVGL library setup
include(FetchContent)

# Download LVGL from GitHub
FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG v8.3.11
)

# Make LVGL available
FetchContent_MakeAvailable(lvgl)

# Set LVGL configuration
target_compile_definitions(lvgl PRIVATE
    LV_CONF_INCLUDE_SIMPLE=1
    LV_CONF_PATH="${PROJECT_SOURCE_DIR}/include/lv_conf.h"
)

# Include LVGL headers
target_include_directories(${PROJECT_NAME} PRIVATE
    ${lvgl_SOURCE_DIR}
    ${lvgl_SOURCE_DIR}/src
)