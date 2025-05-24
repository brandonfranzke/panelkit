#include <SDL2/SDL.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

static volatile int keep_running = 1;

void signal_handler(int sig) {
    keep_running = 0;
}

void test_configuration(const char* video_driver, const char* description) {
    printf("\n=== Testing: %s ===\n", description);
    
    setenv("SDL_VIDEODRIVER", video_driver, 1);
    SDL_SetHint(SDL_HINT_VIDEODRIVER, video_driver);
    
    // Try various input-related hints
    SDL_SetHint(SDL_HINT_LINUX_JOYSTICK_DEADZONES, "0");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    
    // Set DISPLAY environment variable in case it helps
    setenv("DISPLAY", ":0", 1);
    
    printf("Initializing SDL...\n");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return;
    }
    
    printf("Video driver: %s\n", SDL_GetCurrentVideoDriver());
    printf("Touch devices: %d\n", SDL_GetNumTouchDevices());
    
    SDL_Window* window = SDL_CreateWindow(
        "Test",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        480, 640,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }
    
    // Test for a few seconds
    SDL_Event event;
    int events_received = 0;
    Uint32 start_time = SDL_GetTicks();
    
    printf("Waiting for events (3 seconds)...\n");
    while (SDL_GetTicks() - start_time < 3000) {
        while (SDL_PollEvent(&event)) {
            events_received++;
            if (event.type == SDL_FINGERDOWN || event.type == SDL_FINGERUP || 
                event.type == SDL_FINGERMOTION || event.type == SDL_MOUSEBUTTONDOWN ||
                event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEMOTION) {
                printf("  INPUT EVENT RECEIVED! Type: %d\n", event.type);
            }
        }
        usleep(10000);
    }
    
    printf("Total events: %d\n", events_received);
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main() {
    printf("SDL Input Configuration Test\n");
    printf("============================\n");
    printf("This test tries different SDL configurations to find one that receives input.\n");
    printf("Touch the screen during each 3-second test period.\n");
    
    signal(SIGINT, signal_handler);
    
    // Test 1: Dummy driver with hints
    test_configuration("dummy", "Dummy driver with input hints");
    
    // Test 2: X11 driver (might fail on headless)
    test_configuration("x11", "X11 driver");
    
    // Test 3: Wayland driver (might fail on headless)  
    test_configuration("wayland", "Wayland driver");
    
    // Test 4: DirectFB driver
    test_configuration("directfb", "DirectFB driver");
    
    // Test 5: KMSDRM driver (requires permissions)
    test_configuration("kmsdrm", "KMSDRM driver");
    
    // Test 6: No driver specified (SDL chooses)
    unsetenv("SDL_VIDEODRIVER");
    SDL_SetHint(SDL_HINT_VIDEODRIVER, NULL);
    test_configuration("", "Default driver selection");
    
    printf("\n=== Test Complete ===\n");
    printf("If any configuration showed 'INPUT EVENT RECEIVED', that's the one to use.\n");
    
    return 0;
}