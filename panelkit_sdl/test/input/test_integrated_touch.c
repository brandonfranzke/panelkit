/**
 * Integration test for SDL+DRM display with evdev touch input
 * This demonstrates the complete solution for touch input on embedded Linux
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#ifdef __APPLE__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include "display/display_backend.h"
#include "input/input_handler.h"
#include "core/logger.h"

static volatile bool quit = false;

void signal_handler(int sig) {
    (void)sig;
    quit = true;
}

int main(int argc, char* argv[]) {
    // Initialize logging
    logger_init(NULL, "test_integrated_touch");
    log_info("=== Integrated Touch Test Starting ===");
    
    // Parse command line
    DisplayBackendType backend_type = DISPLAY_BACKEND_AUTO;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--sdl") == 0) {
            backend_type = DISPLAY_BACKEND_SDL;
        } else if (strcmp(argv[i], "--sdl-drm") == 0) {
            backend_type = DISPLAY_BACKEND_SDL_DRM;
        }
    }
    
    // Initialize display backend
    DisplayConfig display_config = {
        .width = 800,
        .height = 480,
        .title = "Touch Test",
        .backend_type = backend_type,
        .fullscreen = false,
        .vsync = true
    };
    
    DisplayBackend* display = display_backend_create(&display_config);
    if (!display) {
        log_error("Failed to create display backend");
        logger_shutdown();
        return 1;
    }
    
    log_info("Display initialized: %s (%dx%d)", 
             display->name, display->actual_width, display->actual_height);
    
    // Initialize input handler
    InputConfig input_config = {
        .source_type = INPUT_SOURCE_SDL_NATIVE,
        .device_path = NULL,
        .auto_detect_devices = true,
        .enable_mouse_emulation = false
    };
    
    // Use evdev for SDL+DRM backend
    if (display->type == DISPLAY_BACKEND_SDL_DRM) {
        log_info("Using evdev input for SDL+DRM backend");
        input_config.source_type = INPUT_SOURCE_LINUX_EVDEV;
    }
    
    InputHandler* input = input_handler_create(&input_config);
    if (!input) {
        log_error("Failed to create input handler");
        display_backend_destroy(display);
        logger_shutdown();
        return 1;
    }
    
    if (!input_handler_start(input)) {
        log_error("Failed to start input handler");
        input_handler_destroy(input);
        display_backend_destroy(display);
        logger_shutdown();
        return 1;
    }
    
    // Get input capabilities
    InputCapabilities caps;
    if (input_handler_get_capabilities(input, &caps)) {
        log_info("Input capabilities:");
        log_info("  Touch: %s (max %d points)", 
                 caps.has_touch ? "yes" : "no", caps.max_touch_points);
        if (caps.has_touch && caps.touch_x_max > 0) {
            log_info("  Touch range: X[%d-%d] Y[%d-%d]",
                     caps.touch_x_min, caps.touch_x_max,
                     caps.touch_y_min, caps.touch_y_max);
        }
    }
    
    // Setup signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    SDL_Renderer* renderer = display->renderer;
    
    // Touch visualization
    typedef struct {
        bool active;
        float x, y;
        Uint32 color;
    } TouchPoint;
    
    TouchPoint touches[10] = {0};
    Uint32 colors[] = {
        0xFF0000FF, // Red
        0x00FF00FF, // Green
        0x0000FFFF, // Blue
        0xFFFF00FF, // Yellow
        0xFF00FFFF, // Magenta
        0x00FFFFFF, // Cyan
        0xFFFFFFFF, // White
        0xFF8000FF, // Orange
        0x8000FFFF, // Light blue
        0xFF0080FF  // Pink
    };
    
    log_info("Touch the screen to see visualization. Press Ctrl+C to exit.");
    
    // Main loop
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                    
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        quit = true;
                    }
                    break;
                    
                case SDL_FINGERDOWN: {
                    int id = event.tfinger.fingerId % 10;
                    touches[id].active = true;
                    touches[id].x = event.tfinger.x * display->actual_width;
                    touches[id].y = event.tfinger.y * display->actual_height;
                    touches[id].color = colors[id];
                    log_info("Touch %d DOWN at (%.1f, %.1f)", id, touches[id].x, touches[id].y);
                    break;
                }
                    
                case SDL_FINGERUP: {
                    int id = event.tfinger.fingerId % 10;
                    touches[id].active = false;
                    log_info("Touch %d UP", id);
                    break;
                }
                    
                case SDL_FINGERMOTION: {
                    int id = event.tfinger.fingerId % 10;
                    touches[id].x = event.tfinger.x * display->actual_width;
                    touches[id].y = event.tfinger.y * display->actual_height;
                    break;
                }
                    
                case SDL_MOUSEBUTTONDOWN:
                    // Also handle mouse for desktop testing
                    touches[0].active = true;
                    touches[0].x = event.button.x;
                    touches[0].y = event.button.y;
                    touches[0].color = colors[0];
                    log_info("Mouse DOWN at (%d, %d)", event.button.x, event.button.y);
                    break;
                    
                case SDL_MOUSEBUTTONUP:
                    touches[0].active = false;
                    log_info("Mouse UP");
                    break;
                    
                case SDL_MOUSEMOTION:
                    if (touches[0].active) {
                        touches[0].x = event.motion.x;
                        touches[0].y = event.motion.y;
                    }
                    break;
            }
        }
        
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 32, 32, 32, 255);
        SDL_RenderClear(renderer);
        
        // Draw touch points
        for (int i = 0; i < 10; i++) {
            if (touches[i].active) {
                // Extract RGBA
                Uint8 r = (touches[i].color >> 24) & 0xFF;
                Uint8 g = (touches[i].color >> 16) & 0xFF;
                Uint8 b = (touches[i].color >> 8) & 0xFF;
                Uint8 a = touches[i].color & 0xFF;
                
                SDL_SetRenderDrawColor(renderer, r, g, b, a);
                
                // Draw circle (approximated with filled rectangles)
                int radius = 40;
                for (int dy = -radius; dy <= radius; dy++) {
                    for (int dx = -radius; dx <= radius; dx++) {
                        if (dx*dx + dy*dy <= radius*radius) {
                            SDL_RenderDrawPoint(renderer, 
                                              (int)touches[i].x + dx, 
                                              (int)touches[i].y + dy);
                        }
                    }
                }
                
                // Draw touch ID
                SDL_Rect rect = {
                    (int)touches[i].x - 10,
                    (int)touches[i].y - 10,
                    20, 20
                };
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(renderer, &rect);
            }
        }
        
        // Draw info text background
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        SDL_Rect info_bg = {10, 10, 400, 60};
        SDL_RenderFillRect(renderer, &info_bg);
        
        // Present
        SDL_RenderPresent(renderer);
        display_backend_present(display);
        
        SDL_Delay(16); // ~60 FPS
    }
    
    // Show statistics
    const struct InputHandler_stats* stats = input_handler_get_stats(input);
    if (stats) {
        log_info("Input statistics:");
        log_info("  Total events: %llu", (unsigned long long)stats->events_processed);
        log_info("  Touch events: %llu", (unsigned long long)stats->touch_events);
    }
    
    // Cleanup
    log_info("Shutting down...");
    input_handler_destroy(input);
    display_backend_destroy(display);
    logger_shutdown();
    
    return 0;
}