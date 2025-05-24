#include "config_defaults.h"
#include <string.h>

void config_init_display_defaults(ConfigDisplay* display) {
    display->width = DEFAULT_DISPLAY_WIDTH;
    display->height = DEFAULT_DISPLAY_HEIGHT;
    display->fullscreen = DEFAULT_DISPLAY_FULLSCREEN;
    display->vsync = DEFAULT_DISPLAY_VSYNC;
    strncpy(display->backend, DEFAULT_DISPLAY_BACKEND, CONFIG_MAX_STRING - 1);
    display->backend[CONFIG_MAX_STRING - 1] = '\0';
}

void config_init_input_defaults(ConfigInput* input) {
    strncpy(input->source, DEFAULT_INPUT_SOURCE, CONFIG_MAX_STRING - 1);
    input->source[CONFIG_MAX_STRING - 1] = '\0';
    
    strncpy(input->device_path, DEFAULT_INPUT_DEVICE_PATH, CONFIG_MAX_PATH - 1);
    input->device_path[CONFIG_MAX_PATH - 1] = '\0';
    
    input->mouse_emulation = DEFAULT_INPUT_MOUSE_EMULATION;
    input->auto_detect_devices = DEFAULT_INPUT_AUTO_DETECT;
}

void config_init_api_defaults(ConfigApi* api) {
    // Set defaults
    api->default_timeout_ms = DEFAULT_API_TIMEOUT_MS;
    api->default_retry_count = DEFAULT_API_RETRY_COUNT;
    api->default_retry_delay_ms = DEFAULT_API_RETRY_DELAY_MS;
    api->default_verify_ssl = DEFAULT_API_VERIFY_SSL;
    strncpy(api->default_user_agent, DEFAULT_API_USER_AGENT, CONFIG_MAX_STRING - 1);
    api->default_user_agent[CONFIG_MAX_STRING - 1] = '\0';
    
    // Initialize with no services (will be allocated and populated from config)
    api->services = NULL;
    api->num_services = 0;
    api->max_services = 0;
}

static void config_init_colors_defaults(ColorScheme* colors) {
    strncpy(colors->background, DEFAULT_COLOR_BACKGROUND, CONFIG_MAX_COLOR - 1);
    colors->background[CONFIG_MAX_COLOR - 1] = '\0';
    
    strncpy(colors->primary, DEFAULT_COLOR_PRIMARY, CONFIG_MAX_COLOR - 1);
    colors->primary[CONFIG_MAX_COLOR - 1] = '\0';
    
    strncpy(colors->secondary, DEFAULT_COLOR_SECONDARY, CONFIG_MAX_COLOR - 1);
    colors->secondary[CONFIG_MAX_COLOR - 1] = '\0';
    
    strncpy(colors->accent, DEFAULT_COLOR_ACCENT, CONFIG_MAX_COLOR - 1);
    colors->accent[CONFIG_MAX_COLOR - 1] = '\0';
    
    strncpy(colors->error, DEFAULT_COLOR_ERROR, CONFIG_MAX_COLOR - 1);
    colors->error[CONFIG_MAX_COLOR - 1] = '\0';
    
    strncpy(colors->warning, DEFAULT_COLOR_WARNING, CONFIG_MAX_COLOR - 1);
    colors->warning[CONFIG_MAX_COLOR - 1] = '\0';
    
    strncpy(colors->success, DEFAULT_COLOR_SUCCESS, CONFIG_MAX_COLOR - 1);
    colors->success[CONFIG_MAX_COLOR - 1] = '\0';
}

static void config_init_fonts_defaults(FontConfig* fonts) {
    fonts->regular_size = DEFAULT_FONT_REGULAR_SIZE;
    fonts->large_size = DEFAULT_FONT_LARGE_SIZE;
    fonts->small_size = DEFAULT_FONT_SMALL_SIZE;
    
    strncpy(fonts->family, DEFAULT_FONT_FAMILY, CONFIG_MAX_STRING - 1);
    fonts->family[CONFIG_MAX_STRING - 1] = '\0';
}

static void config_init_animations_defaults(AnimationConfig* animations) {
    animations->enabled = DEFAULT_ANIMATION_ENABLED;
    animations->page_transition_ms = DEFAULT_ANIMATION_PAGE_TRANSITION_MS;
    animations->scroll_friction = DEFAULT_ANIMATION_SCROLL_FRICTION;
    animations->button_press_scale = DEFAULT_ANIMATION_BUTTON_PRESS_SCALE;
}

static void config_init_layout_defaults(LayoutConfig* layout) {
    layout->button_padding = DEFAULT_LAYOUT_BUTTON_PADDING;
    layout->header_height = DEFAULT_LAYOUT_HEADER_HEIGHT;
    layout->margin = DEFAULT_LAYOUT_MARGIN;
    layout->scroll_threshold = DEFAULT_LAYOUT_SCROLL_THRESHOLD;
    layout->swipe_threshold = DEFAULT_LAYOUT_SWIPE_THRESHOLD;
}

void config_init_ui_defaults(ConfigUI* ui) {
    config_init_colors_defaults(&ui->colors);
    config_init_fonts_defaults(&ui->fonts);
    config_init_animations_defaults(&ui->animations);
    config_init_layout_defaults(&ui->layout);
}

void config_init_logging_defaults(ConfigLogging* logging) {
    strncpy(logging->level, DEFAULT_LOG_LEVEL, CONFIG_MAX_STRING - 1);
    logging->level[CONFIG_MAX_STRING - 1] = '\0';
    
    strncpy(logging->file, DEFAULT_LOG_FILE, CONFIG_MAX_PATH - 1);
    logging->file[CONFIG_MAX_PATH - 1] = '\0';
    
    logging->max_size = DEFAULT_LOG_MAX_SIZE;
    logging->max_files = DEFAULT_LOG_MAX_FILES;
    logging->console = DEFAULT_LOG_CONSOLE;
}

void config_init_system_defaults(ConfigSystem* system) {
    system->startup_page = DEFAULT_SYSTEM_STARTUP_PAGE;
    system->debug_overlay = DEFAULT_SYSTEM_DEBUG_OVERLAY;
    system->allow_exit = DEFAULT_SYSTEM_ALLOW_EXIT;
    system->idle_timeout = DEFAULT_SYSTEM_IDLE_TIMEOUT;
    system->config_check_interval = DEFAULT_SYSTEM_CONFIG_CHECK_INTERVAL;
}

void config_init_defaults(Config* config) {
    // Clear the structure
    memset(config, 0, sizeof(Config));
    
    // Set version
    strncpy(config->version, "1.0", CONFIG_MAX_STRING - 1);
    config->version[CONFIG_MAX_STRING - 1] = '\0';
    
    // Initialize all sections with defaults
    config_init_display_defaults(&config->display);
    config_init_input_defaults(&config->input);
    config_init_api_defaults(&config->api);
    config_init_ui_defaults(&config->ui);
    config_init_logging_defaults(&config->logging);
    config_init_system_defaults(&config->system);
}