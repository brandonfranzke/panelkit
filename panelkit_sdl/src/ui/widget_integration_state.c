#include "widget_integration.h"
#include "widget_integration_internal.h"
#include "../state/state_store.h"
#include "../events/event_system.h"
#include "../events/event_system_typed.h"
#include "../events/event_types.h"
#include "../core/sdl_includes.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "core/logger.h"

// Initialize application state in the state store
void widget_integration_init_app_state(WidgetIntegration* integration) {
    if (!integration || !integration->state_store) return;
    
    log_debug("Initializing application state in state store");
    
    // Application running state
    bool quit = false;
    state_store_set(integration->state_store, "app", "quit", &quit, sizeof(bool));
    
    // Current page (initially 0)
    int current_page = 0;
    state_store_set(integration->state_store, "app", "current_page", &current_page, sizeof(int));
    
    // Show time flag
    bool show_time = true;
    state_store_set(integration->state_store, "app", "show_time", &show_time, sizeof(bool));
    
    // Show debug flag  
    bool show_debug = true;
    state_store_set(integration->state_store, "app", "show_debug", &show_debug, sizeof(bool));
    
    // Background color
    SDL_Color bg_color = {33, 33, 33, 255};
    state_store_set(integration->state_store, "app", "bg_color", &bg_color, sizeof(SDL_Color));
    
    // Page 1 text
    const char* page1_text = "Welcome to Page 1! Swipe right to see buttons.";
    state_store_set(integration->state_store, "app", "page1_text", page1_text, strlen(page1_text) + 1);
    
    // Page 1 text color index  
    int page1_text_color = 0;
    state_store_set(integration->state_store, "app", "page1_text_color", &page1_text_color, sizeof(int));
    
    // FPS and debug data
    Uint32 fps = 0;
    state_store_set(integration->state_store, "app", "fps", &fps, sizeof(Uint32));
    
    Uint32 frame_count = 0;
    state_store_set(integration->state_store, "app", "frame_count", &frame_count, sizeof(Uint32));
    
    Uint32 fps_timer = 0;
    state_store_set(integration->state_store, "app", "fps_timer", &fps_timer, sizeof(Uint32));
    
    log_debug("Application state initialized in state store");
}

// Get current page from state store
int widget_integration_get_current_page(WidgetIntegration* integration) {
    if (!integration || !integration->state_store) return 0;
    
    size_t size;
    time_t timestamp;
    int* page = (int*)state_store_get(integration->state_store, "app", "current_page", &size, &timestamp);
    if (page && size == sizeof(int)) {
        int result = *page;
        free(page);
        return result;
    }
    return 0;
}

// Update FPS in state store
void widget_integration_update_fps(WidgetIntegration* integration, Uint32 fps) {
    if (!integration || !integration->state_store) return;
    
    state_store_set(integration->state_store, "app", "fps", &fps, sizeof(Uint32));
}

// Get show_debug flag from state store
bool widget_integration_get_show_debug(WidgetIntegration* integration) {
    if (!integration || !integration->state_store) return true; // Default
    
    size_t size;
    time_t timestamp;
    bool* show_debug = (bool*)state_store_get(integration->state_store, "app", "show_debug", &size, &timestamp);
    if (show_debug && size == sizeof(bool)) {
        bool result = *show_debug;
        free(show_debug);
        return result;
    }
    return true; // Default
}

// Get quit flag from state store
bool widget_integration_get_quit(WidgetIntegration* integration) {
    if (!integration || !integration->state_store) return false;
    
    size_t size;
    time_t timestamp;
    bool* quit = (bool*)state_store_get(integration->state_store, "app", "quit", &size, &timestamp);
    if (quit && size == sizeof(bool)) {
        bool result = *quit;
        free(quit);
        return result;
    }
    return false;
}

// Set quit flag in state store
void widget_integration_set_quit(WidgetIntegration* integration, bool quit) {
    if (!integration || !integration->state_store) return;
    
    state_store_set(integration->state_store, "app", "quit", &quit, sizeof(bool));
    log_debug("Widget state: quit flag set to %s", quit ? "true" : "false");
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

void widget_integration_mirror_user_data(WidgetIntegration* integration,
                                        const void* user_data, size_t data_size) {
    if (!integration || !integration->state_tracking_enabled || !user_data) {
        return;
    }
    
    // Store user data in widget state store
    state_store_set(integration->state_store, "api_data", "user", user_data, data_size);
    
    // Also publish as event if events are enabled
    if (integration->events_enabled) {
        event_publish_api_user_data_updated(integration->event_system, user_data, data_size);
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
        ApiStateChangeData state_data = {{0}, {0}};
        
        strncpy(state_data.state_name, state_name, sizeof(state_data.state_name) - 1);
        strncpy(state_data.value, value, sizeof(state_data.value) - 1);
        
        event_publish_api_state_changed(integration->event_system, &state_data);
    }
    
    log_debug("Mirrored API state: %s = %s", state_name, value);
}

// Sync state from widget store back to global variables (for gradual migration)
void widget_integration_sync_state_to_globals(WidgetIntegration* integration, 
                                             SDL_Color* bg_color, bool* show_time, bool* quit, int* page1_text_color) {
    if (!integration || !integration->state_store) return;
    
    size_t size;
    time_t timestamp;
    
    // Sync background color
    if (bg_color) {
        SDL_Color* stored_color = (SDL_Color*)state_store_get(integration->state_store, 
                                                             "app", "bg_color", &size, &timestamp);
        if (stored_color && size == sizeof(SDL_Color)) {
            *bg_color = *stored_color;
            free(stored_color);
        }
    }
    
    // Sync show_time flag
    if (show_time) {
        bool* stored_show_time = (bool*)state_store_get(integration->state_store, 
                                                       "app", "show_time", &size, &timestamp);
        if (stored_show_time && size == sizeof(bool)) {
            *show_time = *stored_show_time;
            free(stored_show_time);
        }
    }
    
    // Sync quit flag
    if (quit) {
        bool* stored_quit = (bool*)state_store_get(integration->state_store, 
                                                  "app", "quit", &size, &timestamp);
        if (stored_quit && size == sizeof(bool)) {
            *quit = *stored_quit;
            free(stored_quit);
        }
    }
    
    // Sync page1_text_color
    if (page1_text_color) {
        int* stored_color = (int*)state_store_get(integration->state_store, 
                                                 "app", "page1_text_color", &size, &timestamp);
        if (stored_color && size == sizeof(int)) {
            *page1_text_color = *stored_color;
            free(stored_color);
        }
    }
}