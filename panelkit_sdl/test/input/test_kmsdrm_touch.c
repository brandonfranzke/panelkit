#include <SDL2/SDL.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

static volatile int keep_running = 1;

void signal_handler(int sig) {
    (void)sig;
    keep_running = 0;
}

int main() {
    printf("PanelKit Touch Test - KMSDRM Driver\n");
    printf("====================================\n\n");

    signal(SIGINT, signal_handler);

    // Use KMSDRM video driver
    setenv("SDL_VIDEODRIVER", "kmsdrm", 1);
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "kmsdrm");
    
    // Additional hints for better input handling
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    
    printf("Initializing SDL with KMSDRM driver...\n");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    printf("SDL initialized successfully\n");

    printf("=== SDL Configuration ===\n");
    printf("Video driver: %s\n", SDL_GetCurrentVideoDriver());
    printf("Number of displays: %d\n", SDL_GetNumVideoDisplays());
    for (int i = 0; i < SDL_GetNumVideoDisplays(); i++) {
        SDL_DisplayMode mode;
        SDL_GetCurrentDisplayMode(i, &mode);
        printf("  Display %d: %dx%d @ %dHz\n", i, mode.w, mode.h, mode.refresh_rate);
    }
    
    int num_touch_devices = SDL_GetNumTouchDevices();
    printf("Touch devices detected by SDL: %d\n", num_touch_devices);
    for (int i = 0; i < num_touch_devices; i++) {
        SDL_TouchID touch_id = SDL_GetTouchDevice(i);
        printf("  Touch device %d: ID = %lld\n", i, (long long)touch_id);
    }
    printf("========================\n\n");

    // Create window using desktop dimensions
    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    
    SDL_Window* window = SDL_CreateWindow(
        "KMSDRM Touch Test",
        0, 0,  // Position at 0,0
        dm.w, dm.h,  // Use full display dimensions
        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP
    );
    
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    printf("Window created: %dx%d (fullscreen)\n", dm.w, dm.h);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    printf("Software renderer created\n\n");

    printf("Touch the screen to test input. Press Ctrl+C to exit.\n");
    printf("Monitoring SDL events...\n\n");

    SDL_Event event;
    int finger_events = 0;
    int mouse_events = 0;
    int other_events = 0;
    int event_count = 0;

    // Clear screen to blue to show it's working
    SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    while (keep_running) {
        while (SDL_PollEvent(&event)) {
            event_count++;
            
            switch (event.type) {
                case SDL_FINGERDOWN:
                    finger_events++;
                    printf("[%d] FINGER_DOWN: x=%.3f, y=%.3f, pressure=%.3f, finger=%lld\n", 
                           event_count, event.tfinger.x, event.tfinger.y, 
                           event.tfinger.pressure, (long long)event.tfinger.fingerId);
                    // Draw a circle at touch point
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                    SDL_Rect rect = {
                        (int)(event.tfinger.x * dm.w) - 10,
                        (int)(event.tfinger.y * dm.h) - 10,
                        20, 20
                    };
                    SDL_RenderFillRect(renderer, &rect);
                    SDL_RenderPresent(renderer);
                    break;
                
                case SDL_FINGERUP:
                    finger_events++;
                    printf("[%d] FINGER_UP: x=%.3f, y=%.3f, finger=%lld\n", 
                           event_count, event.tfinger.x, event.tfinger.y,
                           (long long)event.tfinger.fingerId);
                    // Clear screen back to blue
                    SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255);
                    SDL_RenderClear(renderer);
                    SDL_RenderPresent(renderer);
                    break;
                
                case SDL_FINGERMOTION:
                    finger_events++;
                    // Don't spam motion events
                    if (finger_events % 10 == 0) {
                        printf("[%d] FINGER_MOTION: x=%.3f, y=%.3f\n", 
                               event_count, event.tfinger.x, event.tfinger.y);
                    }
                    break;
                
                case SDL_MOUSEBUTTONDOWN:
                    mouse_events++;
                    printf("[%d] MOUSE_DOWN: button=%d, x=%d, y=%d\n", 
                           event_count, event.button.button, event.button.x, event.button.y);
                    break;
                
                case SDL_MOUSEBUTTONUP:
                    mouse_events++;
                    printf("[%d] MOUSE_UP: button=%d, x=%d, y=%d\n", 
                           event_count, event.button.button, event.button.x, event.button.y);
                    break;
                
                case SDL_MOUSEMOTION:
                    mouse_events++;
                    // Don't spam mouse motion
                    break;
                
                case SDL_QUIT:
                    keep_running = 0;
                    break;
                
                default:
                    other_events++;
                    if (event.type != 512) { // Skip display events
                        printf("[%d] Other event: type=%d\n", event_count, event.type);
                    }
                    break;
            }
        }
        
        if (event_count % 100 == 0 && event_count > 0) {
            printf("Status: %d finger, %d mouse, %d other events\n", 
                   finger_events, mouse_events, other_events);
        }
        
        usleep(10000); // 10ms delay
    }

    printf("\nShutting down...\n\n");
    printf("=== Event Summary ===\n");
    printf("Finger events: %d\n", finger_events);
    printf("Mouse events:  %d\n", mouse_events);
    printf("Other events:  %d\n", other_events);
    printf("===================\n\n");

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    printf("Cleaning up...\n");
    printf("Test completed. Total events: %d\n\n", event_count);
    
    printf("Analysis:\n");
    if (finger_events > 0) {
        printf("✓ Touch input working correctly with KMSDRM driver!\n");
        printf("  Recommendation: Modify PanelKit to use KMSDRM instead of offscreen\n");
    } else if (mouse_events > 0) {
        printf("✓ Mouse events detected (touch may be emulating mouse)\n");
    } else {
        printf("✗ No touch or mouse events detected\n");
    }
    
    return 0;
}