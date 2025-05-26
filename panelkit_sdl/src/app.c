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
#include "display/display_backend.h"
#include "input/input_handler.h"
#include "input/input_debug.h"

// UI modules
#include "ui/gestures.h"
#include "ui/pages.h"
#include "ui/rendering.h"

// API modules
#include "api/api_manager.h"

// Configuration system
#include "config/config_manager.h"

// Widget integration layer (runs parallel to existing UI)
#include "ui/widget_integration.h"

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
TTF_Font* small_font = NULL; // Smaller font for API data
bool quit = false;

// Background color (will be set from config)
SDL_Color bg_color = {33, 33, 33, 255}; // Default dark gray

// Time display
bool show_time = true; // Whether to show time on the third button

// Page 1 specific
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
UserData current_user_data = {0};

// Widget integration layer (runs parallel to existing system)
WidgetIntegration* widget_integration = NULL;

// Debug info
bool show_debug = true;
Uint32 frame_count = 0;
Uint32 fps_timer = 0;
Uint32 fps = 0;

// Function prototypes
void initialize_pages();
void render_page(int page_index, float offset_x);
int get_button_at_position(int x, int y, int scroll_offset);
void handle_click(int button_index);
void handle_drag(int delta_x, int delta_y, bool is_horizontal);
void handle_swipe(float offset, bool is_complete);

// API callback functions
void on_api_data_received(const UserData* data, void* context);
void on_api_error(ApiError error, const char* message, void* context);
void on_api_state_changed(ApiState state, void* context);

// Old API functions (to be replaced)
void render_api_data(SDL_Renderer* renderer, int x, int y);

// Callback for button hit testing
int button_hit_test(int x, int y, int page_index) {
    Page* page = pages_get(page_index);
    if (!page) return -1;
    
    return get_button_at_position(x, y, page->scroll_position);
}

// Callback for gesture clicks
void on_gesture_click(int button_index) {
    handle_click(button_index);
}

// Callback for gesture drags
void on_gesture_drag(int delta_x, int delta_y, bool is_horizontal) {
    if (!is_horizontal) {
        // Vertical drag - update scroll
        int current_page = pages_get_current();
        pages_update_scroll(current_page, delta_y);
    }
}

// Callback for gesture swipes
void on_gesture_swipe(float offset, bool is_complete) {
    pages_handle_swipe(offset, is_complete);
}

