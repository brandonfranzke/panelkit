/**
 * @file config_schema.h
 * @brief Configuration data structures
 * 
 * Defines the complete configuration schema for the application.
 */

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
} ConfigDisplay;

// Input configuration
typedef struct {
    char source[CONFIG_MAX_STRING];    // "auto", "sdl_native", "evdev"
    char device_path[CONFIG_MAX_PATH]; // Device path or "auto"
    bool mouse_emulation;
    bool auto_detect_devices;
} ConfigInput;

// Individual endpoint within an API
typedef struct {
    char id[CONFIG_MAX_STRING];             // Unique identifier for this endpoint
    char name[CONFIG_MAX_STRING];           // Human-readable name
    char path[CONFIG_MAX_PATH];             // Endpoint path (appended to API base_path)
    char method[16];                        // HTTP method: "GET", "POST", "PUT", "DELETE"
    char required_params[CONFIG_MAX_STRING]; // JSON object string of required params
    char optional_params[CONFIG_MAX_STRING]; // JSON object string of optional params
    bool auto_refresh;                      // Enable auto-refresh for this endpoint
    int refresh_interval_ms;                // Auto-refresh interval (overrides API default)
    
    // Parser function for this endpoint's responses
    // Note: Parser implementations are registered separately in code
    // This field is populated at runtime, not from config
    void* parser_func;  // Will be cast to appropriate parser function type
} ApiEndpointConfig;

// API service configuration
typedef struct {
    char id[CONFIG_MAX_STRING];             // Unique identifier for this API service
    char name[CONFIG_MAX_STRING];           // Human-readable name
    char host[CONFIG_MAX_STRING];           // Hostname or IP
    int port;                               // Port number (0 = use default for protocol)
    char protocol[16];                      // "http", "https", "ws", "wss"
    char base_path[CONFIG_MAX_PATH];        // Base path for all endpoints
    char bearer_token[CONFIG_MAX_STRING];   // Bearer token for auth
    char api_key[CONFIG_MAX_STRING];        // API key for auth
    char username[CONFIG_MAX_STRING];       // Basic auth username
    char password[CONFIG_MAX_STRING];       // Basic auth password
    int timeout_ms;                         // Request timeout in milliseconds
    int retry_count;                        // Number of retries on failure
    int retry_delay_ms;                     // Delay between retries
    bool verify_ssl;                        // Verify SSL certificates
    char user_agent[CONFIG_MAX_STRING];     // Custom User-Agent header
    
    // Custom headers for non-standard API requirements
    char headers[CONFIG_MAX_STRING * 4];    // JSON object string of custom headers
    
    // Meta/data section for service-specific configuration
    // This can contain API-specific keys, tokens, or other data
    // that the service handler can interpret as needed
    char meta[CONFIG_MAX_STRING * 4];       // JSON object string of metadata
    
    // Endpoints for this API service
    ApiEndpointConfig* endpoints;
    size_t num_endpoints;
    size_t max_endpoints;
} ApiServiceConfig;

// API configuration
typedef struct {
    // Default settings for all APIs
    int default_timeout_ms;
    int default_retry_count;
    int default_retry_delay_ms;
    bool default_verify_ssl;
    char default_user_agent[CONFIG_MAX_STRING];
    
    // Defined API services
    ApiServiceConfig* services;
    size_t num_services;
    size_t max_services;
} ConfigApi;

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
} ConfigUI;

// Logging configuration
typedef struct {
    char level[CONFIG_MAX_STRING];     // "debug", "info", "warn", "error"
    char file[CONFIG_MAX_PATH];
    uint64_t max_size;                 // bytes
    int max_files;
    bool console;
} ConfigLogging;

// System configuration
typedef struct {
    int startup_page;
    bool debug_overlay;
    bool allow_exit;
    int idle_timeout;  // seconds, 0 = disabled
    char config_check_interval;  // seconds, 0 = disabled
} ConfigSystem;

// Main configuration structure
struct Config {
    // Metadata
    char version[CONFIG_MAX_STRING];
    char loaded_from[CONFIG_MAX_PATH];
    
    // Configuration sections
    ConfigDisplay display;
    ConfigInput input;
    ConfigApi api;
    ConfigUI ui;
    ConfigLogging logging;
    ConfigSystem system;
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