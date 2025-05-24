/**
 * @file test_sdl_touch.c
 * @brief Test SDL touch input handling specifically
 * 
 * This test helps isolate SDL touch input issues by testing SDL's
 * event handling in isolation from the main application.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <SDL2/SDL.h>

static volatile int running = 1;

void signal_handler(int sig) {
    (void)sig;
    running = 0;
    printf("\nShutting down...\n");
}

void print_sdl_touch_info() {
    int num_touch_devices = SDL_GetNumTouchDevices();
    printf("SDL Touch Devices: %d\n", num_touch_devices);
    
    for (int i = 0; i < num_touch_devices; i++) {
        SDL_TouchID touch_id = SDL_GetTouchDevice(i);
        printf("  Touch Device %d: ID = %lld\n", i, (long long)touch_id);
        
        int num_fingers = SDL_GetNumTouchFingers(touch_id);
        printf("    Active fingers: %d\n", num_fingers);
    }
    printf("\n");
}

void print_mouse_info() {
    int mouse_x, mouse_y;
    Uint32 mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);
    
    printf("Mouse state: x=%d, y=%d, buttons=0x%x\n", mouse_x, mouse_y, mouse_state);
    printf("  Left: %s, Middle: %s, Right: %s\n",
           (mouse_state & SDL_BUTTON_LMASK) ? "DOWN" : "UP",
           (mouse_state & SDL_BUTTON_MMASK) ? "DOWN" : "UP", 
           (mouse_state & SDL_BUTTON_RMASK) ? "DOWN" : "UP");
}

const char* get_event_name(Uint32 type) {
    switch (type) {
        case SDL_FINGERDOWN: return "FINGERDOWN";
        case SDL_FINGERUP: return "FINGERUP";
        case SDL_FINGERMOTION: return "FINGERMOTION";
        case SDL_MOUSEBUTTONDOWN: return "MOUSEBUTTONDOWN";
        case SDL_MOUSEBUTTONUP: return "MOUSEBUTTONUP";
        case SDL_MOUSEMOTION: return "MOUSEMOTION";
        case SDL_KEYDOWN: return "KEYDOWN";
        case SDL_KEYUP: return "KEYUP";
        case SDL_QUIT: return "QUIT";
        default: return "OTHER";
    }
}

int main(int argc, char* argv[]) {
    printf("PanelKit SDL Touch Input Test\n");
    printf("=============================\n\n");
    
    // Parse arguments for backend selection
    const char* video_driver = NULL;
    if (argc > 1) {
        if (strcmp(argv[1], "--offscreen") == 0) {
            video_driver = "offscreen";
        } else if (strcmp(argv[1], "--dummy") == 0) {
            video_driver = "dummy";
        }
    }
    
    if (video_driver) {
        SDL_SetHint(SDL_HINT_VIDEODRIVER, video_driver);
        printf("Using SDL video driver: %s\n", video_driver);
    }
    
    // Setup signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    
    printf("SDL initialized successfully\n");
    printf("Video driver: %s\n", SDL_GetCurrentVideoDriver());
    
    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "SDL Touch Test",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        480, 640,  // Portrait mode to match our setup
        0
    );
    
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    printf("Window created: 480x640\n");
    
    // Print initial touch device info
    print_sdl_touch_info();
    
    printf("Touch/click anywhere in the window. Press 'q' or Ctrl+C to exit.\n");
    printf("Monitoring SDL events...\n\n");
    
    SDL_Event event;
    int event_count = 0;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            event_count++;
            
            switch (event.type) {
                case SDL_FINGERDOWN:
                    printf("[%d] FINGERDOWN: touchId=%lld, fingerId=%lld, x=%.3f, y=%.3f, pressure=%.3f\n",
                           event_count, (long long)event.tfinger.touchId, 
                           (long long)event.tfinger.fingerId,
                           event.tfinger.x, event.tfinger.y, event.tfinger.pressure);
                    break;
                    
                case SDL_FINGERUP:
                    printf("[%d] FINGERUP: touchId=%lld, fingerId=%lld, x=%.3f, y=%.3f\n",
                           event_count, (long long)event.tfinger.touchId,
                           (long long)event.tfinger.fingerId,
                           event.tfinger.x, event.tfinger.y);
                    break;
                    
                case SDL_FINGERMOTION:
                    printf("[%d] FINGERMOTION: touchId=%lld, fingerId=%lld, x=%.3f, y=%.3f, dx=%.3f, dy=%.3f\n",
                           event_count, (long long)event.tfinger.touchId,
                           (long long)event.tfinger.fingerId,
                           event.tfinger.x, event.tfinger.y,
                           event.tfinger.dx, event.tfinger.dy);
                    break;
                    
                case SDL_MOUSEBUTTONDOWN:
                    printf("[%d] MOUSEBUTTONDOWN: button=%d, x=%d, y=%d, clicks=%d\n",
                           event_count, event.button.button, 
                           event.button.x, event.button.y, event.button.clicks);
                    break;
                    
                case SDL_MOUSEBUTTONUP:
                    printf("[%d] MOUSEBUTTONUP: button=%d, x=%d, y=%d\n",
                           event_count, event.button.button,
                           event.button.x, event.button.y);
                    break;
                    
                case SDL_MOUSEMOTION:
                    if (event.motion.state != 0) {  // Only show when button pressed
                        printf("[%d] MOUSEMOTION: x=%d, y=%d, rel=(%d,%d), state=0x%x\n",
                               event_count, event.motion.x, event.motion.y,
                               event.motion.xrel, event.motion.yrel, event.motion.state);
                    }
                    break;
                    
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_q) {
                        printf("Quit key pressed\n");
                        running = 0;
                    } else {
                        printf("[%d] KEYDOWN: key=%s\n", event_count, SDL_GetKeyName(event.key.keysym.sym));
                    }
                    break;
                    
                case SDL_QUIT:
                    printf("[%d] QUIT event received\n", event_count);
                    running = 0;
                    break;
                    
                default:
                    // Uncomment to see all events
                    // printf("[%d] %s (type=%d)\n", event_count, get_event_name(event.type), event.type);
                    break;
            }
        }
        
        SDL_Delay(10);  // Small delay to prevent busy waiting
    }
    
    printf("\nFinal status:\n");
    print_sdl_touch_info();
    print_mouse_info();
    
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    printf("SDL touch test completed. Total events processed: %d\n", event_count);
    return 0;
}