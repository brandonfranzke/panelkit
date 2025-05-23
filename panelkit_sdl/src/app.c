#include <SDL.h>
#include <SDL_ttf.h>
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

// Embedded font data
#include "embedded_font.h"

// Available fonts to test:
//#define FONT_PATH "../fonts/LiberationSans-Regular.ttf"  // Liberation Sans (Helvetica-like)
// #define FONT_PATH "../fonts/roboto-regular.ttf"                  // Roboto (Google's font)
// #define FONT_PATH "../fonts/noto-sans-regular.ttf"               // Noto Sans (Google)
// #define FONT_PATH "../fonts/dejavu-sans.ttf"             // DejaVu Sans (good for embedded)
// #define FONT_PATH "/System/Library/Fonts/Helvetica.ttc" // Helvetica (macOS only)

// Screen dimensions
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

// Button dimensions (per specification)
#define BUTTON_WIDTH (SCREEN_WIDTH / 2)
#define BUTTON_HEIGHT ((SCREEN_HEIGHT * 2) / 3)
#define BUTTON_PADDING 20

// Page indicator
#define PAGE_INDICATOR_RADIUS 4
#define PAGE_INDICATOR_SPACING 16
#define PAGE_INDICATOR_Y (SCREEN_HEIGHT - 20)
#define PAGE_INDICATOR_CONTAINER_PADDING 12
#define PAGE_INDICATOR_CONTAINER_HEIGHT 24 // Fixed height for pill/capsule

// Gesture thresholds
#define CLICK_TIMEOUT_MS 300         // Time in ms to consider a press a "click" vs "hold"
#define DRAG_THRESHOLD_PX 10         // Minimum movement in pixels to consider a drag
#define HOLD_THRESHOLD_MS 500        // Time in ms to consider a press a "hold" vs "click"
#define PAGE_SWIPE_THRESHOLD_PX 100  // Minimum horizontal movement to trigger page change

// Gesture states
typedef enum {
    GESTURE_NONE,         // No gesture in progress
    GESTURE_POTENTIAL,    // Press detected, waiting to determine gesture type
    GESTURE_CLICK,        // Quick press and release (under thresholds)
    GESTURE_DRAG_VERT,    // Vertical dragging (scrolling)
    GESTURE_DRAG_HORZ,    // Horizontal dragging (page swiping)
    GESTURE_HOLD          // Press and hold (over time threshold)
} GestureState;

// Page transition states
typedef enum {
    TRANSITION_NONE,      // No transition in progress
    TRANSITION_DRAGGING,  // User is actively dragging between pages
    TRANSITION_ANIMATING  // Automatic animation after drag release
} TransitionState;

// Button states
typedef enum {
    BUTTON_NORMAL,
    BUTTON_HOVER,
    BUTTON_PRESSED,
    BUTTON_HELD
} ButtonState;

// Page definition
typedef struct {
    int scroll_position;         // Vertical scroll position for this page
    int max_scroll;              // Maximum scroll value for this page
    const char* title;           // Page title
    int button_count;            // Number of buttons on this page
    char button_texts[9][64];    // Text for each button
    SDL_Color button_colors[9];  // Color for each button
} Page;

// Global variables
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;
TTF_Font* large_font = NULL;
TTF_Font* small_font = NULL; // Smaller font for API data
bool quit = false;

// Background color
SDL_Color bg_color = {33, 33, 33, 255}; // Dark gray

// Time display
bool show_time = true; // Whether to show time on the third button

// Pages
int current_page = 0;  // Start on page 1 (0-indexed, so 0 is the first page)
int total_pages = 2;   // Total number of pages
Page pages[2];         // Array of pages
float page_transition = 0.0f;  // For smooth page transitions (0.0 = current page, 1.0 = target page)
int target_page = -1;          // Page we're transitioning to, or -1 if not transitioning
TransitionState transition_state = TRANSITION_NONE; // Current transition state
float drag_offset = 0.0f;      // Current drag offset for page transitions

// Page indicator visibility
bool show_indicators = false;   // Only show indicators during swipe and briefly after
Uint32 indicator_hide_time = 0; // When to hide the indicators
int indicator_alpha = 153;      // Indicator alpha value (60% opacity)
bool indicator_fading = false;  // Whether indicator is currently fading out
Uint32 fade_start_time = 0;     // When the fade animation started
#define FADE_DURATION 400       // Fade duration in milliseconds

// API-related variables
Uint32 last_api_call_time = 0;  // Used by API functions

// API data
typedef struct {
    char* data;
    size_t size;
    bool is_ready;
    bool is_loading;
    pthread_mutex_t mutex;
    
    // User data fields
    char name[128];
    char email[128];
    char location[128];
    char phone[64];
    char picture_url[256];
    char nationality[32];
    int age;
} ApiResponse;

ApiResponse api_response = {0};

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

// Gesture state tracking
GestureState current_gesture = GESTURE_NONE;
int gesture_button = -1;        // Button involved in current gesture, if any
int gesture_page = -1;          // Page involved in current gesture, if any
Uint32 gesture_start_time = 0;  // When the gesture started
int gesture_start_x = 0;        // Starting X position
int gesture_start_y = 0;        // Starting Y position
int last_mouse_x = 0;           // Last mouse X for delta calculations
int last_mouse_y = 0;           // Last mouse Y for delta calculations

// Debug info
bool show_debug = true;
Uint32 frame_count = 0;
Uint32 fps_timer = 0;
Uint32 fps = 0;

