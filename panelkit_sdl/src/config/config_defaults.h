/**
 * @file config_defaults.h
 * @brief Default configuration values
 * 
 * Defines compile-time defaults for all configuration parameters.
 */

#ifndef CONFIG_DEFAULTS_H
#define CONFIG_DEFAULTS_H

#include "config_schema.h"

// Default configuration values
// These are used when values are not specified in configuration files

// Display defaults
#define DEFAULT_DISPLAY_WIDTH 640
#define DEFAULT_DISPLAY_HEIGHT 480
#define DEFAULT_DISPLAY_FULLSCREEN false
#define DEFAULT_DISPLAY_VSYNC true
#define DEFAULT_DISPLAY_BACKEND "auto"

// Input defaults
#define DEFAULT_INPUT_SOURCE "auto"
#define DEFAULT_INPUT_DEVICE_PATH "auto"
#define DEFAULT_INPUT_MOUSE_EMULATION false
#define DEFAULT_INPUT_AUTO_DETECT true

// API defaults
#define DEFAULT_API_TIMEOUT_MS 10000
#define DEFAULT_API_RETRY_COUNT 3
#define DEFAULT_API_RETRY_DELAY_MS 1000
#define DEFAULT_API_VERIFY_SSL true
#define DEFAULT_API_USER_AGENT "PanelKit/1.0"

// UI Color defaults (Material Design inspired)
#define DEFAULT_COLOR_BACKGROUND "#212121"
#define DEFAULT_COLOR_PRIMARY "#ffffff"
#define DEFAULT_COLOR_SECONDARY "#cccccc"
#define DEFAULT_COLOR_ACCENT "#00ff00"
#define DEFAULT_COLOR_ERROR "#ff0000"
#define DEFAULT_COLOR_WARNING "#ff9800"
#define DEFAULT_COLOR_SUCCESS "#4caf50"

// UI Font defaults
#define DEFAULT_FONT_REGULAR_SIZE 18
#define DEFAULT_FONT_LARGE_SIZE 32
#define DEFAULT_FONT_SMALL_SIZE 14
#define DEFAULT_FONT_FAMILY "default"

// UI Animation defaults
#define DEFAULT_ANIMATION_ENABLED true
#define DEFAULT_ANIMATION_PAGE_TRANSITION_MS 300
#define DEFAULT_ANIMATION_SCROLL_FRICTION 0.95f
#define DEFAULT_ANIMATION_BUTTON_PRESS_SCALE 0.95f

// UI Layout defaults
#define DEFAULT_LAYOUT_BUTTON_PADDING 20
#define DEFAULT_LAYOUT_HEADER_HEIGHT 60
#define DEFAULT_LAYOUT_MARGIN 10
#define DEFAULT_LAYOUT_SCROLL_THRESHOLD 10
#define DEFAULT_LAYOUT_SWIPE_THRESHOLD 50

// Logging defaults
#define DEFAULT_LOG_LEVEL "info"
#define DEFAULT_LOG_FILE "/var/log/panelkit/panelkit.log"
#define DEFAULT_LOG_MAX_SIZE (10 * 1024 * 1024)  // 10MB
#define DEFAULT_LOG_MAX_FILES 5
#define DEFAULT_LOG_CONSOLE true

// System defaults
#define DEFAULT_SYSTEM_STARTUP_PAGE 0
#define DEFAULT_SYSTEM_DEBUG_OVERLAY false
#define DEFAULT_SYSTEM_ALLOW_EXIT true
#define DEFAULT_SYSTEM_IDLE_TIMEOUT 0
#define DEFAULT_SYSTEM_CONFIG_CHECK_INTERVAL 0

// Initialize a Config structure with all defaults
void config_init_defaults(Config* config);

// Initialize individual sections with defaults
void config_init_display_defaults(ConfigDisplay* display);
void config_init_input_defaults(ConfigInput* input);
void config_init_api_defaults(ConfigApi* api);
void config_init_ui_defaults(ConfigUI* ui);
void config_init_logging_defaults(ConfigLogging* logging);
void config_init_system_defaults(ConfigSystem* system);

#endif // CONFIG_DEFAULTS_H