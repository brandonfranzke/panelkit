#include "weather_widget.h"
#include "../../events/event_system.h"
#include "../../events/event_system_typed.h"
#include "../../state/state_store.h"
#include <SDL_ttf.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "core/logger.h"

// Forward declarations for virtual functions
static void weather_widget_render(Widget* widget, SDL_Renderer* renderer);
static void weather_widget_handle_event(Widget* widget, const SDL_Event* event);
static void weather_widget_handle_data_event(Widget* widget, const char* event_name,
                                           const void* data, size_t data_size);
static void weather_widget_destroy(Widget* widget);

WeatherWidget* weather_widget_create(const char* id, const char* location) {
    WeatherWidget* weather = calloc(1, sizeof(WeatherWidget));
    if (!weather) {
        log_error("Failed to allocate weather widget");
        return NULL;
    }
    
    // Initialize base widget
    Widget* base = &weather->base;
    strncpy(base->id, id, sizeof(base->id) - 1);
    base->type = WIDGET_TYPE_WEATHER;
    
    // Set virtual functions
    base->render = weather_widget_render;
    base->handle_event = weather_widget_handle_event;
    base->handle_data_event = weather_widget_handle_data_event;
    base->destroy = weather_widget_destroy;
    
    // Initialize widget arrays (required by base widget)
    base->child_capacity = 0;  // Weather widget has no children
    base->children = NULL;
    
    base->event_capacity = 4;
    base->subscribed_events = calloc(base->event_capacity, sizeof(char*));
    if (!base->subscribed_events) {
        free(weather);
        return NULL;
    }
    
    // Set default colors
    base->background_color = (SDL_Color){250, 250, 250, 255};
    base->foreground_color = (SDL_Color){0, 0, 0, 255};
    base->border_color = (SDL_Color){200, 200, 200, 255};
    base->border_width = 1;
    base->padding = 10;
    
    // Set default size
    base->bounds.w = 200;
    base->bounds.h = 150;
    
    // Initialize weather-specific data
    strncpy(weather->current_weather.location, location, 
            sizeof(weather->current_weather.location) - 1);
    weather->show_humidity = true;
    weather->show_description = true;
    weather->use_celsius = false;
    weather->update_interval = 300;  // 5 minutes
    
    // Subscribe to weather events
    widget_subscribe_event(base, "weather.current");
    widget_subscribe_event(base, "weather.temperature");
    
    log_info("Created weather widget '%s' for location '%s'", id, location);
    return weather;
}

void weather_widget_set_location(WeatherWidget* widget, const char* location) {
    if (!widget || !location) {
        return;
    }
    
    strncpy(widget->current_weather.location, location,
            sizeof(widget->current_weather.location) - 1);
    widget_invalidate(&widget->base);
}

void weather_widget_set_units(WeatherWidget* widget, bool use_celsius) {
    if (!widget) {
        return;
    }
    
    widget->use_celsius = use_celsius;
    widget_invalidate(&widget->base);
}

void weather_widget_set_display_options(WeatherWidget* widget, 
                                      bool show_humidity, 
                                      bool show_description) {
    if (!widget) {
        return;
    }
    
    widget->show_humidity = show_humidity;
    widget->show_description = show_description;
    widget_invalidate(&widget->base);
}

void weather_widget_set_data(WeatherWidget* widget, const WeatherData* data) {
    if (!widget || !data) {
        return;
    }
    
    widget->current_weather = *data;
    widget->has_data = true;
    widget->last_update = time(NULL);
    widget_invalidate(&widget->base);
}

void weather_widget_request_update(WeatherWidget* widget) {
    if (!widget || !widget->base.event_system) {
        return;
    }
    
    // Publish request event
    struct {
        char location[64];
    } request = {0};
    
    strncpy(request.location, widget->current_weather.location,
            sizeof(request.location) - 1);
    
    event_publish_weather_request(widget->base.event_system, request.location);
    
    log_debug("Weather widget requested update for '%s'", request.location);
}