// Function prototypes
void render_button(int x, int y, int w, int h, const char* text, SDL_Color color, ButtonState state);
void draw_text(const char* text, int x, int y, SDL_Color color);
void draw_text_left(const char* text, int x, int y, SDL_Color color);
void draw_small_text_left(const char* text, int x, int y, SDL_Color color, int max_width);
void draw_large_text(const char* text, int x, int y, SDL_Color color);
void begin_gesture(int x, int y, int button_index);
void update_gesture(int x, int y);
void end_gesture(int x, int y);
void cancel_gesture();
void handle_click(int button_index);
int get_button_at_position(int x, int y, int scroll_offset);
void initialize_pages();
void render_page(int page_index, float offset_x);
void render_page_indicators(int current, int total, float transition);
void transition_to_page(int page_index);

// API functions
size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata);
void* fetch_api_data(void* arg);
void update_api_data(Uint32 current_time, bool force_refresh);
void render_api_data(SDL_Renderer* renderer, int x, int y);
void init_api();
void cleanup_api();
void parse_api_response();

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
    log_build_info();
    
    // Log command line arguments
    log_debug("Command line: %d arguments", argc);
    for (int i = 0; i < argc; i++) {
        log_debug("  argv[%d] = %s", i, argv[i]);
    }
    
    // Initialize SDL
    log_state_change("SDL", "NONE", "INITIALIZING");
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_SDL_ERROR("SDL initialization failed");
        logger_shutdown();
        return 1;
    }
    log_state_change("SDL", "INITIALIZING", "READY");
    
    // Initialize SDL_ttf
    log_state_change("SDL_ttf", "NONE", "INITIALIZING");
    if (TTF_Init() < 0) {
        log_error("SDL_ttf initialization failed: %s", TTF_GetError());
        SDL_Quit();
        logger_shutdown();
        return 1;
    }
    log_state_change("SDL_ttf", "INITIALIZING", "READY");
    
    // Create window
    log_info("Creating window: %dx%d", SCREEN_WIDTH, SCREEN_HEIGHT);
    window = SDL_CreateWindow("PanelKit", 
                            SDL_WINDOWPOS_CENTERED, 
                            SDL_WINDOWPOS_CENTERED, 
                            SCREEN_WIDTH, SCREEN_HEIGHT, 
                            SDL_WINDOW_SHOWN);
    if (window == NULL) {
        LOG_SDL_ERROR("Window creation failed");
        TTF_Quit();
        SDL_Quit();
        logger_shutdown();
        return 1;
    }
    log_state_change("Window", "NONE", "CREATED");
    
    // Create renderer
    log_info("Creating renderer");
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        LOG_SDL_ERROR("Renderer creation failed");
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        logger_shutdown();
        return 1;
    }
    log_state_change("Renderer", "NONE", "CREATED");
    
    // Log display info after renderer creation
    log_display_info(SCREEN_WIDTH, SCREEN_HEIGHT, "SDL");
    
    // Load embedded fonts
    SDL_RWops* font_rw_24 = SDL_RWFromConstMem(embedded_font_data, embedded_font_size);
    SDL_RWops* font_rw_32 = SDL_RWFromConstMem(embedded_font_data, embedded_font_size);
    SDL_RWops* font_rw_18 = SDL_RWFromConstMem(embedded_font_data, embedded_font_size);
    
    font = TTF_OpenFontRW(font_rw_24, 1, 24);           // 1 = freesrc
    large_font = TTF_OpenFontRW(font_rw_32, 1, 32);     // 1 = freesrc  
    small_font = TTF_OpenFontRW(font_rw_18, 1, 18);     // 1 = freesrc
    if (font == NULL || large_font == NULL || small_font == NULL) {
        log_error("Font loading failed: %s", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        logger_shutdown();
        return 1;
    }
    log_info("Fonts loaded successfully");
    
    // Initialize pages
    initialize_pages();
    
    // Initialize API
    init_api();
    
    log_info("PanelKit initialized successfully");
    log_info("Controls:");
    log_info("  • Swipe horizontally to switch pages");
    log_info("  • Swipe vertically to scroll content");
    log_info("  • Tap buttons to activate them");
    log_info("  • D: Toggle debug info");
    log_info("  • ESC: Quit");
    log_debug("Pages:");
    log_debug("  • Page 1: Text page with color changing button");
    log_debug("  • Page 2: Buttons page with page navigation");
    
    log_state_change("Application", "INITIALIZED", "RUNNING");
    
    // Timing variables
    Uint32 frame_start;
    Uint32 frame_time;
    
    // Initialize FPS timer
    fps_timer = SDL_GetTicks();
    
    // Main loop
    while (!quit) {
        frame_start = SDL_GetTicks();
        
        // Get current mouse position
        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);
        
        // Event handling
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            switch (e.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                
                case SDL_KEYDOWN:
                    if (e.key.keysym.sym == SDLK_ESCAPE) {
                        quit = true;
                    }
                    else if (e.key.keysym.sym == SDLK_d) {
                        // Toggle debug overlay
                        show_debug = !show_debug;
                        log_info("Debug overlay: %s", show_debug ? "enabled" : "disabled");
                    }
                    else if (e.key.keysym.sym == SDLK_LEFT) {
                        // Previous page
                        if (current_page > 0 && target_page == -1) {
                            transition_to_page(current_page - 1);
                        }
                    }
                    else if (e.key.keysym.sym == SDLK_RIGHT) {
                        // Next page
                        if (current_page < total_pages - 1 && target_page == -1) {
                            transition_to_page(current_page + 1);
                        }
                    }
                    else if (e.key.keysym.sym == SDLK_UP) {
                        // Scroll up
                        pages[current_page].scroll_position -= 50;
                        if (pages[current_page].scroll_position < 0) {
                            pages[current_page].scroll_position = 0;
                        }
                    }
                    else if (e.key.keysym.sym == SDLK_DOWN) {
                        // Scroll down
                        pages[current_page].scroll_position += 50;
                        if (pages[current_page].scroll_position > pages[current_page].max_scroll) {
                            pages[current_page].scroll_position = pages[current_page].max_scroll;
                        }
                    }
                    break;
                
                case SDL_MOUSEBUTTONDOWN:
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        // Only start a gesture if not currently transitioning between pages
                        if (target_page == -1) {
                            // Get button under mouse (if any)
                            int button_index = get_button_at_position(
                                e.button.x, e.button.y, pages[current_page].scroll_position);
                            
                            // Start a new gesture
                            begin_gesture(e.button.x, e.button.y, button_index);
                            gesture_page = current_page;
                        }
                    }
                    break;
                
                case SDL_MOUSEBUTTONUP:
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        // End the current gesture (if any)
                        end_gesture(e.button.x, e.button.y);
                    }
                    break;
                
                case SDL_MOUSEMOTION:
                    // Update current gesture (if any)
                    update_gesture(e.motion.x, e.motion.y);
                    break;
            }
        }
        
        // Process page transition animation
        if (transition_state == TRANSITION_DRAGGING) {
            // During dragging, page_transition follows drag_offset directly
            page_transition = drag_offset;
        }
        else if (transition_state == TRANSITION_ANIMATING) {
            // Automatic animation after drag ends (moderate swipe speed)
            float transition_speed = 0.06f; // Slower, more natural swipe
            
            if (target_page > current_page) {
                // Moving to next page (negative transition)
                if (page_transition > -1.0f) {
                    page_transition -= transition_speed; // Move toward -1.0
                } else {
                    // Transition complete
                    page_transition = 0.0f;
                    current_page = target_page;
                    target_page = -1;
                    transition_state = TRANSITION_NONE;
                }
            } else if (target_page < current_page) {
                // Moving to previous page (positive transition)
                if (page_transition < 1.0f) {
                    page_transition += transition_speed; // Move toward 1.0
                } else {
                    // Transition complete
                    page_transition = 0.0f;
                    current_page = target_page;
                    target_page = -1;
                    transition_state = TRANSITION_NONE;
                }
            } else {
                // Snapping back to current page
                float direction = (page_transition > 0) ? -1.0f : 1.0f;
                page_transition += direction * transition_speed;
                
                // Check if we've returned to initial position
                if ((direction < 0 && page_transition <= 0.0f) || 
                    (direction > 0 && page_transition >= 0.0f)) {
                    page_transition = 0.0f;
                    target_page = -1;
                    transition_state = TRANSITION_NONE;
                }
            }
        }
        
        // Check for hold timeout (switch from potential to hold)
        if (current_gesture == GESTURE_POTENTIAL) {
            Uint32 time_elapsed = SDL_GetTicks() - gesture_start_time;
            if (time_elapsed > HOLD_THRESHOLD_MS) {
                current_gesture = GESTURE_HOLD;
                printf("Gesture changed to HOLD (timeout)\n");
            }
        }
        
        // Update timers
        Uint32 current_time = SDL_GetTicks();
        
        // Frame rate calculation
        frame_count++;
        if (current_time - fps_timer > 1000) {
            fps = frame_count;
            frame_count = 0;
            fps_timer = current_time;
        }
        
        // Handle indicator fading
        if (show_indicators && current_time > indicator_hide_time && transition_state == TRANSITION_NONE) {
            if (!indicator_fading) {
                indicator_fading = true;
                fade_start_time = current_time;
            } else {
                // Calculate fade progress
                Uint32 fade_time = current_time - fade_start_time;
                if (fade_time >= FADE_DURATION) {
                    // Fade complete
                    show_indicators = false;
                    indicator_fading = false;
                    indicator_alpha = 153; // Reset to default
                } else {
                    // Calculate alpha based on fade progress
                    float progress = (float)fade_time / FADE_DURATION;
                    indicator_alpha = (int)(153 * (1.0f - progress));
                }
            }
        }
        
        // Check if we need to update API data
        update_api_data(current_time, false);
        
        // Clear screen with current background color
        SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        SDL_RenderClear(renderer);
        
        // Update time button text if needed
        if (show_time) {
            time_t now = time(NULL);
            struct tm* timeinfo = localtime(&now);
            strftime(pages[1].button_texts[2], sizeof(pages[1].button_texts[0]), 
                    "%H:%M:%S\n%Y-%m-%d", timeinfo);
        } else {
            strcpy(pages[1].button_texts[2], "Time (off)");
        }
        
        // Determine which pages to draw and their offsets
        if (transition_state == TRANSITION_NONE) {
            // Not transitioning, just draw current page
            render_page(current_page, 0);
        } else {
            // Transitioning between pages (either by dragging or animating)
            float current_offset = page_transition * SCREEN_WIDTH;
            
            if (target_page > current_page) {
                // Moving to next page, current slides left (negative transition)
                render_page(current_page, current_offset);
                render_page(target_page, SCREEN_WIDTH + current_offset);
            } else if (target_page < current_page) {
                // Moving to previous page, current slides right (positive transition)
                render_page(current_page, current_offset);
                render_page(target_page, -SCREEN_WIDTH + current_offset);
            } else {
                // Elastic bounce at edges (without showing another page)
                render_page(current_page, current_offset);
            }
        }
        
        // Render page indicators (only when they should be visible)
        if (show_indicators || transition_state != TRANSITION_NONE) {
            render_page_indicators(current_page, total_pages, page_transition);
        }
        
        // Draw debug overlay if enabled
        if (show_debug) {
            // Draw semi-transparent background
            SDL_Rect debug_bg = {0, 0, SCREEN_WIDTH, 60};
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
            SDL_RenderFillRect(renderer, &debug_bg);
            
            // Status line
            char status[128];
            const char* gesture_name = 
                current_gesture == GESTURE_NONE ? "NONE" :
                current_gesture == GESTURE_POTENTIAL ? "POTENTIAL" :
                current_gesture == GESTURE_CLICK ? "CLICK" :
                current_gesture == GESTURE_DRAG_VERT ? "DRAG_VERT" :
                current_gesture == GESTURE_DRAG_HORZ ? "DRAG_HORZ" : "HOLD";
                
            sprintf(status, "Page: %d | Gesture: %s | FPS: %d", 
                    current_page + 1, gesture_name, fps);
            draw_text(status, SCREEN_WIDTH / 2, 15, (SDL_Color){255, 255, 255, 255});
            
            // Gesture info
            char gesture_info[128];
            sprintf(gesture_info, "Button: %d | Page: %d | Transition: %.2f", 
                    gesture_button, gesture_page, page_transition);
            draw_text(gesture_info, SCREEN_WIDTH / 2, 40, (SDL_Color){200, 200, 200, 255});
            
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
        
        // Present renderer
        SDL_RenderPresent(renderer);
        
        // Cap frame rate
        frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < 16) {
            SDL_Delay(16 - frame_time); // ~60 FPS
        }
        
        // Log frame time periodically
        log_frame_time((float)(SDL_GetTicks() - frame_start));
    }
    
    // Cleanup
    log_state_change("Application", "RUNNING", "SHUTDOWN");
    log_info("Shutting down...");
    
    cleanup_api();
    TTF_CloseFont(font);
    TTF_CloseFont(large_font);
    TTF_CloseFont(small_font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    
    log_info("=== PanelKit Shutdown Complete ===");
    logger_shutdown();
    
    return 0;
}

