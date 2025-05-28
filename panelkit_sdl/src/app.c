#include "core/sdl_includes.h"
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>
#include <unistd.h>

// Core includes
#include "core/logger.h"
#include "core/build_info.h"
#include "core/error.h"
#include "core/error_logger.h"
#include "display/display_backend.h"
#include "input/input_handler.h"
#include "input/input_debug.h"

// API modules
#include "api/api_manager.h"

// Configuration system
#include "config/config_manager.h"

// UI system
#include "ui/ui_init.h"
#include "ui/widget.h"
#include "ui/widget_manager.h"
#include "events/event_system.h"
#include "state/state_store.h"

// Embedded font data
#include "embedded_font.h"

// Default screen dimensions
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

// Actual runtime dimensions (will be set from display backend)
static int actual_width = SCREEN_WIDTH;
static int actual_height = SCREEN_HEIGHT;

// Button dimensions (calculated dynamically)
#define BUTTON_WIDTH (actual_width / 2)
#define BUTTON_HEIGHT ((actual_height * 2) / 3)
#define BUTTON_PADDING 20

// Global variables
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
DisplayBackend* display_backend = NULL;  // Display abstraction
InputHandler* input_handler = NULL;     // Input abstraction
ConfigManager* config_manager = NULL;   // Configuration system
TTF_Font* font = NULL;
TTF_Font* large_font = NULL;
// TODO: Remove - was only used by removed render_api_data function
TTF_Font* small_font = NULL; // Smaller font for API data
bool quit = false;

// Background color (will be set from config)
SDL_Color bg_color = {33, 33, 33, 255}; // Default dark gray

// Time display
// Time display is now handled by UI system (g_show_time in ui_init.c)

// Page 1 specific
// TODO: Remove when layout/theme system is implemented - now handled by widget state
char page1_text[256] = "Welcome to Page 1! Swipe right to see buttons.";
int page1_text_color = 0; // 0=white, 1=red, 2=green, 3=blue, etc.
SDL_Color text_colors[] = {
    {255, 255, 255, 255}, // White
    {255, 100, 100, 255}, // Red
    {100, 255, 100, 255}, // Green
    {100, 100, 255, 255}, // Blue
    {255, 255, 100, 255}, // Yellow
    {255, 100, 255, 255}, // Purple
    {100, 255, 255, 255}, // Cyan
};

// API manager
ApiManager* api_manager = NULL;
// TODO: Remove - user data now stored in state store via widget integration
UserData current_user_data = {0};

// UI system components
WidgetManager* widget_manager = NULL;
EventSystem* event_system = NULL;
StateStore* state_store = NULL;

// Debug info
bool show_debug = true;
Uint32 frame_count = 0;
Uint32 fps_timer = 0;
Uint32 fps = 0;

// Function prototypes
static void on_system_page_transition(const char* event_name, const void* data, size_t data_size, void* context);
static void on_system_api_refresh(const char* event_name, const void* data, size_t data_size, void* context);

// API callback functions
void on_api_data_received(const UserData* data, void* context);
void on_api_error(ApiError error, const char* message, void* context);
void on_api_state_changed(ApiState state, void* context);

// Simple text rendering for debug overlay
void draw_text_left(const char* text, int x, int y, SDL_Color color) {
    if (!text || !font || !renderer) return;
    
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    if (!surface) return;
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        SDL_Rect dst = {x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &dst);
        SDL_DestroyTexture(texture);
    }
    
    SDL_FreeSurface(surface);
}


// Configuration helper functions
static DisplayBackendType backend_type_from_string(const char* str) {
    if (strcmp(str, "sdl") == 0) return DISPLAY_BACKEND_SDL;
    if (strcmp(str, "sdl_drm") == 0) return DISPLAY_BACKEND_SDL_DRM;
    return DISPLAY_BACKEND_AUTO;
}

