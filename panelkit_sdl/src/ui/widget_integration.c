#include "widget_integration.h"
#include "../state/state_store.h"
#include "../events/event_system.h"
#include "../core/sdl_includes.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Simple logging macros for integration layer
#ifndef log_info
#define log_info(fmt, ...) printf("[WIDGET_INTEGRATION] " fmt "\n", ##__VA_ARGS__)
#define log_error(fmt, ...) fprintf(stderr, "[WIDGET_INTEGRATION_ERROR] " fmt "\n", ##__VA_ARGS__)
#define log_debug(fmt, ...) printf("[WIDGET_INTEGRATION_DEBUG] " fmt "\n", ##__VA_ARGS__)
#endif

WidgetIntegration* widget_integration_create(SDL_Renderer* renderer) {
    WidgetIntegration* integration = calloc(1, sizeof(WidgetIntegration));
    if (!integration) {
        log_error("Failed to allocate widget integration");
        return NULL;
    }
    
    integration->renderer = renderer;
    
    // Create state store (always available)
    integration->state_store = state_store_create();
    if (!integration->state_store) {
        log_error("Failed to create state store for integration");
        free(integration);
        return NULL;
    }
    
    // Create event system (always available)
    integration->event_system = event_system_create();
    if (!integration->event_system) {
        log_error("Failed to create event system for integration");
        state_store_destroy(integration->state_store);
        free(integration);
        return NULL;
    }
    
    // Start with minimal integration - events disabled to avoid interference
    integration->widget_system_enabled = false;
    integration->events_enabled = false;
    integration->state_tracking_enabled = true;  // Safe to start state tracking immediately
    
    log_info("Widget integration layer created (running in background)");
    return integration;
}

void widget_integration_destroy(WidgetIntegration* integration) {
    if (!integration) {
        return;
    }
    
    event_system_destroy(integration->event_system);
    state_store_destroy(integration->state_store);
    free(integration);
    
    log_info("Widget integration layer destroyed");
}

void widget_integration_set_dimensions(WidgetIntegration* integration, int width, int height) {
    if (!integration) {
        return;
    }
    
    integration->screen_width = width;
    integration->screen_height = height;
    
    log_debug("Set integration dimensions: %dx%d", width, height);
}

void widget_integration_enable_events(WidgetIntegration* integration) {
    if (!integration) {
        return;
    }
    
    integration->events_enabled = true;
    log_info("Event integration enabled - events will be mirrored to widget system");
}

void widget_integration_enable_state_tracking(WidgetIntegration* integration) {
    if (!integration) {
        return;
    }
    
    integration->state_tracking_enabled = true;
    log_info("State tracking enabled - application state will be mirrored to widget system");
}

void widget_integration_enable_widgets(WidgetIntegration* integration) {
    if (!integration) {
        return;
    }
    
    integration->widget_system_enabled = true;
    log_info("Widget system enabled - widgets will be created alongside existing UI");
}

void widget_integration_mirror_touch_event(WidgetIntegration* integration, 
                                          int x, int y, bool is_down) {
    if (!integration || !integration->events_enabled) {
        return;
    }
    
    // Mirror touch events to widget event system
    struct {
        int x, y;
        bool is_down;
        uint32_t timestamp;
    } touch_data = {x, y, is_down, SDL_GetTicks()};
    
    const char* event_name = is_down ? "input.touch_down" : "input.touch_up";
    event_publish(integration->event_system, event_name, &touch_data, sizeof(touch_data));
    
    log_debug("Mirrored touch event: %s at (%d, %d)", event_name, x, y);
}

void widget_integration_mirror_button_press(WidgetIntegration* integration,
                                           int button_index, const char* button_text) {
    if (!integration || !integration->events_enabled) {
        return;
    }
    
    // Mirror button press to widget event system
    struct {
        int button_index;
        char button_text[64];
        uint32_t timestamp;
    } button_data = {button_index, {0}, SDL_GetTicks()};
    
    if (button_text) {
        strncpy(button_data.button_text, button_text, sizeof(button_data.button_text) - 1);
    }
    
    event_publish(integration->event_system, "ui.button_pressed", &button_data, sizeof(button_data));
    
    log_debug("Mirrored button press: index=%d text='%s'", button_index, button_text ? button_text : "");
}

void widget_integration_mirror_page_change(WidgetIntegration* integration,
                                          int from_page, int to_page) {
    if (!integration || !integration->events_enabled) {
        return;
    }
    
    // Mirror page change to widget event system
    struct {
        int from_page;
        int to_page;
        uint32_t timestamp;
    } page_data = {from_page, to_page, SDL_GetTicks()};
    
    event_publish(integration->event_system, "ui.page_changed", &page_data, sizeof(page_data));
    
    log_debug("Mirrored page change: %d -> %d", from_page, to_page);
}

void widget_integration_mirror_user_data(WidgetIntegration* integration,
                                        const void* user_data, size_t data_size) {
    if (!integration || !integration->state_tracking_enabled || !user_data) {
        return;
    }
    
    // Store user data in widget state store
    state_store_set(integration->state_store, "api_data", "user", user_data, data_size);
    
    // Also publish as event if events are enabled
    if (integration->events_enabled) {
        event_publish(integration->event_system, "api.user_data_updated", user_data, data_size);
    }
    
    log_debug("Mirrored user data (%zu bytes) to widget state", data_size);
}

void widget_integration_mirror_api_state(WidgetIntegration* integration,
                                        const char* state_name, const char* value) {
    if (!integration || !integration->state_tracking_enabled || !state_name || !value) {
        return;
    }
    
    // Store API state in widget state store
    state_store_set(integration->state_store, "api_state", state_name, 
                   value, strlen(value) + 1);
    
    // Also publish as event if events are enabled
    if (integration->events_enabled) {
        struct {
            char state_name[64];
            char value[256];
        } state_data = {{0}, {0}};
        
        strncpy(state_data.state_name, state_name, sizeof(state_data.state_name) - 1);
        strncpy(state_data.value, value, sizeof(state_data.value) - 1);
        
        event_publish(integration->event_system, "api.state_changed", &state_data, sizeof(state_data));
    }
    
    log_debug("Mirrored API state: %s = %s", state_name, value);
}

void widget_integration_update(WidgetIntegration* integration) {
    if (!integration) {
        return;
    }
    
    // Perform any periodic widget system updates
    // For now, this is just a placeholder for future widget updates
    // The existing UI system continues to handle all rendering and updates
}

bool widget_integration_has_user_data(WidgetIntegration* integration) {
    if (!integration || !integration->state_tracking_enabled) {
        return false;
    }
    
    size_t size;
    time_t timestamp;
    void* data = state_store_get(integration->state_store, "api_data", "user", &size, &timestamp);
    
    bool has_data = (data != NULL && size > 0);
    if (data) {
        free(data);
    }
    
    return has_data;
}

void* widget_integration_get_user_data(WidgetIntegration* integration, size_t* size) {
    if (!integration || !integration->state_tracking_enabled || !size) {
        return NULL;
    }
    
    time_t timestamp;
    void* data = state_store_get(integration->state_store, "api_data", "user", size, &timestamp);
    
    return data;  // Caller must free this
}