#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include "ui/page_manager.h"
#include "state/state_store.h"
#include "events/event_system.h"
#include "config/config_manager.h"
// #include "ui/gestures.h" // Not using gestures for this test

// Simple logging
#define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)

#define WINDOW_WIDTH 480
#define WINDOW_HEIGHT 640

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_ERROR("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }
    
    if (TTF_Init() < 0) {
        LOG_ERROR("SDL_ttf initialization failed: %s", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    SDL_Window* window = SDL_CreateWindow("Page System Test",
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_SHOWN);
    if (!window) {
        LOG_ERROR("Window creation failed: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 
                                               SDL_RENDERER_ACCELERATED | 
                                               SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        LOG_ERROR("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Create systems
    StateStore* state_store = state_store_create();
    EventSystem* event_system = event_system_create();
    ConfigManager* config_manager = NULL;  // Not using config for this test
    
    // Create page manager
    PageManager* page_manager = page_manager_create(renderer, event_system, 
                                                  state_store, config_manager);
    if (!page_manager) {
        LOG_ERROR("Failed to create page manager");
        event_system_destroy(event_system);
        state_store_destroy(state_store);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Set dimensions
    page_manager_set_dimensions(page_manager, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    // Initialize pages
    if (!page_manager_init_pages(page_manager)) {
        LOG_ERROR("Failed to initialize pages");
        page_manager_destroy(page_manager);
        event_system_destroy(event_system);
        state_store_destroy(state_store);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    LOG_INFO("Page System Test Started");
    LOG_INFO("- Use arrow keys to change pages");
    LOG_INFO("- Click buttons to interact");
    LOG_INFO("- ESC to quit");
    
    // Simulate initial weather data
    typedef struct {
        float temperature;
        float humidity;
        char location[64];
        char description[128];
        time_t timestamp;
    } WeatherData;
    
    WeatherData initial_weather = {
        .temperature = 75.0f,
        .humidity = 60.0f,
        .timestamp = time(NULL)
    };
    strcpy(initial_weather.location, "New York");
    strcpy(initial_weather.description, "Clear");
    event_publish(event_system, "weather.current", &initial_weather, sizeof(initial_weather));
    
    bool running = true;
    SDL_Event event;
    
    // Touch tracking
    bool is_dragging = false;
    float drag_start_x = 0;
    float drag_offset = 0;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else if (event.key.keysym.sym == SDLK_LEFT) {
                    // Previous page
                    int current = page_manager_get_current_page(page_manager);
                    if (current > 0) {
                        page_manager_goto_page(page_manager, current - 1);
                    }
                } else if (event.key.keysym.sym == SDLK_RIGHT) {
                    // Next page
                    int current = page_manager_get_current_page(page_manager);
                    if (current < 1) {  // We have 2 pages (0 and 1)
                        page_manager_goto_page(page_manager, current + 1);
                    }
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                is_dragging = true;
                drag_start_x = event.button.x;
                drag_offset = 0;
            } else if (event.type == SDL_MOUSEMOTION && is_dragging) {
                drag_offset = event.motion.x - drag_start_x;
                page_manager_handle_drag(page_manager, drag_offset, false);
            } else if (event.type == SDL_MOUSEBUTTONUP) {
                if (is_dragging) {
                    page_manager_handle_drag(page_manager, drag_offset, true);
                    is_dragging = false;
                    drag_offset = 0;
                }
            }
            
            // Let page manager handle events
            page_manager_handle_event(page_manager, &event);
        }
        
        // Update
        page_manager_update(page_manager);
        
        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        page_manager_render(page_manager);
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);  // ~60 FPS
    }
    
    LOG_INFO("Shutting down Page System Test");
    
    // Cleanup
    page_manager_destroy(page_manager);
    event_system_destroy(event_system);
    state_store_destroy(state_store);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}