// Initialize all pages
void initialize_pages() {
    // Page 1 - Text page
    pages[0].scroll_position = 0;
    pages[0].max_scroll = 0; // No scrolling needed
    pages[0].title = "Text Page";
    pages[0].button_count = 1;
    strcpy(pages[0].button_texts[0], "Change Text Color");
    pages[0].button_colors[0] = (SDL_Color){52, 152, 219, 255}; // Blue
    
    // Page 2 - Buttons page
    pages[1].scroll_position = 0;
    pages[1].title = "Buttons Page";
    pages[1].button_count = 9;
    
    // Button texts
    strcpy(pages[1].button_texts[0], "Blue");
    strcpy(pages[1].button_texts[1], "Random");
    strcpy(pages[1].button_texts[2], "Time"); // Will be updated with time
    strcpy(pages[1].button_texts[3], "Go to Page 1");
    strcpy(pages[1].button_texts[4], "Refresh User");
    strcpy(pages[1].button_texts[5], "Exit App");
    strcpy(pages[1].button_texts[6], "Button 7");
    strcpy(pages[1].button_texts[7], "Button 8");
    strcpy(pages[1].button_texts[8], "Button 9");
    
    // Button colors
    pages[1].button_colors[0] = (SDL_Color){41, 128, 185, 255};  // Blue
    pages[1].button_colors[1] = (SDL_Color){142, 68, 173, 255};  // Purple
    pages[1].button_colors[2] = (SDL_Color){142, 142, 142, 255}; // Gray
    pages[1].button_colors[3] = (SDL_Color){231, 76, 60, 255};   // Red
    pages[1].button_colors[4] = (SDL_Color){39, 174, 96, 255};  // Green (for refresh)
    pages[1].button_colors[5] = (SDL_Color){192, 57, 43, 255};   // Dark Red (for exit)
    pages[1].button_colors[6] = (SDL_Color){52, 152, 219, 255};  // Blue
    pages[1].button_colors[7] = (SDL_Color){241, 196, 15, 255};  // Yellow
    pages[1].button_colors[8] = (SDL_Color){230, 126, 34, 255};  // Orange
    
    // Calculate max scroll for page 2 (add space for title)
    int total_content_height = 90 + BUTTON_PADDING + pages[1].button_count * (BUTTON_HEIGHT + BUTTON_PADDING);
    pages[1].max_scroll = total_content_height - SCREEN_HEIGHT;
    if (pages[1].max_scroll < 0) pages[1].max_scroll = 0;
}

