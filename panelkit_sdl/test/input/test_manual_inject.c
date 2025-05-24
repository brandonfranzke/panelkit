#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <pthread.h>
#include <signal.h>

static volatile int keep_running = 1;
static int touch_fd = -1;

// Touch state tracking
static int touch_active = 0;
static int touch_x = 0;
static int touch_y = 0;

void signal_handler(int sig) {
    keep_running = 0;
}

void* input_thread(void* arg) {
    SDL_Window* window = (SDL_Window*)arg;
    struct input_event ev;
    
    // Open the touch device
    touch_fd = open("/dev/input/event4", O_RDONLY);
    if (touch_fd < 0) {
        perror("Failed to open touch device");
        return NULL;
    }
    
    printf("Touch input thread started, reading from /dev/input/event4\n");
    
    while (keep_running) {
        if (read(touch_fd, &ev, sizeof(ev)) == sizeof(ev)) {
            if (ev.type == EV_ABS) {
                switch (ev.code) {
                    case ABS_MT_POSITION_X:
                        touch_x = ev.value;
                        break;
                    case ABS_MT_POSITION_Y:
                        touch_y = ev.value;
                        break;
                    case ABS_MT_TRACKING_ID:
                        if (ev.value == -1) {
                            // Touch up - push SDL event
                            touch_active = 0;
                            SDL_Event sdl_event;
                            sdl_event.type = SDL_FINGERUP;
                            sdl_event.tfinger.touchId = 1;
                            sdl_event.tfinger.fingerId = 0;
                            sdl_event.tfinger.x = touch_x / 480.0f; // Normalize
                            sdl_event.tfinger.y = touch_y / 640.0f;
                            sdl_event.tfinger.pressure = 0.0f;
                            SDL_PushEvent(&sdl_event);
                            printf("[Manual] Injected FINGER_UP at %d,%d\n", touch_x, touch_y);
                        } else {
                            // Touch down - push SDL event
                            touch_active = 1;
                            SDL_Event sdl_event;
                            sdl_event.type = SDL_FINGERDOWN;
                            sdl_event.tfinger.touchId = 1;
                            sdl_event.tfinger.fingerId = 0;
                            sdl_event.tfinger.x = touch_x / 480.0f; // Normalize
                            sdl_event.tfinger.y = touch_y / 640.0f;
                            sdl_event.tfinger.pressure = 1.0f;
                            SDL_PushEvent(&sdl_event);
                            printf("[Manual] Injected FINGER_DOWN at %d,%d\n", touch_x, touch_y);
                        }
                        break;
                }
            } else if (ev.type == EV_SYN && ev.code == SYN_REPORT && touch_active) {
                // Motion event - push SDL event
                SDL_Event sdl_event;
                sdl_event.type = SDL_FINGERMOTION;
                sdl_event.tfinger.touchId = 1;
                sdl_event.tfinger.fingerId = 0;
                sdl_event.tfinger.x = touch_x / 480.0f; // Normalize
                sdl_event.tfinger.y = touch_y / 640.0f;
                sdl_event.tfinger.dx = 0; // Would need to track previous position
                sdl_event.tfinger.dy = 0;
                sdl_event.tfinger.pressure = 1.0f;
                SDL_PushEvent(&sdl_event);
            }
        }
    }
    
    if (touch_fd >= 0) {
        close(touch_fd);
    }
    
    return NULL;
}

int main() {
    printf("SDL Manual Input Injection Test\n");
    printf("===============================\n");
    printf("This test reads touch events from /dev/input/event4 and injects them into SDL.\n\n");
    
    signal(SIGINT, signal_handler);
    
    // Use offscreen driver like PanelKit
    setenv("SDL_VIDEODRIVER", "offscreen", 1);
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "offscreen");
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    
    printf("SDL initialized with %s driver\n", SDL_GetCurrentVideoDriver());
    printf("Touch devices detected by SDL: %d\n", SDL_GetNumTouchDevices());
    
    SDL_Window* window = SDL_CreateWindow(
        "Manual Input Test",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        480, 640,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Start input thread
    pthread_t input_thread_id;
    if (pthread_create(&input_thread_id, NULL, input_thread, window) != 0) {
        printf("Failed to create input thread\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    printf("\nTouch the screen to test. Press Ctrl+C to exit.\n");
    printf("SDL event loop running...\n\n");
    
    SDL_Event event;
    int finger_events = 0;
    int other_events = 0;
    
    while (keep_running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_FINGERDOWN:
                    finger_events++;
                    printf("[SDL] FINGER_DOWN received: x=%.3f, y=%.3f\n", 
                           event.tfinger.x, event.tfinger.y);
                    break;
                
                case SDL_FINGERUP:
                    finger_events++;
                    printf("[SDL] FINGER_UP received: x=%.3f, y=%.3f\n", 
                           event.tfinger.x, event.tfinger.y);
                    break;
                
                case SDL_FINGERMOTION:
                    finger_events++;
                    // Don't spam motion events
                    break;
                
                case SDL_QUIT:
                    keep_running = 0;
                    break;
                
                default:
                    other_events++;
                    if (event.type != 512) { // Skip display events
                        printf("[SDL] Other event: type=%d\n", event.type);
                    }
                    break;
            }
        }
        
        usleep(10000); // 10ms
    }
    
    printf("\nShutting down...\n");
    
    // Wait for input thread
    pthread_join(input_thread_id, NULL);
    
    printf("\n=== Summary ===\n");
    printf("Finger events processed: %d\n", finger_events);
    printf("Other events: %d\n", other_events);
    
    if (finger_events > 0) {
        printf("✓ SUCCESS: Manual input injection works!\n");
        printf("  This confirms SDL can process touch events when injected manually.\n");
        printf("  The issue is that SDL's input subsystem isn't reading from Linux input devices.\n");
    } else {
        printf("✗ Manual injection didn't work - deeper SDL issues\n");
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}