/**
 * Test program for input handler abstraction
 * Tests SDL native, mock, and evdev (Linux only) input sources
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __APPLE__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include "input/input_handler.h"

void test_mock_input() {
    printf("\n=== Testing Mock Input Source ===\n");
    
    InputConfig config = {
        .source_type = INPUT_SOURCE_MOCK,
        .device_path = NULL,
        .auto_detect_devices = false,
        .enable_mouse_emulation = false
    };
    
    InputHandler* handler = input_handler_create(&config);
    if (!handler) {
        printf("Failed to create mock input handler\n");
        return;
    }
    
    if (!input_handler_start(handler)) {
        printf("Failed to start mock input handler\n");
        input_handler_destroy(handler);
        return;
    }
    
    // Get capabilities
    InputCapabilities caps;
    if (input_handler_get_capabilities(handler, &caps)) {
        printf("Mock capabilities:\n");
        printf("  Touch: %s (max %d points)\n", 
               caps.has_touch ? "yes" : "no", caps.max_touch_points);
        printf("  Mouse: %s\n", caps.has_mouse ? "yes" : "no");
        printf("  Keyboard: %s\n", caps.has_keyboard ? "yes" : "no");
    }
    
    // Start a tap pattern
    printf("\nStarting tap pattern...\n");
    input_mock_start_pattern(handler->source, MOCK_PATTERN_TAP);
    
    // Process events for 3 seconds
    Uint32 start = SDL_GetTicks();
    int event_count = 0;
    
    while (SDL_GetTicks() - start < 3000) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_FINGERDOWN:
                    printf("  [%4d ms] FINGER DOWN at (%.2f, %.2f)\n",
                           SDL_GetTicks() - start, event.tfinger.x, event.tfinger.y);
                    event_count++;
                    break;
                case SDL_FINGERUP:
                    printf("  [%4d ms] FINGER UP at (%.2f, %.2f)\n",
                           SDL_GetTicks() - start, event.tfinger.x, event.tfinger.y);
                    event_count++;
                    break;
                case SDL_FINGERMOTION:
                    printf("  [%4d ms] FINGER MOTION at (%.2f, %.2f)\n",
                           SDL_GetTicks() - start, event.tfinger.x, event.tfinger.y);
                    event_count++;
                    break;
            }
        }
        SDL_Delay(10);
    }
    
    input_mock_stop_pattern(handler->source);
    printf("\nReceived %d events in 3 seconds\n", event_count);
    
    // Test other patterns
    printf("\nTesting swipe pattern for 2 seconds...\n");
    input_mock_start_pattern(handler->source, MOCK_PATTERN_SWIPE);
    SDL_Delay(2000);
    input_mock_stop_pattern(handler->source);
    
    // Cleanup
    input_handler_destroy(handler);
    printf("Mock input test complete\n");
}

void test_sdl_native_input() {
    printf("\n=== Testing SDL Native Input Source ===\n");
    
    InputConfig config = {
        .source_type = INPUT_SOURCE_SDL_NATIVE,
        .device_path = NULL,
        .auto_detect_devices = false,
        .enable_mouse_emulation = false
    };
    
    InputHandler* handler = input_handler_create(&config);
    if (!handler) {
        printf("Failed to create SDL native input handler\n");
        return;
    }
    
    if (!input_handler_start(handler)) {
        printf("Failed to start SDL native input handler\n");
        input_handler_destroy(handler);
        return;
    }
    
    // Get capabilities
    InputCapabilities caps;
    if (input_handler_get_capabilities(handler, &caps)) {
        printf("SDL Native capabilities:\n");
        printf("  Touch: %s (max %d points)\n", 
               caps.has_touch ? "yes" : "no", caps.max_touch_points);
        printf("  Mouse: %s\n", caps.has_mouse ? "yes" : "no");
        printf("  Keyboard: %s\n", caps.has_keyboard ? "yes" : "no");
    }
    
    printf("\nMove mouse or touch the screen. Press ESC to continue...\n");
    
    // Process events until ESC
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        done = true;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    printf("  Mouse motion at (%d, %d)\n", event.motion.x, event.motion.y);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    printf("  Mouse button %d down at (%d, %d)\n",
                           event.button.button, event.button.x, event.button.y);
                    break;
                case SDL_FINGERDOWN:
                    printf("  Finger down at (%.2f, %.2f)\n",
                           event.tfinger.x, event.tfinger.y);
                    break;
            }
        }
        SDL_Delay(10);
    }
    
    // Cleanup
    input_handler_destroy(handler);
    printf("SDL native input test complete\n");
}

#ifdef __linux__
void test_evdev_input() {
    printf("\n=== Testing Linux evdev Input Source ===\n");
    
    InputConfig config = {
        .source_type = INPUT_SOURCE_LINUX_EVDEV,
        .device_path = NULL,  // Auto-detect
        .auto_detect_devices = true,
        .enable_mouse_emulation = false
    };
    
    InputHandler* handler = input_handler_create(&config);
    if (!handler) {
        printf("Failed to create evdev input handler\n");
        return;
    }
    
    if (!input_handler_start(handler)) {
        printf("Failed to start evdev input handler\n");
        input_handler_destroy(handler);
        return;
    }
    
    // Get capabilities
    InputCapabilities caps;
    if (input_handler_get_capabilities(handler, &caps)) {
        printf("Evdev capabilities:\n");
        printf("  Touch: %s (max %d points)\n", 
               caps.has_touch ? "yes" : "no", caps.max_touch_points);
        printf("  Touch range: X[%d-%d] Y[%d-%d]\n",
               caps.touch_x_min, caps.touch_x_max,
               caps.touch_y_min, caps.touch_y_max);
    }
    
    printf("\nTouch the screen. Press Ctrl+C to exit...\n");
    
    // Process events for 10 seconds
    Uint32 start = SDL_GetTicks();
    while (SDL_GetTicks() - start < 10000) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_FINGERDOWN:
                    printf("  Touch down at (%.2f, %.2f)\n",
                           event.tfinger.x, event.tfinger.y);
                    break;
                case SDL_FINGERUP:
                    printf("  Touch up at (%.2f, %.2f)\n",
                           event.tfinger.x, event.tfinger.y);
                    break;
                case SDL_FINGERMOTION:
                    printf("  Touch motion at (%.2f, %.2f)\n",
                           event.tfinger.x, event.tfinger.y);
                    break;
            }
        }
        SDL_Delay(10);
    }
    
    // Cleanup
    input_handler_destroy(handler);
    printf("Evdev input test complete\n");
}
#endif

int main(int argc, char* argv[]) {
    printf("Input Handler Test Program\n");
    printf("==========================\n");
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }
    
    // Create a dummy window (required for SDL events)
    SDL_Window* window = SDL_CreateWindow("Input Test",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          640, 480,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Test mock input (always available)
    test_mock_input();
    
    // Test SDL native input
    test_sdl_native_input();
    
#ifdef __linux__
    // Test evdev input (Linux only)
    if (argc > 1 && strcmp(argv[1], "--evdev") == 0) {
        test_evdev_input();
    } else {
        printf("\nSkipping evdev test. Run with --evdev to test Linux evdev input.\n");
    }
#endif
    
    // Cleanup
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    printf("\nAll tests complete!\n");
    return 0;
}