// Start a page transition
void transition_to_page(int page_index) {
    if (page_index >= 0 && page_index < total_pages && page_index != current_page) {
        target_page = page_index;
        transition_state = TRANSITION_ANIMATING;
        
        // Set initial transition value
        if (target_page > current_page) {
            // Moving forward (current slides left, new comes from right)
            page_transition = 0.0f;
        } else {
            // Moving backward (current slides right, new comes from left)
            page_transition = 0.0f;
        }
        
        log_event("PAGE_TRANSITION", "from=%d to=%d", current_page, page_index);
    }
}

// Render a specific page at the given horizontal offset
void render_page(int page_index, float offset_x) {
    // Create clip rect for the page - use full height since indicators will float on top
    SDL_Rect page_rect = {
        (int)offset_x, 0, 
        SCREEN_WIDTH, SCREEN_HEIGHT
    };
    SDL_RenderSetClipRect(renderer, &page_rect);
    
    if (page_index == 0) {
        // Page 1 - Text page
        // Draw page title
        draw_large_text(pages[0].title, SCREEN_WIDTH / 2 + (int)offset_x, 60, 
                      (SDL_Color){255, 255, 255, 255});
        
        // Draw text content
        draw_text(page1_text, SCREEN_WIDTH / 2 + (int)offset_x, 120, 
                text_colors[page1_text_color]);
        
        // Draw color change button
        int button_x = (SCREEN_WIDTH - BUTTON_WIDTH) / 2;
        int button_y = 200;
        
        // Determine button state
        ButtonState state = BUTTON_NORMAL;
        if (current_gesture != GESTURE_NONE && gesture_page == 0) {
            if (gesture_button == 0) {
                if (current_gesture == GESTURE_POTENTIAL || current_gesture == GESTURE_CLICK) {
                    state = BUTTON_PRESSED;
                } else if (current_gesture == GESTURE_HOLD) {
                    state = BUTTON_HELD;
                }
            }
        }
        
        render_button(button_x + (int)offset_x, button_y, 
                     BUTTON_WIDTH, BUTTON_HEIGHT / 2, // Half height
                     pages[0].button_texts[0], 
                     pages[0].button_colors[0], state);
    } 
    else if (page_index == 1) {
        // Page 2 - Buttons page
        // Get current scroll position
        int scroll_position = pages[1].scroll_position;
        
        // Create separate clip rects for the scrollable and fixed areas
        
        // First, create clip rect just for the scrollable button area on the left side
        SDL_Rect button_area_rect = {
            (int)offset_x, 0, 
            BUTTON_PADDING + BUTTON_WIDTH + BUTTON_PADDING, // Width includes left padding, button width, and right padding
            SCREEN_HEIGHT
        };
        SDL_RenderSetClipRect(renderer, &button_area_rect);
        
        // Draw page title in the button area (scrolls with content)
        int title_y = 60 - scroll_position;
        if (title_y >= 0 && title_y <= SCREEN_HEIGHT) {
            draw_large_text(pages[1].title, (BUTTON_PADDING + BUTTON_WIDTH) / 2 + (int)offset_x, title_y, 
                          (SDL_Color){255, 255, 255, 255});
        }
        
        // Render all buttons with scroll position applied
        for (int i = 0; i < pages[1].button_count; i++) {
            int button_y = BUTTON_PADDING + i * (BUTTON_HEIGHT + BUTTON_PADDING) + 90 - scroll_position; // Add space for title
            
            // Skip buttons that are completely outside the visible area
            if (button_y + BUTTON_HEIGHT < 0 || button_y > SCREEN_HEIGHT) {
                continue;
            }
            
            // Determine button state
            ButtonState state = BUTTON_NORMAL;
            if (current_gesture != GESTURE_NONE && gesture_page == 1) {
                if (gesture_button == i) {
                    if (current_gesture == GESTURE_POTENTIAL || current_gesture == GESTURE_CLICK) {
                        state = BUTTON_PRESSED;
                    } else if (current_gesture == GESTURE_HOLD) {
                        state = BUTTON_HELD;
                    }
                }
            }
            
            render_button(BUTTON_PADDING + (int)offset_x, button_y, 
                         BUTTON_WIDTH, BUTTON_HEIGHT, 
                         pages[1].button_texts[i], 
                         pages[1].button_colors[i], state);
        }
        
        // Now set clip rect for the fixed API data area on the right side
        SDL_Rect api_area_rect = {
            BUTTON_PADDING + BUTTON_WIDTH + BUTTON_PADDING + (int)offset_x, 0,
            SCREEN_WIDTH - (BUTTON_PADDING + BUTTON_WIDTH + BUTTON_PADDING),
            SCREEN_HEIGHT
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

// Render page indicator dots
void render_page_indicators(int current, int total, float transition) {
    (void)transition; // Used for transition effects below
    int indicator_width = total * (PAGE_INDICATOR_RADIUS * 2 + PAGE_INDICATOR_SPACING) - PAGE_INDICATOR_SPACING;
    
    // Calculate precise positions for proper symmetry
    int container_width = indicator_width + PAGE_INDICATOR_CONTAINER_PADDING * 2;
    int container_height = PAGE_INDICATOR_CONTAINER_HEIGHT; // Fixed height for capsule
    int container_x = (SCREEN_WIDTH - container_width) / 2;
    int container_y = PAGE_INDICATOR_Y - container_height / 2;
    
    // The radius for the capsule ends is exactly half the height
    int capsule_radius = container_height / 2;
    
    // Draw the pill/capsule with alpha blend (using current alpha value for fading)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, indicator_alpha); // Current alpha for fading
    
    // Draw the capsule shape with inward-facing half-circles (<III>)
    // First, draw the full pill rectangle
    SDL_Rect full_rect = {
        container_x,
        container_y,
        container_width,
        container_height
    };
    SDL_RenderFillRect(renderer, &full_rect);
    
    // Now remove the corners to create inward-facing half-circles
    for (int y = 0; y < container_height; y++) {
        for (int x = 0; x < capsule_radius; x++) {
            // Calculate distance from the imaginary circle centers outside the pill
            float dx_left = x - capsule_radius;
            float dy_left = y - capsule_radius;
            float distance_left = sqrt(dx_left*dx_left + dy_left*dy_left);
            
            // If point is outside our "inward circle", erase it (draw with transparent color)
            if (distance_left > capsule_radius) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0); // Fully transparent
                SDL_RenderDrawPoint(renderer, container_x + x, container_y + y);
            }
            
            // Right side (mirrored)
            float dx_right = x - capsule_radius;
            float dy_right = y - capsule_radius;
            float distance_right = sqrt(dx_right*dx_right + dy_right*dy_right);
            if (distance_right > capsule_radius) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0); // Fully transparent
                SDL_RenderDrawPoint(renderer, 
                    container_x + container_width - x - 1, 
                    container_y + y);
            }
        }
    }
    
    // Restore color for remaining drawing
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 153); // 60% opacity (153/255)
    
    // Position dots for perfect symmetry in the capsule
    int total_indicator_space = (PAGE_INDICATOR_RADIUS * 2 + PAGE_INDICATOR_SPACING) * total - PAGE_INDICATOR_SPACING;
    int start_x = container_x + (container_width - total_indicator_space) / 2;
    
    // Draw the indicator dots (with full opacity)
    for (int i = 0; i < total; i++) {
        int dot_x = start_x + i * (PAGE_INDICATOR_RADIUS * 2 + PAGE_INDICATOR_SPACING) + PAGE_INDICATOR_RADIUS;
        
        // Determine fill based on current page and transition
        bool filled = (i == current);
        int alpha = 255;
        
        // Adjust for transition
        if (target_page != -1) {
            if (i == current || i == target_page) {
                float t = fabs(page_transition);
                if (i == current) {
                    alpha = (int)(255 * (1.0f - t));
                } else {
                    alpha = (int)(255 * t);
                }
            }
        }
        
        // Choose color based on whether it's the active page
        SDL_Color dot_color;
        if (filled) {
            dot_color = (SDL_Color){255, 255, 255, 255}; // White for active
        } else {
            dot_color = (SDL_Color){150, 150, 150, 255}; // Gray for inactive
        }
        
        // Draw the indicator dot with full opacity but adjusted alpha for transitions
        SDL_SetRenderDrawColor(renderer, dot_color.r, dot_color.g, dot_color.b, alpha);
        
        // Draw filled circle
        for (int r = 0; r <= PAGE_INDICATOR_RADIUS; r++) {
            for (int angle = 0; angle < 360; angle += 5) {
                float rad = angle * M_PI / 180.0f;
                int x = dot_x + (int)(r * cos(rad));
                int y = PAGE_INDICATOR_Y + (int)(r * sin(rad));
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

// Begin a new gesture
void begin_gesture(int x, int y, int button_index) {
    current_gesture = GESTURE_POTENTIAL;
    gesture_button = button_index;
    gesture_start_time = SDL_GetTicks();
    gesture_start_x = x;
    gesture_start_y = y;
    last_mouse_x = x;
    last_mouse_y = y;
    
    log_debug("Gesture started at (%d, %d), button: %d", x, y, button_index);
}

// Update an ongoing gesture
void update_gesture(int x, int y) {
    // Only update if there's an active gesture
    if (current_gesture == GESTURE_NONE) {
        return;
    }
    
    // Calculate distance from start position
    int distance_x = x - gesture_start_x;
    int distance_y = y - gesture_start_y;
    float distance = sqrt(distance_x * distance_x + distance_y * distance_y);
    
    // If we're in the potential state and exceed the drag threshold,
    // determine if this is a horizontal or vertical drag
    if (current_gesture == GESTURE_POTENTIAL && distance > DRAG_THRESHOLD_PX) {
        if (abs(distance_x) > abs(distance_y)) {
            // Horizontal drag - page swipe
            current_gesture = GESTURE_DRAG_HORZ;
            log_debug("Gesture: horizontal drag detected (%.1f px)", distance);
        } else {
            // Vertical drag - content scroll
            current_gesture = GESTURE_DRAG_VERT;
            log_debug("Gesture: vertical drag detected (%.1f px)", distance);
        }
    }
    
    // Show indicators whenever we detect horizontal dragging
    if (current_gesture == GESTURE_DRAG_HORZ) {
        show_indicators = true;
        indicator_hide_time = SDL_GetTicks() + 2000; // Keep visible for 2 seconds after last drag
    }

    // Process based on gesture type
    if (current_gesture == GESTURE_DRAG_VERT) {
        // Vertical dragging - scroll content
        int delta_y = last_mouse_y - y;
        
        // Update scroll position for current page
        int old_scroll = pages[current_page].scroll_position;
        pages[current_page].scroll_position += delta_y;
        
        // Enforce limits
        if (pages[current_page].scroll_position < 0) {
            pages[current_page].scroll_position = 0;
        }
        if (pages[current_page].scroll_position > pages[current_page].max_scroll) {
            pages[current_page].scroll_position = pages[current_page].max_scroll;
        }
        
        // Log significant changes
        if (old_scroll != pages[current_page].scroll_position && abs(delta_y) > 5) {
            log_debug("Scroll position: %d (delta: %d)", 
                   pages[current_page].scroll_position, delta_y);
        }
    }
    else if (current_gesture == GESTURE_DRAG_HORZ) {
        // Horizontal dragging - page swiping
        int delta_x = x - gesture_start_x;
        float normalized_delta = (float)delta_x / SCREEN_WIDTH; // Convert to 0.0-1.0 scale
        
        // Start dragging transition if not already in one
        if (transition_state == TRANSITION_NONE) {
            transition_state = TRANSITION_DRAGGING;
            
            // Determine potential target page based on drag direction
            if (normalized_delta > 0 && current_page > 0) {
                // Dragging right - previous page is potential target 
                target_page = current_page - 1;
            } 
            else if (normalized_delta < 0 && current_page < total_pages - 1) {
                // Dragging left - next page is potential target
                target_page = current_page + 1;
            }
            else {
                // Can't go beyond first/last page
                target_page = current_page;
            }
        }
        
        // Update drag offset based on finger position (with limits)
        if (target_page != current_page) {
            // Calculate drag offset based on direction
            if (target_page > current_page) {
                // Dragging to next page (negative offset)
                drag_offset = fmax(normalized_delta, -1.0f); // Limit to -1.0
            } else {
                // Dragging to previous page (positive offset) 
                drag_offset = fmin(normalized_delta, 1.0f);  // Limit to 1.0
            }
        } else {
            // Elastic resistance when trying to go beyond first/last page
            drag_offset = normalized_delta * 0.3f; // Reduced movement to indicate boundary
        }
    }
    
    // Update last position
    last_mouse_x = x;
    last_mouse_y = y;
}

// End the current gesture
void end_gesture(int x, int y) {
    // Only process if there's an active gesture
    if (current_gesture == GESTURE_NONE) {
        return;
    }
    
    // Calculate time elapsed and distance
    Uint32 time_elapsed = SDL_GetTicks() - gesture_start_time;
    int distance_x = x - gesture_start_x;
    int distance_y = y - gesture_start_y;
    float distance = sqrt(distance_x * distance_x + distance_y * distance_y);
    
    log_debug("Gesture ended: %s, time: %dms, distance: %.1fpx", 
           current_gesture == GESTURE_POTENTIAL ? "POTENTIAL" :
           current_gesture == GESTURE_DRAG_VERT ? "DRAG_VERT" :
           current_gesture == GESTURE_DRAG_HORZ ? "DRAG_HORZ" :
           current_gesture == GESTURE_HOLD ? "HOLD" : "UNKNOWN",
           time_elapsed, distance);
    
    // If this was a potential gesture that didn't exceed thresholds,
    // it's a click
    if (current_gesture == GESTURE_POTENTIAL && 
        time_elapsed < CLICK_TIMEOUT_MS && 
        distance < DRAG_THRESHOLD_PX) {
        
        current_gesture = GESTURE_CLICK;
        
        // Handle button click if applicable
        if (gesture_button >= 0 && gesture_page == current_page) {
            handle_click(gesture_button);
        }
    }
    
    // Handle the end of a horizontal drag (page swipe)
    if (current_gesture == GESTURE_DRAG_HORZ && transition_state == TRANSITION_DRAGGING) {
        float half_threshold = 0.3f; // Snap to new page if dragged more than 30% of screen width
        
        // Determine which page to snap to based on drag distance
        if (target_page != current_page) {
            bool snap_to_target = false;
            
            if (target_page > current_page) {
                // Next page: check if dragged far enough left
                snap_to_target = (drag_offset <= -half_threshold);
            } else {
                // Previous page: check if dragged far enough right
                snap_to_target = (drag_offset >= half_threshold);
            }
            
            if (snap_to_target) {
                // Continue to target page
                page_transition = drag_offset;
                transition_state = TRANSITION_ANIMATING;
                log_event("PAGE_SWIPE", "to=%d", target_page);
            } else {
                // Snap back to current page
                transition_state = TRANSITION_ANIMATING;
                // Keep same target and current page, but animate back to 0 offset
                log_debug("Swipe cancelled, returning to page %d", current_page);
            }
        } else {
            // Reset if dragging at page boundary
            transition_state = TRANSITION_NONE;
            target_page = -1;
            drag_offset = 0.0f;
        }
        
        // Show indicators and set timer to hide them after 2 seconds
        show_indicators = true;
        indicator_hide_time = SDL_GetTicks() + 2000; // 2 seconds from now
    }
    
    // Reset gesture state
    current_gesture = GESTURE_NONE;
    gesture_button = -1;
}

// Cancel the current gesture
void cancel_gesture() {
    if (current_gesture != GESTURE_NONE) {
        log_debug("Gesture cancelled");
        current_gesture = GESTURE_NONE;
        gesture_button = -1;
    }
}

// Handle a button click
void handle_click(int button_index) {
    log_event("BUTTON_CLICK", "button=%d page=%d", button_index, gesture_page);
    
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
                if (current_page != 0 && target_page == -1) {
                    transition_to_page(0);
                }
                break;
                
            case 4: // Refresh User
                // Force refresh API data
                log_info("Refreshing user data");
                Uint32 current_time = SDL_GetTicks();
                update_api_data(current_time, true); // Force refresh
                last_api_call_time = current_time;   // Reset the timer
                break;
                
            case 5: // Exit
                // Exit application
                log_info("Exit button pressed");
                quit = true;
                break;
        }
    }
}

// Get the button at a specific position
int get_button_at_position(int x, int y, int scroll_offset) {
    // Check which page we're on
    if (current_page == 0) {
        // Page 1 has just one centered button
        int button_x = (SCREEN_WIDTH - BUTTON_WIDTH) / 2;
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
        for (int i = 0; i < pages[current_page].button_count; i++) {
            int button_y = BUTTON_PADDING + i * (BUTTON_HEIGHT + BUTTON_PADDING);
            if (adjusted_y >= button_y && adjusted_y < button_y + BUTTON_HEIGHT) {
                return i;
            }
        }
    }
    
    return -1;
}

// Render a button with text and state effects
void render_button(int x, int y, int w, int h, const char* text, SDL_Color color, ButtonState state) {
    // Adjust colors based on state
    SDL_Color button_color = color;
    
    // Brighten when hovered
    if (state == BUTTON_HOVER) {
        button_color.r = (Uint8)fmin(color.r * 1.2, 255);
        button_color.g = (Uint8)fmin(color.g * 1.2, 255);
        button_color.b = (Uint8)fmin(color.b * 1.2, 255);
    }
    
    // Darken when pressed
    if (state == BUTTON_PRESSED) {
        button_color.r = (Uint8)(color.r * 0.8);
        button_color.g = (Uint8)(color.g * 0.8);
        button_color.b = (Uint8)(color.b * 0.8);
    }
    
    // Pulse when held
    if (state == BUTTON_HELD) {
        Uint32 pulse = (SDL_GetTicks() / 100) % 10;
        float pulse_factor = 0.8f + (pulse / 10.0f) * 0.4f;
        button_color.r = (Uint8)fmin(color.r * pulse_factor, 255);
        button_color.g = (Uint8)fmin(color.g * pulse_factor, 255);
        button_color.b = (Uint8)fmin(color.b * pulse_factor, 255);
    }
    
    // Draw button background
    SDL_Rect button_rect = {x, y, w, h};
    SDL_SetRenderDrawColor(renderer, button_color.r, button_color.g, button_color.b, button_color.a);
    SDL_RenderFillRect(renderer, &button_rect);
    
    // Draw button border
    if (state != BUTTON_NORMAL) {
        // Thicker border for interactive states
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect outer_border = {x - 2, y - 2, w + 4, h + 4};
        SDL_RenderDrawRect(renderer, &outer_border);
    }
    
    // Normal border
    SDL_SetRenderDrawColor(renderer, 
                          button_color.r * 0.7, 
                          button_color.g * 0.7, 
                          button_color.b * 0.7, 
                          button_color.a);
    SDL_RenderDrawRect(renderer, &button_rect);
    
    // Draw text centered on button
    draw_text(text, x + w/2, y + h/2, (SDL_Color){255, 255, 255, 255});
}

// Draw text centered at given position with normal font
void draw_text(const char* text, int x, int y, SDL_Color color) {
    SDL_Surface* text_surface = TTF_RenderText_Blended(font, text, color);
    if (!text_surface) {
        return;
    }
    
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    if (!text_texture) {
        SDL_FreeSurface(text_surface);
        return;
    }
    
    SDL_Rect text_rect = {
        x - text_surface->w / 2, 
        y - text_surface->h / 2, 
        text_surface->w, 
        text_surface->h
    };
    
    SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
    
    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(text_surface);
}

// Draw text centered at given position with large font
void draw_large_text(const char* text, int x, int y, SDL_Color color) {
    SDL_Surface* text_surface = TTF_RenderText_Blended(large_font, text, color);
    if (!text_surface) {
        return;
    }
    
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    if (!text_texture) {
        SDL_FreeSurface(text_surface);
        return;
    }
    
    SDL_Rect text_rect = {
        x - text_surface->w / 2, 
        y - text_surface->h / 2, 
        text_surface->w, 
        text_surface->h
    };
    
    SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
    
    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(text_surface);
}

// Draw text left-aligned at given position with normal font
void draw_text_left(const char* text, int x, int y, SDL_Color color) {
    SDL_Surface* text_surface = TTF_RenderText_Blended(font, text, color);
    if (!text_surface) {
        return;
    }
    
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    if (!text_texture) {
        SDL_FreeSurface(text_surface);
        return;
    }
    
    SDL_Rect text_rect = {
        x, // Left-aligned, no centering
        y - text_surface->h / 2, // Still center vertically 
        text_surface->w, 
        text_surface->h
    };
    
    SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
    
    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(text_surface);
}

// Draw text left-aligned with small font and width limit
void draw_small_text_left(const char* text, int x, int y, SDL_Color color, int max_width) {
    SDL_Surface* text_surface = TTF_RenderText_Blended(small_font, text, color);
    if (!text_surface) {
        return;
    }
    
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    if (!text_texture) {
        SDL_FreeSurface(text_surface);
        return;
    }
    
    // Calculate dimensions for rendering
    int width = text_surface->w;
    int height = text_surface->h;
    
    // If text exceeds max width, clip it
    SDL_Rect src_rect = {0, 0, width, height};
    if (width > max_width) {
        src_rect.w = max_width;
    }
    
    SDL_Rect dest_rect = {
        x, // Left-aligned
        y - height / 2, // Center vertically
        src_rect.w, // Only show up to max_width
        height
    };
    
    // Render with clipping if needed
    SDL_RenderCopy(renderer, text_texture, &src_rect, &dest_rect);
    
    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(text_surface);
}
