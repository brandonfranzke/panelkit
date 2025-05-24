#ifndef CONFIG_SCHEMA_H
#define CONFIG_SCHEMA_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>  // for size_t

// Maximum string lengths for configuration values
#define CONFIG_MAX_PATH 256
#define CONFIG_MAX_URL 512
#define CONFIG_MAX_STRING 128
#define CONFIG_MAX_COLOR 8  // #RRGGBB

// Forward declaration
typedef struct Config Config;

// Display configuration
typedef struct {
    int width;
    int height;
    bool fullscreen;
    bool vsync;
    char backend[CONFIG_MAX_STRING];  // "auto", "sdl", "sdl_drm"
} DisplayConfig;

// Input configuration
typedef struct {
    char source[CONFIG_MAX_STRING];    // "auto", "sdl_native", "evdev"
    char device_path[CONFIG_MAX_PATH]; // Device path or "auto"
    bool mouse_emulation;
    bool auto_detect_devices;
} InputConfig;

// API endpoint configuration
typedef struct {
    char name[CONFIG_MAX_STRING];
    char url[CONFIG_MAX_URL];
    int refresh_interval;  // seconds
} ApiEndpoint;

// API configuration
typedef struct {
    char base_url[CONFIG_MAX_URL];
    int timeout;  // seconds
    bool auto_refresh;
    int refresh_interval;  // seconds
    
    // Custom endpoints (future expansion)
    ApiEndpoint* custom_endpoints;
    size_t num_custom_endpoints;
} ApiConfig;

// Color scheme
typedef struct {
    char background[CONFIG_MAX_COLOR];
    char primary[CONFIG_MAX_COLOR];
    char secondary[CONFIG_MAX_COLOR];
    char accent[CONFIG_MAX_COLOR];
    char error[CONFIG_MAX_COLOR];
    char warning[CONFIG_MAX_COLOR];
    char success[CONFIG_MAX_COLOR];
} ColorScheme;

// Font configuration
typedef struct {
    int regular_size;
    int large_size;
    int small_size;
    char family[CONFIG_MAX_STRING];  // "default" or future custom font
} FontConfig;

// Animation configuration
typedef struct {
    bool enabled;
    int page_transition_ms;
    float scroll_friction;
    float button_press_scale;
} AnimationConfig;

// Layout configuration
typedef struct {
    int button_padding;
    int header_height;
    int margin;
    int scroll_threshold;
    int swipe_threshold;
} LayoutConfig;

// UI configuration
typedef struct {
    ColorScheme colors;
    FontConfig fonts;
    AnimationConfig animations;
    LayoutConfig layout;
} UIConfig;

// Logging configuration
typedef struct {
    char level[CONFIG_MAX_STRING];     // "debug", "info", "warn", "error"
    char file[CONFIG_MAX_PATH];
    uint64_t max_size;                 // bytes
    int max_files;
    bool console;
} LogConfig;

// System configuration
typedef struct {
    int startup_page;
    bool debug_overlay;
    bool allow_exit;
    int idle_timeout;  // seconds, 0 = disabled
    char config_check_interval;  // seconds, 0 = disabled
} SystemConfig;

// Main configuration structure
struct Config {
    // Metadata
    char version[CONFIG_MAX_STRING];
    char loaded_from[CONFIG_MAX_PATH];
    
    // Configuration sections
    DisplayConfig display;
    InputConfig input;
    ApiConfig api;
    UIConfig ui;
    LogConfig logging;
    SystemConfig system;
};

// Required field indicators (for validation)
typedef struct {
    // Display
    bool display_width;
    bool display_height;
    
    // API
    bool api_base_url;
    
    // Logging
    bool logging_file;
    
    // Add more as needed
} RequiredFields;

#endif // CONFIG_SCHEMA_H