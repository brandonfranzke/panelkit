#include "event_button_poc.h"
#include "../events/event_system.h"
#include "../state/state_store.h"
#include <SDL_ttf.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// Simple logging macros for POC
#define log_info(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define log_error(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#define log_debug(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

// Test data structure for weather
typedef struct {
    float temperature;
    char location[64];
    time_t timestamp;
} WeatherData;

// Event handler for the button
static void button_event_handler(const char* event_name, 
                               const void* data, 
                               size_t data_size,
                               void* context) {
    EventButtonPOC* button = (EventButtonPOC*)context;
    if (!button) {
        log_error("Invalid button context in event handler");
        return;
    }
    
    log_info("Button '%s' received event '%s' (%zu bytes)", 
             button->label, event_name, data_size);
    
    // Handle weather temperature event
    if (strcmp(event_name, "weather.temperature") == 0 && 
        data_size == sizeof(WeatherData)) {
        const WeatherData* weather = (const WeatherData*)data;
        
        // Update display value
        snprintf(button->display_value, sizeof(button->display_value),
                "%.1f°F @ %s", weather->temperature, weather->location);
        
        button->has_data = true;
        
        // Also store in state store if available
        if (button->state_store) {
            state_store_set(button->state_store, "weather_temperature", 
                          weather->location, weather, sizeof(*weather));
            log_debug("Stored weather data in state store");
        }
    }
}

EventButtonPOC* event_button_poc_create(int x, int y, int width, int height, 
                                       const char* label, const char* event_name) {
    EventButtonPOC* button = calloc(1, sizeof(EventButtonPOC));
    if (!button) {
        log_error("Failed to allocate event button");
        return NULL;
    }
    
    // Initialize button properties
    button->rect.x = x;
    button->rect.y = y;
    button->rect.w = width;
    button->rect.h = height;
    
    // Default colors
    button->color = (SDL_Color){100, 100, 100, 255};
    button->hover_color = (SDL_Color){120, 120, 120, 255};
    button->text_color = (SDL_Color){255, 255, 255, 255};
    
    // Set label and event
    strncpy(button->label, label, sizeof(button->label) - 1);
    strncpy(button->subscribed_event, event_name, sizeof(button->subscribed_event) - 1);
    strncpy(button->display_value, "No data", sizeof(button->display_value) - 1);
    
    log_info("Created event button '%s' for event '%s'", label, event_name);
    return button;
}

void event_button_poc_destroy(EventButtonPOC* button) {
    if (!button) {
        return;
    }
    
    // Unsubscribe if still subscribed
    if (button->is_subscribed) {
        event_button_poc_unsubscribe(button);
    }
    
    free(button);
}

bool event_button_poc_subscribe(EventButtonPOC* button, EventSystem* events, StateStore* store) {
    if (!button || !events) {
        log_error("Invalid parameters for button subscription");
        return false;
    }
    
    // Unsubscribe first if already subscribed
    if (button->is_subscribed) {
        event_button_poc_unsubscribe(button);
    }
    
    // Store references
    button->event_system = events;
    button->state_store = store;
    
    // Subscribe to the event
    bool success = event_subscribe(events, button->subscribed_event, 
                                  button_event_handler, button);
    
    if (success) {
        button->is_subscribed = true;
        log_info("Button '%s' subscribed to event '%s'", 
                button->label, button->subscribed_event);
        
        // Check if we have existing data in state store
        if (store) {
            // Try to get latest weather data for any location
            size_t data_size;
            time_t timestamp;
            void* existing_data = state_store_get(store, "weather_temperature", 
                                                "default", &data_size, &timestamp);
            if (existing_data && data_size == sizeof(WeatherData)) {
                button_event_handler(button->subscribed_event, existing_data, 
                                   data_size, button);
                free(existing_data);
            }
        }
    } else {
        log_error("Failed to subscribe button to event");
    }
    
    return success;
}

