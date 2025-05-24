/**
 * @file test_touch_minimal.c
 * @brief Minimal test that mimics PanelKit's SDL setup to isolate touch issues
 * 
 * This test uses the same SDL configuration as the main PanelKit app:
 * - Offscreen video driver
 * - Software rendering to SDL surface
 * - Same window dimensions
 * 
 * This helps isolate whether touch issues are specific to the SDL+DRM
 * backend or are general SDL configuration problems.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

// Minimal SDL includes - we don't need the full SDL2 stack
#include <SDL.h>

static volatile int running = 1;

void signal_handler(int sig) {
    (void)sig;
    running = 0;
    printf("\nShutting down...\n");
}

void print_sdl_info() {
    printf("=== SDL Configuration ===\n");
    printf("Video driver: %s\n", SDL_GetCurrentVideoDriver());
    
    int num_displays = SDL_GetNumVideoDisplays();
    printf("Number of displays: %d\n", num_displays);
    
    for (int i = 0; i < num_displays; i++) {
        SDL_DisplayMode mode;
        if (SDL_GetCurrentDisplayMode(i, &mode) == 0) {
            printf("  Display %d: %dx%d @ %dHz\n", i, mode.w, mode.h, mode.refresh_rate);
        }
    }
    
    int num_touch_devices = SDL_GetNumTouchDevices();
    printf("Touch devices detected by SDL: %d\n", num_touch_devices);
    
    for (int i = 0; i < num_touch_devices; i++) {
        SDL_TouchID touch_id = SDL_GetTouchDevice(i);
        printf("  Touch device %d: ID = %lld\n", i, (long long)touch_id);
    }
    
    printf("========================\n\n");
}

void print_event_summary(int finger_events, int mouse_events, int other_events) {
    printf("\n=== Event Summary ===\n");
    printf("Finger events: %d\n", finger_events);
    printf("Mouse events:  %d\n", mouse_events);
    printf("Other events:  %d\n", other_events);
    printf("===================\n");
}

int main() {
    printf("PanelKit Touch Test - Minimal SDL Setup\n");
    printf("=======================================\n\n");
    
    // Setup signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Set SDL video driver to offscreen (same as main app)
    setenv("SDL_VIDEODRIVER", "offscreen", 1);
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "offscreen");
    
    printf("Initializing SDL with offscreen driver...\n");
    
    // Initialize SDL (same as main app)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    
    printf("SDL initialized successfully\n");
    
    // Print SDL configuration info
    print_sdl_info();
    
    // Create window with same dimensions as main app
    SDL_Window* window = SDL_CreateWindow(
        "Touch Test",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        480, 640,  // Same as main app
        0
    );
    
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    printf("Window created: 480x640\n");
    
    // Create renderer (software rendering like main app)
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    printf("Software renderer created\n");
    printf("\nTouch the screen to test input. Press Ctrl+C to exit.\n");
    printf("Monitoring SDL events...\n\n");
    
    // Event counters
    int finger_events = 0;
    int mouse_events = 0; 
    int other_events = 0;
    int total_events = 0;
    
    SDL_Event e;
    
    // Clear screen to black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    
    while (running) {
        while (SDL_PollEvent(&e)) {
            total_events++;
            
            switch (e.type) {
                case SDL_FINGERDOWN:
                    finger_events++;
                    printf("[%d] FINGER DOWN: x=%.3f, y=%.3f, pressure=%.3f\n",
                           total_events, e.tfinger.x, e.tfinger.y, e.tfinger.pressure);
                    
                    // Draw a white circle where touched
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    int pixel_x = (int)(e.tfinger.x * 480);
                    int pixel_y = (int)(e.tfinger.y * 640);
                    
                    // Draw a simple cross at touch point
                    SDL_RenderDrawLine(renderer, pixel_x - 10, pixel_y, pixel_x + 10, pixel_y);
                    SDL_RenderDrawLine(renderer, pixel_x, pixel_y - 10, pixel_x, pixel_y + 10);
                    SDL_RenderPresent(renderer);
                    break;
                    
                case SDL_FINGERUP:
                    finger_events++;
                    printf("[%d] FINGER UP: x=%.3f, y=%.3f\n",
                           total_events, e.tfinger.x, e.tfinger.y);
                    break;
                    
                case SDL_FINGERMOTION:
                    finger_events++;
                    printf("[%d] FINGER MOTION: x=%.3f, y=%.3f, dx=%.3f, dy=%.3f\n",
                           total_events, e.tfinger.x, e.tfinger.y, e.tfinger.dx, e.tfinger.dy);
                    break;
                    
                case SDL_MOUSEBUTTONDOWN:
                    mouse_events++;
                    printf("[%d] MOUSE DOWN: button=%d, x=%d, y=%d\n",
                           total_events, e.button.button, e.button.x, e.button.y);
                    
                    // Draw a red circle for mouse events
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                    SDL_RenderDrawLine(renderer, e.button.x - 5, e.button.y, e.button.x + 5, e.button.y);
                    SDL_RenderDrawLine(renderer, e.button.x, e.button.y - 5, e.button.x, e.button.y + 5);
                    SDL_RenderPresent(renderer);
                    break;
                    
                case SDL_MOUSEBUTTONUP:
                    mouse_events++;
                    printf("[%d] MOUSE UP: button=%d, x=%d, y=%d\n",
                           total_events, e.button.button, e.button.x, e.button.y);
                    break;
                    
                case SDL_MOUSEMOTION:
                    if (e.motion.state != 0) {  // Only when button pressed
                        mouse_events++;
                        printf("[%d] MOUSE MOTION: x=%d, y=%d\n",
                               total_events, e.motion.x, e.motion.y);
                    }
                    break;
                    
                case SDL_QUIT:
                    printf("[%d] QUIT event\n", total_events);
                    running = 0;
                    break;
                    
                default:
                    other_events++;
                    if (other_events <= 5) {  // Only show first few to avoid spam
                        printf("[%d] Other event: type=%d\n", total_events, e.type);
                    }
                    break;
            }
        }
        
        // Print periodic status
        static int last_status_time = 0;
        int current_time = SDL_GetTicks();
        if (current_time - last_status_time > 5000) {  // Every 5 seconds
            printf("Status: %d finger, %d mouse, %d other events\n", 
                   finger_events, mouse_events, other_events);
            last_status_time = current_time;
        }
        
        SDL_Delay(10);  // Small delay
    }
    
    print_event_summary(finger_events, mouse_events, other_events);
    
    printf("\nCleaning up...\n");
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    printf("Test completed. Total events: %d\n", total_events);
    
    // Print analysis
    printf("\nAnalysis:\n");
    if (finger_events > 0) {
        printf("✓ SDL finger/touch events are working!\n");
    } else if (mouse_events > 0) {
        printf("⚠ Only mouse events detected - touch may be converted to mouse\n");
    } else {
        printf("✗ No touch or mouse events detected - SDL input issue\n");
    }
    
    return 0;
}