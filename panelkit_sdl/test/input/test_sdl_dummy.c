#include <SDL.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

static volatile int keep_running = 1;

void signal_handler(int sig) {
    keep_running = 0;
}

int main() {
    printf("PanelKit Touch Test - SDL Dummy Driver\n");
    printf("=====================================\n\n");

    signal(SIGINT, signal_handler);

    // Use dummy video driver instead of offscreen
    setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "dummy");
    
    printf("Initializing SDL with dummy driver...\n");
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

    SDL_Window* window = SDL_CreateWindow(
        "Touch Test",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        480, 640,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    printf("Window created: 480x640\n");

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

    while (keep_running) {
        while (SDL_PollEvent(&event)) {
            event_count++;
            
            switch (event.type) {
                case SDL_FINGERDOWN:
                    finger_events++;
                    printf("[%d] FINGER_DOWN: x=%.3f, y=%.3f, pressure=%.3f\n", 
                           event_count, event.tfinger.x, event.tfinger.y, event.tfinger.pressure);
                    break;
                
                case SDL_FINGERUP:
                    finger_events++;
                    printf("[%d] FINGER_UP: x=%.3f, y=%.3f, pressure=%.3f\n", 
                           event_count, event.tfinger.x, event.tfinger.y, event.tfinger.pressure);
                    break;
                
                case SDL_FINGERMOTION:
                    finger_events++;
                    printf("[%d] FINGER_MOTION: x=%.3f, y=%.3f, pressure=%.3f\n", 
                           event_count, event.tfinger.x, event.tfinger.y, event.tfinger.pressure);
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
                    printf("[%d] MOUSE_MOTION: x=%d, y=%d\n", 
                           event_count, event.motion.x, event.motion.y);
                    break;
                
                case SDL_QUIT:
                    keep_running = 0;
                    break;
                
                default:
                    other_events++;
                    printf("[%d] Other event: type=%d\n", event_count, event.type);
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
    if (finger_events > 0 || mouse_events > 0) {
        printf("✓ SDL receiving input events with dummy driver\n");
    } else {
        printf("✗ No touch or mouse events detected - SDL input issue persists\n");
    }
    
    return 0;
}