void event_button_poc_unsubscribe(EventButtonPOC* button) {
    if (!button || !button->event_system || !button->is_subscribed) {
        return;
    }
    
    event_unsubscribe(button->event_system, button->subscribed_event, 
                     button_event_handler);
    
    button->is_subscribed = false;
    button->event_system = NULL;
    
    log_info("Button '%s' unsubscribed from event '%s'", 
            button->label, button->subscribed_event);
}

void event_button_poc_handle_event(EventButtonPOC* button, SDL_Event* event) {
    if (!button || !event) {
        return;
    }
    
    switch (event->type) {
        case SDL_MOUSEMOTION: {
            int x = event->motion.x;
            int y = event->motion.y;
            button->is_hovered = (x >= button->rect.x && 
                                x < button->rect.x + button->rect.w &&
                                y >= button->rect.y && 
                                y < button->rect.y + button->rect.h);
            break;
        }
        
        case SDL_MOUSEBUTTONDOWN: {
            if (event->button.button == SDL_BUTTON_LEFT && button->is_hovered) {
                button->is_pressed = true;
                
                // Simulate triggering a weather update when clicked
                if (button->event_system) {
                    log_info("Button clicked - simulating weather update");
                    event_button_poc_simulate_weather_event(button->event_system, 
                                                          75.5f, "TestCity");
                }
            }
            break;
        }
        
        case SDL_MOUSEBUTTONUP: {
            if (event->button.button == SDL_BUTTON_LEFT) {
                button->is_pressed = false;
            }
            break;
        }
    }
}

void event_button_poc_update(EventButtonPOC* button) {
    // Could add animations or other updates here
}

void event_button_poc_render(EventButtonPOC* button, SDL_Renderer* renderer) {
    if (!button || !renderer) return;
    
    // Draw button background
    if (button->is_pressed) {
        SDL_SetRenderDrawColor(renderer, 
                              button->hover_color.r - 20, 
                              button->hover_color.g - 20, 
                              button->hover_color.b - 20, 
                              button->hover_color.a);
    } else if (button->is_hovered) {
        SDL_SetRenderDrawColor(renderer, 
                              button->hover_color.r, 
                              button->hover_color.g, 
                              button->hover_color.b, 
                              button->hover_color.a);
    } else {
        SDL_SetRenderDrawColor(renderer, 
                              button->color.r, 
                              button->color.g, 
                              button->color.b, 
                              button->color.a);
    }
    SDL_RenderFillRect(renderer, &button->rect);
    
    // Draw border
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &button->rect);
    
    // Draw simple text indicators
    // Top half: label
    SDL_Rect label_area = {
        button->rect.x + 10,
        button->rect.y + 10,
        button->rect.w - 20,
        button->rect.h / 2 - 10
    };
    
    // Draw a simple box for label text area
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(renderer, &label_area);
    
    // Bottom half: display value or status
    if (button->has_data && strlen(button->display_value) > 0) {
        SDL_Rect value_area = {
            button->rect.x + 10,
            button->rect.y + button->rect.h / 2,
            button->rect.w - 20,
            button->rect.h / 2 - 10
        };
        
        // Green indicator for data
        SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
        SDL_RenderDrawRect(renderer, &value_area);
    } else {
        // Show subscription status
        SDL_Rect status_area = {
            button->rect.x + 10,
            button->rect.y + button->rect.h / 2,
            button->rect.w - 20,
            button->rect.h / 2 - 10
        };
        
        if (button->is_subscribed) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 200, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 200, 100, 100, 255);
        }
        SDL_RenderDrawRect(renderer, &status_area);
    }
}

void event_button_poc_simulate_weather_event(EventSystem* events, 
                                           float temperature, 
                                           const char* location) {
    if (!events) {
        return;
    }
    
    WeatherData weather = {
        .temperature = temperature,
        .timestamp = time(NULL)
    };
    strncpy(weather.location, location, sizeof(weather.location) - 1);
    weather.location[sizeof(weather.location) - 1] = '\0';
    
    log_info("Publishing simulated weather event: %.1f°F at %s", 
            temperature, location);
    
    event_publish(events, "weather.temperature", &weather, sizeof(weather));
}