static void weather_widget_render(Widget* widget, SDL_Renderer* renderer) {
    WeatherWidget* weather = (WeatherWidget*)widget;
    if (!weather || !renderer) {
        return;
    }
    
    // Call base render for background and border
    widget_default_render(widget, renderer);
    
    // Draw content area
    SDL_Rect content = {
        widget->bounds.x + widget->padding,
        widget->bounds.y + widget->padding,
        widget->bounds.w - widget->padding * 2,
        widget->bounds.h - widget->padding * 2
    };
    
    if (!weather->has_data) {
        // Draw loading indicator
        SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
        SDL_Rect loading_rect = {
            content.x + content.w / 2 - 30,
            content.y + content.h / 2 - 10,
            60, 20
        };
        SDL_RenderDrawRect(renderer, &loading_rect);
        return;
    }
    
    // Draw weather data with simple rectangles to indicate content areas
    int y_offset = content.y;
    
    // Location
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_Rect location_rect = {content.x, y_offset, content.w, 25};
    SDL_RenderDrawRect(renderer, &location_rect);
    y_offset += 30;
    
    // Temperature
    float temp = weather->current_weather.temperature;
    if (weather->use_celsius) {
        temp = (temp - 32.0f) * 5.0f / 9.0f;
    }
    
    SDL_SetRenderDrawColor(renderer, 50, 50, 200, 255);
    SDL_Rect temp_rect = {content.x, y_offset, content.w, 40};
    SDL_RenderDrawRect(renderer, &temp_rect);
    
    // Draw temperature indicator (blue for cold, red for hot)
    int temp_indicator = (int)((temp - 32.0f) / 68.0f * 100.0f);  // 0-100 scale
    if (temp_indicator < 0) temp_indicator = 0;
    if (temp_indicator > 100) temp_indicator = 100;
    
    SDL_SetRenderDrawColor(renderer, 
                          temp_indicator * 2, 
                          50, 
                          (100 - temp_indicator) * 2, 
                          255);
    SDL_Rect temp_bar = {
        temp_rect.x + 5,
        temp_rect.y + 5,
        (temp_rect.w - 10) * temp_indicator / 100,
        temp_rect.h - 10
    };
    SDL_RenderFillRect(renderer, &temp_bar);
    y_offset += 45;
    
    // Humidity
    if (weather->show_humidity && weather->current_weather.humidity > 0) {
        SDL_SetRenderDrawColor(renderer, 100, 150, 200, 255);
        SDL_Rect humidity_rect = {content.x, y_offset, content.w, 20};
        SDL_RenderDrawRect(renderer, &humidity_rect);
        
        // Humidity bar
        int humidity_width = (int)(content.w * weather->current_weather.humidity / 100.0f);
        SDL_Rect humidity_bar = {
            humidity_rect.x + 2,
            humidity_rect.y + 2,
            humidity_width - 4,
            humidity_rect.h - 4
        };
        SDL_RenderFillRect(renderer, &humidity_bar);
        y_offset += 25;
    }
    
    // Description
    if (weather->show_description && strlen(weather->current_weather.description) > 0) {
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        SDL_Rect desc_rect = {content.x, y_offset, content.w, 20};
        SDL_RenderDrawRect(renderer, &desc_rect);
    }
    
    // Update indicator
    time_t now = time(NULL);
    int age = (int)(now - weather->last_update);
    if (age < 60) {
        SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
    } else if (age < 300) {
        SDL_SetRenderDrawColor(renderer, 200, 200, 0, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
    }
    
    SDL_Rect update_indicator = {
        widget->bounds.x + widget->bounds.w - 15,
        widget->bounds.y + 5,
        10, 10
    };
    SDL_RenderFillRect(renderer, &update_indicator);
}

static void weather_widget_handle_event(Widget* widget, const SDL_Event* event) {
    WeatherWidget* weather = (WeatherWidget*)widget;
    if (!weather) {
        return;
    }
    
    // Call base handler for hover/press states
    widget_default_handle_event(widget, event);
    
    // Handle click to request update
    if (event->type == SDL_MOUSEBUTTONDOWN && 
        event->button.button == SDL_BUTTON_LEFT &&
        widget_contains_point(widget, event->button.x, event->button.y)) {
        
        time_t now = time(NULL);
        if (now - weather->last_update > 5) {  // Rate limit to 5 seconds
            weather_widget_request_update(weather);
        }
    }
}

static void weather_widget_handle_data_event(Widget* widget, const char* event_name,
                                           const void* data, size_t data_size) {
    WeatherWidget* weather = (WeatherWidget*)widget;
    if (!weather || !data) {
        return;
    }
    
    log_debug("Weather widget received event '%s' (%zu bytes)", event_name, data_size);
    
    // Handle different weather event types
    if (strcmp(event_name, "weather.current") == 0 && 
        data_size == sizeof(WeatherData)) {
        const WeatherData* weather_data = (const WeatherData*)data;
        
        // Only update if it's for our location
        if (strcmp(weather_data->location, weather->current_weather.location) == 0) {
            weather_widget_set_data(weather, weather_data);
            log_info("Weather widget updated: %.1f°F at %s",
                    weather_data->temperature, weather_data->location);
        }
    } else if (strcmp(event_name, "weather.temperature") == 0) {
        // Handle simple temperature update
        if (data_size >= sizeof(WeatherData)) {
            const WeatherData* weather_data = (const WeatherData*)data;
            if (strcmp(weather_data->location, weather->current_weather.location) == 0) {
                weather->current_weather.temperature = weather_data->temperature;
                weather->current_weather.timestamp = weather_data->timestamp;
                weather->has_data = true;
                weather->last_update = time(NULL);
                widget_invalidate(widget);
            }
        }
    }
    
    // Store in state store if available
    if (widget->state_store && weather->has_data) {
        state_store_set(widget->state_store, "weather_data",
                       weather->current_weather.location,
                       &weather->current_weather,
                       sizeof(weather->current_weather));
    }
}

static void weather_widget_destroy(Widget* widget) {
    WeatherWidget* weather = (WeatherWidget*)widget;
    if (!weather) {
        return;
    }
    
    // No additional cleanup needed for weather widget
    // Base widget cleanup handles the rest
}