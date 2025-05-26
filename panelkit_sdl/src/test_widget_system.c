#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include "ui/widget.h"
#include "ui/widget_manager.h"
#include "ui/widget_factory.h"
#include "ui/widgets/button_widget.h"
#include "ui/widgets/weather_widget.h"
#include "state/state_store.h"
#include "events/event_system.h"

// Simple logging
#define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

// Button callback for testing
static void on_refresh_clicked(ButtonWidget* button, void* user_data) {
    EventSystem* events = (EventSystem*)user_data;
    LOG_INFO("Refresh button clicked!");
    
    // Simulate weather data update
    WeatherData weather = {
        .temperature = 68.0f + (rand() % 20),
        .humidity = 40.0f + (rand() % 40),
        .timestamp = time(NULL)
    };
    strcpy(weather.location, "Test City");
    strcpy(weather.description, "Partly Cloudy");
    
    event_publish(events, "weather.current", &weather, sizeof(weather));
}

static void on_toggle_clicked(ButtonWidget* button, void* user_data) {
    WeatherWidget* weather = (WeatherWidget*)user_data;
    static bool use_celsius = false;
    
    use_celsius = !use_celsius;
    weather_widget_set_units(weather, use_celsius);
    button_widget_set_label(button, use_celsius ? "Show °F" : "Show °C");
    
    LOG_INFO("Temperature units toggled");
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_ERROR("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }
    
    if (TTF_Init() < 0) {
        LOG_ERROR("SDL_ttf initialization failed: %s", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    
    SDL_Window* window = SDL_CreateWindow("Widget System Test",
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
    
    // Create widget manager
    WidgetManager* widget_manager = widget_manager_create(renderer, event_system, state_store);
    if (!widget_manager) {
        LOG_ERROR("Failed to create widget manager");
        event_system_destroy(event_system);
        state_store_destroy(state_store);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Create widget factory
    WidgetFactory* factory = widget_factory_create_default();
    
    // Create main container
    Widget* main_container = widget_factory_create_widget(factory, "container", 
                                                        "main_container", NULL);
    widget_set_bounds(main_container, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    main_container->background_color = (SDL_Color){240, 240, 240, 255};
    
    // Create weather widget
    WeatherParams weather_params = {"Test City"};
    WeatherWidget* weather_widget = (WeatherWidget*)widget_factory_create_widget(
        factory, "weather", "weather1", &weather_params);
    widget_set_relative_bounds(&weather_widget->base, 50, 50, 300, 200);
    
    // Create refresh button
    ButtonParams refresh_params = {"Refresh Weather"};
    ButtonWidget* refresh_button = (ButtonWidget*)widget_factory_create_widget(
        factory, "button", "refresh_btn", &refresh_params);
    widget_set_relative_bounds(&refresh_button->base, 50, 270, 150, 40);
    button_widget_set_click_callback(refresh_button, on_refresh_clicked, event_system);
    
    // Create toggle units button
    ButtonParams toggle_params = {"Show °C"};
    ButtonWidget* toggle_button = (ButtonWidget*)widget_factory_create_widget(
        factory, "button", "toggle_btn", &toggle_params);
    widget_set_relative_bounds(&toggle_button->base, 220, 270, 130, 40);
    button_widget_set_click_callback(toggle_button, on_toggle_clicked, weather_widget);
    
    // Create info label
    LabelParams info_params = {"Click Refresh to update weather data"};
    Widget* info_label = widget_factory_create_widget(
        factory, "label", "info_label", &info_params);
    widget_set_relative_bounds(info_label, 50, 330, 300, 30);
    
    // Build widget hierarchy
    widget_add_child(main_container, &weather_widget->base);
    widget_add_child(main_container, &refresh_button->base);
    widget_add_child(main_container, &toggle_button->base);
    widget_add_child(main_container, info_label);
    
    // Add root to manager
    widget_manager_add_root(widget_manager, main_container, "main");
    
    LOG_INFO("Widget System Test Started");
    LOG_INFO("- Click 'Refresh Weather' to simulate weather updates");
    LOG_INFO("- Click temperature toggle to switch between °F and °C");
    LOG_INFO("- ESC to quit");
    
    // Simulate initial weather data
    WeatherData initial_weather = {
        .temperature = 72.5f,
        .humidity = 65.0f,
        .timestamp = time(NULL)
    };
    strcpy(initial_weather.location, "Test City");
    strcpy(initial_weather.description, "Sunny");
    event_publish(event_system, "weather.current", &initial_weather, sizeof(initial_weather));
    
    bool running = true;
    SDL_Event event;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            } else {
                widget_manager_handle_event(widget_manager, &event);
            }
        }
        
        // Update
        widget_manager_update(widget_manager);
        
        // Render
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        
        widget_manager_render(widget_manager);
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);  // ~60 FPS
    }
    
    LOG_INFO("Shutting down Widget System Test");
    
    // Cleanup
    widget_factory_destroy(factory);
    widget_manager_destroy(widget_manager);
    event_system_destroy(event_system);
    state_store_destroy(state_store);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}