// Callback for page rendering
void on_page_render(int page_index, float offset_x) {
    render_page(page_index, offset_x);
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
    
    // Initialize modules
    gestures_init(on_gesture_click, on_gesture_drag, on_gesture_swipe, button_hit_test);
    pages_init(2, on_page_render);  // 2 pages
    pages_set_dimensions(actual_width, actual_height);
    rendering_init(renderer, font, large_font, small_font);
    rendering_set_dimensions(actual_width, actual_height);
    
    // Apply UI configuration
    bg_color = parse_color(config->ui.colors.background);
    
    // Initialize pages
    initialize_pages();
    
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
    
    // Initialize widget integration layer (runs parallel to existing UI)
    widget_integration = widget_integration_create(renderer);
    if (!widget_integration) {
        log_warn("Widget integration layer failed to initialize - continuing without it");
    } else {
        widget_integration_set_dimensions(widget_integration, actual_width, actual_height);
        // Start with minimal integration - just state tracking for now
        log_info("Widget integration layer initialized (background mode)");
    }
    
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
                quit = true;
            }
            // Handle SDL touch events
            else if (e.type == SDL_FINGERDOWN) {
                int touch_x = (int)(e.tfinger.x * actual_width);
                int touch_y = (int)(e.tfinger.y * actual_height);
                handle_touch_down(touch_x, touch_y, "touch");
                // Mirror to widget integration layer
                if (widget_integration) {
                    widget_integration_mirror_touch_event(widget_integration, touch_x, touch_y, true);
                }
            }
            else if (e.type == SDL_FINGERUP) {
                int touch_x = (int)(e.tfinger.x * actual_width);
                int touch_y = (int)(e.tfinger.y * actual_height);
                handle_touch_up(touch_x, touch_y, "touch");
            }
            else if (e.type == SDL_FINGERMOTION) {
                int touch_x = (int)(e.tfinger.x * actual_width);
                int touch_y = (int)(e.tfinger.y * actual_height);
                handle_touch_motion(touch_x, touch_y, "touch");
            }
            // Handle mouse events as touch
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                handle_touch_down(e.button.x, e.button.y, "mouse");
                // Mirror to widget integration layer
                if (widget_integration) {
                    widget_integration_mirror_touch_event(widget_integration, e.button.x, e.button.y, true);
                }
            }
            else if (e.type == SDL_MOUSEBUTTONUP) {
                handle_touch_up(e.button.x, e.button.y, "mouse");
            }
            else if (e.type == SDL_MOUSEMOTION && (e.motion.state & SDL_BUTTON_LMASK)) {
                handle_touch_motion(e.motion.x, e.motion.y, "mouse");
            }
        }
        
        // Update page transitions
        pages_update_transition();
        gestures_set_current_page(pages_get_current());
        
        // Update API manager
        api_manager_update(api_manager, current_time);
        
        // Clear screen
        SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        SDL_RenderClear(renderer);
        
        // Render current page(s)
        int current_page = pages_get_current();
        int target_page = pages_get_target_page();
        float transition_offset = pages_get_transition_offset();
        
        if (target_page != -1) {
            // Render both pages during transition
            float current_offset = transition_offset * actual_width;
            float target_offset = current_offset + (target_page > current_page ? actual_width : -actual_width);
            
            render_page(current_page, current_offset);
            render_page(target_page, target_offset);
        } else {
            // Render single page
            render_page(current_page, 0);
        }
        
        // Render page indicators if visible
        if (pages_should_show_indicators()) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, pages_get_indicator_alpha());
            render_page_indicators(current_page, pages_get_total(), transition_offset);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
        
        // Draw debug overlay if enabled (at bottom of screen - two lines)
        if (show_debug) {
            const char* gesture_name = 
                gestures_get_state() == GESTURE_NONE ? "NONE" :
                gestures_get_state() == GESTURE_POTENTIAL ? "POTENTIAL" :
                gestures_get_state() == GESTURE_CLICK ? "CLICK" :
                gestures_get_state() == GESTURE_DRAG_VERT ? "DRAG_VERT" :
                gestures_get_state() == GESTURE_DRAG_HORZ ? "DRAG_HORZ" : "HOLD";
                
            const char* transition_state = 
                pages_get_transition_state() == TRANSITION_NONE ? "NONE" :
                pages_get_transition_state() == TRANSITION_DRAGGING ? "DRAGGING" : "ANIMATING";
            
            // First line: Page | FPS | Gesture
            char debug_line1[256];
            snprintf(debug_line1, sizeof(debug_line1), "Page: %d | FPS: %d | Gesture: %s", 
                    current_page + 1, fps, gesture_name);
            draw_text_left(debug_line1, 10, actual_height - 55, (SDL_Color){255, 255, 255, 128});
            
            // Second line: Button | Transition | Scroll
            Page* current_page_data = pages_get(current_page);
            char debug_line2[256];
            snprintf(debug_line2, sizeof(debug_line2), "Button: %d | Transition: %s (%.2f) | Scroll: %d/%d", 
                    gestures_get_button(), transition_state, transition_offset,
                    current_page_data ? current_page_data->scroll_position : 0,
                    current_page_data ? current_page_data->max_scroll : 0);
            draw_text_left(debug_line2, 10, actual_height - 30, (SDL_Color){200, 200, 200, 128});
        }
        
        // Calculate FPS
        frame_count++;
        if (current_time - fps_timer >= 1000) {
            fps = frame_count;
            frame_count = 0;
            fps_timer = current_time;
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
    if (widget_integration) {
        widget_integration_destroy(widget_integration);
    }
    TTF_CloseFont(font);
    TTF_CloseFont(large_font);
    TTF_CloseFont(small_font);
    TTF_Quit();
    display_backend_destroy(display_backend);
    
    log_info("=== PanelKit Shutdown Complete ===");
    logger_shutdown();
    
    return 0;
}