static InputSourceType input_source_from_string(const char* str) {
    if (strcmp(str, "sdl_native") == 0) return INPUT_SOURCE_SDL_NATIVE;
    if (strcmp(str, "evdev") == 0) return INPUT_SOURCE_LINUX_EVDEV;
    return INPUT_SOURCE_SDL_NATIVE;  // Default
}

static SDL_Color parse_color(const char* hex) {
    SDL_Color color = {0, 0, 0, 255};
    if (hex && hex[0] == '#' && strlen(hex) == 7) {
        unsigned int rgb;
        sscanf(hex + 1, "%06x", &rgb);
        color.r = (rgb >> 16) & 0xFF;
        color.g = (rgb >> 8) & 0xFF;
        color.b = rgb & 0xFF;
    }
    return color;
}

int main(int argc, char* argv[]) {
    // Initialize logging first
    const char* config_paths[] = {
        "/etc/panelkit/zlog.conf",      // Production location
        "config/zlog.conf",              // Development location
        NULL
    };
    
    const char* config_file = NULL;
    for (int i = 0; config_paths[i] != NULL; i++) {
        if (access(config_paths[i], R_OK) == 0) {
            config_file = config_paths[i];
            break;
        }
    }
    
    if (!logger_init(config_file, "panelkit")) {
        fprintf(stderr, "Warning: Using fallback logging\n");
    }
    
    // Initialize error logger
    ErrorLogConfig error_log_config = error_logger_default_config();
    error_log_config.log_directory = "logs";
    error_log_config.log_to_console = false;  // Main logger already logs to console
    if (!error_logger_init(&error_log_config)) {
        log_warn("Failed to initialize error logger - errors will not be logged to file");
    }
    
    // Log startup
    log_info("=== PanelKit Starting ===");
    log_system_info();
    
    // Log comprehensive build info
    build_log_info();
    
    // Log command line arguments
    log_debug("Command line: %d arguments", argc);
    for (int i = 0; i < argc; i++) {
        log_debug("  argv[%d] = %s", i, argv[i]);
    }
    
    // Initialize configuration system
    ConfigManagerOptions config_options = {0};
    config_manager = config_manager_create(&config_options);
    if (!config_manager) {
        log_error("Failed to create configuration manager");
        logger_shutdown();
        return 1;
    }
    
    // Load configuration from all sources
    if (!config_manager_load(config_manager)) {
        log_error("Failed to load configuration");
        config_manager_destroy(config_manager);
        logger_shutdown();
        return 1;
    }
    
    // Get configuration
    const Config* config = config_manager_get_config(config_manager);
    config_log_summary(config_manager);
    
    // Parse command line for overrides and special options
    DisplayBackendType backend_type = backend_type_from_string(config->display.backend);
    int display_width = config->display.width;
    int display_height = config->display.height;
    bool portrait_mode = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            config_manager_load_file(config_manager, argv[i + 1], CONFIG_SOURCE_CLI);
            log_info("Loaded configuration from: %s", argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--config-override") == 0 && i + 1 < argc) {
            char* arg_copy = strdup(argv[i + 1]);
            char* key = strtok(arg_copy, "=");
            char* value = strtok(NULL, "=");
            if (key && value) {
                config_manager_apply_override(config_manager, key, value);
                log_info("Configuration override: %s = %s", key, value);
            }
            free(arg_copy);
            i++;
        } else if (strcmp(argv[i], "--validate-config") == 0 && i + 1 < argc) {
            ValidationResult result = config_validate_file(argv[i + 1]);
            if (result.valid) {
                printf("Configuration file is valid: %s\n", argv[i + 1]);
            } else {
                printf("Configuration error at line %d: %s\n", 
                       result.error_line, result.error_message);
            }
            config_manager_destroy(config_manager);
            logger_shutdown();
            return result.valid ? 0 : 1;
        } else if (strcmp(argv[i], "--generate-config") == 0 && i + 1 < argc) {
            if (config_generate_default(argv[i + 1], true)) {
                printf("Generated configuration file: %s\n", argv[i + 1]);
            } else {
                printf("Failed to generate configuration file\n");
            }
            config_manager_destroy(config_manager);
            logger_shutdown();
            return config_generate_default(argv[i + 1], true) ? 0 : 1;
        } else if (strcmp(argv[i], "--display-backend") == 0 && i + 1 < argc) {
            const char* backend = argv[i + 1];
            backend_type = backend_type_from_string(backend);
            log_info("Display backend override: %s", backend);
            i++;
        } else if (strcmp(argv[i], "--portrait") == 0) {
            portrait_mode = true;
            log_info("Portrait mode requested");
        } else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            display_width = atoi(argv[i + 1]);
            log_info("Display width override: %d", display_width);
            i++;
        } else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            display_height = atoi(argv[i + 1]);
            log_info("Display height override: %d", display_height);
            i++;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("PanelKit - Touch UI Application\n");
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --config <file>                  Load configuration from file\n");
            printf("  --config-override <key=value>    Override configuration value\n");
            printf("  --validate-config <file>         Validate configuration file\n");
            printf("  --generate-config <file>         Generate default configuration\n");
            printf("  --display-backend <sdl|sdl_drm>  Select display backend\n");
            printf("  --portrait                       Use portrait mode (swap width/height)\n");
            printf("  --width <pixels>                 Set display width\n");
            printf("  --height <pixels>                Set display height\n");
            printf("  --help, -h                       Show this help\n");
            config_manager_destroy(config_manager);
            logger_shutdown();
            return 0;
        }
    }
    
    // Apply portrait mode if requested
    if (portrait_mode) {
        int temp = display_width;
        display_width = display_height;
        display_height = temp;
        log_info("Portrait mode: %dx%d", display_width, display_height);
    }
    
    // Initialize display backend
    log_state_change("Display", "NONE", "INITIALIZING");
    DisplayConfig display_config = {
        .width = display_width,
        .height = display_height,
        .title = "PanelKit",
        .backend_type = backend_type,
        .fullscreen = config->display.fullscreen,
        .vsync = config->display.vsync
    };
    
    display_backend = display_backend_create(&display_config);
    if (!display_backend) {
        log_error("Failed to create display backend");
        logger_shutdown();
        return 1;
    }
    
    // Get the actual window and renderer
    window = display_backend->window;
    renderer = display_backend->renderer;
    
    if (!window || !renderer) {
        log_error("Failed to get window or renderer from display backend");
        display_backend_destroy(display_backend);
        logger_shutdown();
        return 1;
    }
    
    log_state_change("Display", "INITIALIZING", "READY");
    
    // Update actual dimensions from display
    SDL_GetWindowSize(window, &actual_width, &actual_height);
    log_info("Display initialized: %dx%d", actual_width, actual_height);
    
    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        log_error("SDL_ttf initialization failed: %s", TTF_GetError());
        display_backend_destroy(display_backend);
        logger_shutdown();
        return 1;
    }
    
    // Load embedded font from memory
    SDL_RWops* font_rw = SDL_RWFromConstMem(embedded_font_data, embedded_font_size);
    if (!font_rw) {
        log_error("Failed to create RWops from embedded font data");
        TTF_Quit();
        display_backend_destroy(display_backend);
        logger_shutdown();
        return 1;
    }
    
    // Load font at different sizes - don't close RWops, TTF_OpenFontRW handles it
    font = TTF_OpenFontRW(font_rw, 1, config->ui.fonts.regular_size);  // The '1' means SDL will free the RWops
    if (!font) {
        log_error("Failed to load font from embedded data: %s", TTF_GetError());
        TTF_Quit();
        display_backend_destroy(display_backend);
        logger_shutdown();
        return 1;
    }
    
    // Load same font at larger size
    SDL_RWops* large_font_rw = SDL_RWFromConstMem(embedded_font_data, embedded_font_size);
    large_font = TTF_OpenFontRW(large_font_rw, 1, config->ui.fonts.large_size);
    if (!large_font) {
        log_error("Failed to load large font: %s", TTF_GetError());
        TTF_CloseFont(font);
        TTF_Quit();
        display_backend_destroy(display_backend);
        logger_shutdown();
        return 1;
    }
    
    // Load same font at smaller size
    SDL_RWops* small_font_rw = SDL_RWFromConstMem(embedded_font_data, embedded_font_size);
    small_font = TTF_OpenFontRW(small_font_rw, 1, config->ui.fonts.small_size);
    if (!small_font) {
        log_error("Failed to load small font: %s", TTF_GetError());
        TTF_CloseFont(font);
        TTF_CloseFont(large_font);
        TTF_Quit();
        display_backend_destroy(display_backend);
        logger_shutdown();
        return 1;
    }
    
    log_info("Fonts loaded successfully from embedded data");
    
    // Initialize input handler
    log_state_change("Input", "NONE", "INITIALIZING");
    InputConfig input_config = {
        .source_type = input_source_from_string(config->input.source),
        .device_path = strcmp(config->input.device_path, "auto") == 0 ? NULL : config->input.device_path,
        .auto_detect_devices = config->input.auto_detect_devices,
        .enable_mouse_emulation = config->input.mouse_emulation
    };
    
    // If using SDL+DRM backend, switch to evdev input source
    if (display_backend->type == DISPLAY_BACKEND_SDL_DRM) {
        log_info("SDL+DRM backend detected, switching to evdev input source");
        input_config.source_type = INPUT_SOURCE_LINUX_EVDEV;
    }
    
    input_handler = input_handler_create(&input_config);
    if (!input_handler) {
        log_error("Failed to create input handler");
        TTF_CloseFont(font);
        TTF_CloseFont(large_font);
        TTF_CloseFont(small_font);
        TTF_Quit();
        display_backend_destroy(display_backend);
        logger_shutdown();
        return 1;
    }
    
    if (!input_handler_start(input_handler)) {
        log_error("Failed to start input handler");
        input_handler_destroy(input_handler);
        TTF_CloseFont(font);
        TTF_CloseFont(large_font);
        TTF_CloseFont(small_font);
        TTF_Quit();
        display_backend_destroy(display_backend);
        logger_shutdown();
        return 1;
    }
    
    log_state_change("Input", "INITIALIZING", "READY");
    
    // Log input debug info
    input_debug_log_state(input_handler);
    
    // If we're using evdev, log device capabilities
    if (input_handler->config.source_type == INPUT_SOURCE_LINUX_EVDEV &&
        input_handler->config.device_path) {
        input_debug_log_device_caps(input_handler->config.device_path);
    }
    
    // Apply UI configuration
    bg_color = parse_color(config->ui.colors.background);
    
    // Initialize API manager
    // TODO: Update to use new multi-service API structure
    ApiManagerConfig api_config = api_manager_default_config();
    api_config.timeout_seconds = config->api.default_timeout_ms / 1000;  // Convert ms to seconds
    api_config.retry_count = config->api.default_retry_count;
    api_config.retry_delay_ms = config->api.default_retry_delay_ms;
    // For now, keep using the default base_url until API manager is updated
    
    api_manager = api_manager_create(&api_config);
    if (!api_manager) {
        log_error("Failed to create API manager");
        TTF_CloseFont(font);
        TTF_CloseFont(large_font);
        TTF_CloseFont(small_font);
        TTF_Quit();
        display_backend_destroy(display_backend);
        logger_shutdown();
        return 1;
    }
    
    // Set up API callbacks
    api_manager_set_data_callback(api_manager, on_api_data_received, NULL);
    api_manager_set_error_callback(api_manager, on_api_error, NULL);
    api_manager_set_state_callback(api_manager, on_api_state_changed, NULL);
    
    // Initialize core UI systems
    event_system = event_system_create();
    if (!event_system) {
        log_error("Failed to create event system");
        TTF_CloseFont(font);
        TTF_CloseFont(large_font);
        TTF_CloseFont(small_font);
        TTF_Quit();
        display_backend_destroy(display_backend);
        logger_shutdown();
        return 1;
    }
    
    state_store = state_store_create();
    if (!state_store) {
        log_error("Failed to create state store");
        event_system_destroy(event_system);
        TTF_CloseFont(font);
        TTF_CloseFont(large_font);
        TTF_CloseFont(small_font);
        TTF_Quit();
        display_backend_destroy(display_backend);
        logger_shutdown();
        return 1;
    }
    
    widget_manager = widget_manager_create(renderer, event_system, state_store);
    if (!widget_manager) {
        log_error("Failed to create widget manager");
        state_store_destroy(state_store);
        event_system_destroy(event_system);
        TTF_CloseFont(font);
        TTF_CloseFont(large_font);
        TTF_CloseFont(small_font);
        TTF_Quit();
        display_backend_destroy(display_backend);
        logger_shutdown();
        return 1;
    }
    
    // Initialize UI with hardcoded layout
    PkError ui_err = ui_init(widget_manager, event_system, state_store,
                            actual_width, actual_height,
                            font, large_font, small_font);
    if (ui_err != PK_OK) {
        log_error("Failed to initialize UI: %s", pk_error_string(ui_err));
        widget_manager_destroy(widget_manager);
        state_store_destroy(state_store);
        event_system_destroy(event_system);
        TTF_CloseFont(font);
        TTF_CloseFont(large_font);
        TTF_CloseFont(small_font);
        TTF_Quit();
        display_backend_destroy(display_backend);
        logger_shutdown();
        return 1;
    }
    
    // Subscribe to system events
    event_subscribe(event_system, "system.page_transition", on_system_page_transition, NULL);
    event_subscribe(event_system, "system.api_refresh", on_system_api_refresh, NULL);
    log_info("UI system initialized");
    
    // Force initial API fetch immediately
    api_manager_fetch_user_async(api_manager);
    
    // Main loop
    Uint32 last_time = SDL_GetTicks();
    while (!quit) {
        Uint32 current_time = SDL_GetTicks();
        
        // Note: SDL event processing happens below
        
        // Process SDL events for unified input handling
        SDL_Event e;
        int event_count = 0;
        while (SDL_PollEvent(&e)) {
            event_count++;
            if (e.type == SDL_QUIT) {
                // Set quit through state store
                bool quit_flag = true;
                state_store_set(state_store, "app", "quit", &quit_flag, sizeof(bool));
                quit = true;
            }
            
            // Forward SDL events to widget manager
            if (widget_manager) {
                widget_manager_handle_event(widget_manager, &e);
            }
        }
        
        // Update API manager (still needed for data)
        api_manager_update(api_manager, current_time);
        
        // Get background color from state store
        SDL_Color widget_bg_color = {33, 33, 33, 255}; // Default
        if (state_store) {
            size_t size;
            time_t timestamp;
            SDL_Color* stored_color = (SDL_Color*)state_store_get(state_store, 
                                                                  "app", "bg_color", &size, &timestamp);
            if (stored_color && size == sizeof(SDL_Color)) {
                widget_bg_color = *stored_color;
                free(stored_color);
            }
            
            // Check quit flag from state store
            bool* stored_quit = (bool*)state_store_get(state_store, 
                                                      "app", "quit", &size, &timestamp);
            if (stored_quit && size == sizeof(bool)) {
                quit = *stored_quit;
                free(stored_quit);
            }
        }
        
        // Clear screen with background color
        SDL_SetRenderDrawColor(renderer, widget_bg_color.r, widget_bg_color.g, 
                             widget_bg_color.b, widget_bg_color.a);
        SDL_RenderClear(renderer);
        
        // Widget-based rendering
        Widget* page_manager = ui_get_page_manager();
        if (page_manager) {
            // Update UI rendering state
            ui_update_rendering();
            
            // Update widgets
            widget_manager_update(widget_manager);
            
            // Render widgets
            widget_manager_render(widget_manager);
        }
        
        // Draw debug overlay if enabled
        if (show_debug) {
            // Get current page from state store
            int current_page = 0;
            size_t size;
            time_t timestamp;
            int* stored_page = (int*)state_store_get(state_store, "app", "current_page", &size, &timestamp);
            if (stored_page) {
                current_page = *stored_page;
                free(stored_page);
            }
            
            char debug_line1[256];
            snprintf(debug_line1, sizeof(debug_line1), "Page: %d | FPS: %d", 
                    current_page + 1, fps);
            draw_text_left(debug_line1, 10, actual_height - 55, (SDL_Color){255, 255, 255, 128});
        }
        
        
        // Calculate FPS
        frame_count++;
        if (current_time - fps_timer >= 1000) {
            fps = frame_count;
            frame_count = 0;
            fps_timer = current_time;
            
            // FPS is now just displayed in debug overlay
        }
        
        // Update screen
        SDL_RenderPresent(renderer);
        display_backend_present(display_backend);
        
        // Frame limiting
        Uint32 frame_time = SDL_GetTicks() - current_time;
        if (frame_time < 16) {
            SDL_Delay(16 - frame_time);
        }
    }
    
    // Cleanup
    log_state_change("Application", "RUNNING", "SHUTTING_DOWN");
    
    if (api_manager) {
        api_manager_destroy(api_manager);
    }
    if (input_handler) {
        input_handler_destroy(input_handler);
    }
    if (config_manager) {
        config_manager_destroy(config_manager);
    }
    // Cleanup UI systems
    ui_cleanup();
    if (widget_manager) {
        widget_manager_destroy(widget_manager);
    }
    if (state_store) {
        state_store_destroy(state_store);
    }
    if (event_system) {
        event_system_destroy(event_system);
    }
    TTF_CloseFont(font);
    TTF_CloseFont(large_font);
    TTF_CloseFont(small_font);
    TTF_Quit();
    display_backend_destroy(display_backend);
    
    log_info("=== PanelKit Shutdown Complete ===");
    error_logger_shutdown();
    logger_shutdown();
    
    return 0;
}