// Initialize all pages - exact same implementation as original
void initialize_pages() {
    Page* page0 = pages_get(0);
    Page* page1 = pages_get(1);
    
    // Page 1 - Text page
    page0->scroll_position = 0;
    page0->max_scroll = 0; // No scrolling needed
    page0->title = "Text Page";
    page0->button_count = 1;
    strcpy(page0->button_texts[0], "Change Text Color");
    page0->button_colors[0] = (SDL_Color){52, 152, 219, 255}; // Blue
    
    // Page 2 - Buttons page
    page1->scroll_position = 0;
    page1->title = "Buttons Page";
    page1->button_count = 9;
    
    // Button texts
    strcpy(page1->button_texts[0], "Blue");
    strcpy(page1->button_texts[1], "Random");
    strcpy(page1->button_texts[2], "Time"); // Will be updated with time
    strcpy(page1->button_texts[3], "Go to Page 1");
    strcpy(page1->button_texts[4], "Refresh User");
    strcpy(page1->button_texts[5], "Exit App");
    strcpy(page1->button_texts[6], "Button 7");
    strcpy(page1->button_texts[7], "Button 8");
    strcpy(page1->button_texts[8], "Button 9");
    
    // Button colors
    page1->button_colors[0] = (SDL_Color){41, 128, 185, 255};  // Blue
    page1->button_colors[1] = (SDL_Color){142, 68, 173, 255};  // Purple
    page1->button_colors[2] = (SDL_Color){142, 142, 142, 255}; // Gray
    page1->button_colors[3] = (SDL_Color){231, 76, 60, 255};   // Red
    page1->button_colors[4] = (SDL_Color){39, 174, 96, 255};  // Green (for refresh)
    page1->button_colors[5] = (SDL_Color){192, 57, 43, 255};   // Dark Red (for exit)
    page1->button_colors[6] = (SDL_Color){52, 152, 219, 255};  // Blue
    page1->button_colors[7] = (SDL_Color){241, 196, 15, 255};  // Yellow
    page1->button_colors[8] = (SDL_Color){230, 126, 34, 255};  // Orange
    
    // Calculate max scroll for page 2 (add space for title)
    int total_content_height = 90 + BUTTON_PADDING + page1->button_count * (BUTTON_HEIGHT + BUTTON_PADDING);
    page1->max_scroll = total_content_height - actual_height;
    if (page1->max_scroll < 0) page1->max_scroll = 0;
}

// Render a page - exact same implementation as original
void render_page(int page_index, float offset_x) {
    Page* page = pages_get(page_index);
    if (!page) return;
    
    // Save current clip rect
    SDL_Rect old_clip;
    SDL_RenderGetClipRect(renderer, &old_clip);
    
    // Set clip rect for this page (with offset)
    SDL_Rect page_clip = {
        (int)offset_x,
        0,
        actual_width,
        actual_height
    };
    SDL_RenderSetClipRect(renderer, &page_clip);
    
    if (page_index == 0) {
        // Page 1: Text display
        // Draw title
        draw_large_text(page->title, (int)(actual_width/2 + offset_x), 60, (SDL_Color){255, 255, 255, 255});
        
        // Draw single button
        int button_x = (actual_width - BUTTON_WIDTH) / 2;
        int button_y = 200;
        int button_height = BUTTON_HEIGHT / 2;
        
        // Determine button state based on current gesture
        ButtonState button_state = BUTTON_NORMAL;
        if (gestures_get_state() != GESTURE_NONE && 
            gestures_get_button() == 0 && 
            gestures_get_page() == page_index &&
            pages_get_current() == page_index) {
            button_state = BUTTON_PRESSED;
        }
        
        render_button((int)(button_x + offset_x), button_y, BUTTON_WIDTH, button_height,
                     page->button_texts[0], page->button_colors[0], button_state);
        
        // Draw text content below button with proper spacing - split into two lines
        SDL_Color current_text_color = text_colors[page1_text_color];
        draw_text("Welcome to Page 1!", (int)(actual_width/2 + offset_x), button_y + button_height + 80, current_text_color);
        draw_text("Swipe right to see buttons.", (int)(actual_width/2 + offset_x), button_y + button_height + 110, current_text_color);
    }
    else if (page_index == 1) {
        // Page 2: Scrollable buttons
        // Get current scroll position
        int scroll_position = page->scroll_position;
        
        // Create separate clip rects for the scrollable and fixed areas
        
        // First, create clip rect just for the scrollable button area on the left side
        SDL_Rect button_area_rect = {
            (int)offset_x, 0, 
            BUTTON_PADDING + BUTTON_WIDTH + BUTTON_PADDING, // Width includes left padding, button width, and right padding
            actual_height
        };
        SDL_RenderSetClipRect(renderer, &button_area_rect);
        
        // Draw page title in the button area (scrolls with content)
        int title_y = 60 - scroll_position;
        if (title_y >= 0 && title_y <= actual_height) {
            draw_large_text(page->title, (BUTTON_PADDING + BUTTON_WIDTH) / 2 + (int)offset_x, title_y, 
                          (SDL_Color){255, 255, 255, 255});
        }
        
        // Update time for button 3
        if (show_time) {
            time_t rawtime;
            struct tm* timeinfo;
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            // Format: HH:MM:SS on first line, YYYY-MMM-DD on second line
            char time_str[32], date_str[32];
            strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);
            strftime(date_str, sizeof(date_str), "%Y-%b-%d", timeinfo); // %b gives short month name
            snprintf(page->button_texts[2], 64, "%s\n%s", time_str, date_str);
        } else {
            strcpy(page->button_texts[2], "Time (off)");
        }
        
        // Render all buttons with scroll position applied
        for (int i = 0; i < page->button_count; i++) {
            int button_y = BUTTON_PADDING + i * (BUTTON_HEIGHT + BUTTON_PADDING) + 90 - scroll_position; // Add space for title
            
            // Skip buttons that are completely outside the visible area
            if (button_y + BUTTON_HEIGHT < 0 || button_y > actual_height) {
                continue;
            }
            
            // Determine button state
            ButtonState button_state = BUTTON_NORMAL;
            if (gestures_get_state() != GESTURE_NONE && 
                gestures_get_button() == i && 
                gestures_get_page() == page_index &&
                pages_get_current() == page_index) {
                button_state = BUTTON_PRESSED;
            }
            
            render_button((int)(BUTTON_PADDING + offset_x), button_y, BUTTON_WIDTH, BUTTON_HEIGHT,
                         page->button_texts[i], page->button_colors[i], button_state);
        }
        
        // Now set clip rect for the fixed API data area on the right side
        SDL_Rect api_area_rect = {
            BUTTON_PADDING + BUTTON_WIDTH + BUTTON_PADDING + (int)offset_x, 0,
            actual_width - (BUTTON_PADDING + BUTTON_WIDTH + BUTTON_PADDING),
            actual_height
        };
        SDL_RenderSetClipRect(renderer, &api_area_rect);
        
        // Render API data in the blank area on the right side (fixed, not scrolling)
        // Position it to the right of the buttons with proper padding
        int api_data_x = BUTTON_PADDING + BUTTON_WIDTH + 20 + (int)offset_x;
        render_api_data(renderer, api_data_x, 150);
        
    }
    
    // Clear clipping rectangle
    SDL_RenderSetClipRect(renderer, NULL);
}

// Get the button at a specific position - exact same implementation as original
int get_button_at_position(int x, int y, int scroll_offset) {
    int current_page = pages_get_current();
    
    // Check which page we're on
    if (current_page == 0) {
        // Page 1 has just one centered button
        int button_x = (actual_width - BUTTON_WIDTH) / 2;
        int button_y = 200;
        int button_height = BUTTON_HEIGHT / 2;
        
        if (x >= button_x && x < button_x + BUTTON_WIDTH &&
            y >= button_y && y < button_y + button_height) {
            return 0;
        }
    }
    else if (current_page == 1) {
        // Page 2 has multiple buttons with scrolling
        
        // Adjust y for scroll position
        int adjusted_y = y + scroll_offset;
        
        // Check if x is within button width
        if (x < BUTTON_PADDING || x >= BUTTON_PADDING + BUTTON_WIDTH) {
            return -1;
        }
        
        // Calculate which button is at this y position
        Page* page = pages_get(current_page);
        for (int i = 0; i < page->button_count; i++) {
            int button_y = BUTTON_PADDING + i * (BUTTON_HEIGHT + BUTTON_PADDING);
            if (adjusted_y >= button_y && adjusted_y < button_y + BUTTON_HEIGHT) {
                return i;
            }
        }
    }
    
    return -1;
}