// API callback functions
void on_api_data_received(const UserData* data, void* context) {
    (void)context; // Unused
    
    if (data) {
        current_user_data = *data;
        log_info("API data received: %s", current_user_data.name);
        
        // Store API data in state store and publish event
        if (state_store) {
            state_store_set(state_store, "api", "user_data", data, sizeof(*data));
            
            // Publish event for widgets
            if (event_system) {
                event_publish(event_system, "api.user_data_updated", data, sizeof(*data));
            }
        }
    }
}

void on_api_error(ApiError error, const char* message, void* context) {
    (void)context; // Unused
    
    log_error("API error: %s - %s", api_error_string(error), message ? message : "Unknown error");
}

void on_api_state_changed(ApiState state, void* context) {
    (void)context; // Unused
    
    log_debug("API state changed: %s", api_state_string(state));
}

// Handle system page transition events from widget integration
static void on_system_page_transition(const char* event_name, const void* data, size_t data_size, void* context) {
    (void)event_name; (void)context;
    
    if (!data || data_size < sizeof(struct { int from_page; int to_page; })) {
        return;
    }
    
    struct {
        int from_page;
        int to_page;
    } *page_event = (void*)data;
    
    log_debug("System page transition event: %d -> %d", page_event->from_page, page_event->to_page);
}

// Event handler for API refresh requests
static void on_system_api_refresh(const char* event_name, const void* data, size_t data_size, void* context) {
    (void)event_name; (void)data; (void)data_size; (void)context;
    
    log_debug("System API refresh event received");
    
    // Trigger API refresh
    // TODO(Phase7): Remove global api_manager when old system is fully retired
    if (api_manager) {
        api_manager_fetch_user_async(api_manager);
        log_info("API refresh triggered via system event");
    }
}