// Handle a button click - exact same implementation as original
void handle_click(int button_index) {
    int gesture_page = gestures_get_page();
    log_event("BUTTON_CLICK", "button=%d page=%d", button_index, gesture_page);
    
    // Mirror button press to widget integration layer
    if (widget_integration) {
        Page* page = pages_get(gesture_page);
        const char* button_text = (page && button_index < page->button_count) ? 
                                page->button_texts[button_index] : NULL;
        widget_integration_mirror_button_press(widget_integration, button_index, button_text);
    }
    
    // Handle button actions based on page and button index
    if (gesture_page == 0) {
        // Page 1 buttons
        switch (button_index) {
            case 0: // Change text color
                page1_text_color = (page1_text_color + 1) % 7;
                log_info("Text color changed to index %d", page1_text_color);
                break;
        }
    }
    else if (gesture_page == 1) {
        // Page 2 buttons
        switch (button_index) {
            case 0: // Blue
                // Set background to blue
                bg_color = (SDL_Color){41, 128, 185, 255}; // Blue
                log_info("Background color set to blue");
                break;
                
            case 1: // Random
                // Set background to random color
                bg_color.r = rand() % 256;
                bg_color.g = rand() % 256;
                bg_color.b = rand() % 256;
                log_info("Background color set to RGB(%d,%d,%d)", 
                       bg_color.r, bg_color.g, bg_color.b);
                break;
                
            case 2: // Time
                // Toggle time display
                show_time = !show_time;
                log_info("Time display: %s", show_time ? "enabled" : "disabled");
                break;
                
            case 3: // Go to Page 1
                // Go to Page 1
                if (pages_get_current() != 0 && pages_get_target_page() == -1) {
                    pages_transition_to(0);
                }
                break;
                
            case 4: // Refresh User
                // Force refresh API data
                log_info("Refreshing user data");
                api_manager_fetch_user_async(api_manager);
                break;
                
            case 5: // Exit
                // Exit application
                log_info("Exit button pressed");
                quit = true;
                break;
        }
    }
}


void render_api_data(SDL_Renderer* renderer, int x, int y) {
    (void)renderer; // Unused parameter
    
    SDL_Color text_color = {255, 255, 255, 255};
    
    // Calculate the maximum width for text (with proper padding like buttons)
    int right_padding = BUTTON_PADDING; // Use same padding as buttons
    int max_width = actual_width - x - right_padding;
    
    // Set line height for smaller font - exact from original
    int line_height = 25;
    
    ApiState state = api_manager_get_state(api_manager);
    
    if (state == API_STATE_LOADING) {
        draw_small_text_left("Loading user data...", x, y, text_color, max_width);
    }
    else if (state == API_STATE_SUCCESS && current_user_data.is_valid) {
        // Render the API data nicely formatted - exact format from original
        char buffer[256];
        
        // Name
        snprintf(buffer, sizeof(buffer), "Name: %s", current_user_data.name);
        draw_small_text_left(buffer, x, y, text_color, max_width);
        
        // Age
        snprintf(buffer, sizeof(buffer), "Age: %d", current_user_data.age);
        draw_small_text_left(buffer, x, y + line_height, text_color, max_width);
        
        // Nationality
        snprintf(buffer, sizeof(buffer), "Nationality: %s", current_user_data.nationality);
        draw_small_text_left(buffer, x, y + line_height * 2, text_color, max_width);
        
        // Location
        snprintf(buffer, sizeof(buffer), "Location: %s", current_user_data.location);
        draw_small_text_left(buffer, x, y + line_height * 3, text_color, max_width);
        
        // Email 
        snprintf(buffer, sizeof(buffer), "Email: %s", current_user_data.email);
        draw_small_text_left(buffer, x, y + line_height * 4, text_color, max_width);
        
        // Phone
        snprintf(buffer, sizeof(buffer), "Phone: %s", current_user_data.phone);
        draw_small_text_left(buffer, x, y + line_height * 5, text_color, max_width);
        
        // Picture URL info (just show that it's available)
        if (strlen(current_user_data.picture_url) > 0) {
            draw_small_text_left("Image URL available", x, y + line_height * 6, text_color, max_width);
        }
    }
    else if (state == API_STATE_ERROR) {
        const char* error_msg = api_manager_get_error_message(api_manager);
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Error: %s", error_msg);
        draw_small_text_left(buffer, x, y, text_color, max_width);
    }
    else {
        draw_small_text_left("No user data loaded", x, y, text_color, max_width);
    }
}

// API callback functions
void on_api_data_received(const UserData* data, void* context) {
    (void)context; // Unused
    
    if (data) {
        current_user_data = *data;
        log_info("API data received: %s", current_user_data.name);
        
        // Mirror API data to widget integration layer
        if (widget_integration) {
            widget_integration_mirror_user_data(widget_integration, data, sizeof(*data));
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

