#include "widget_integration.h"
#include "../state/state_store.h"
#include "../events/event_system.h"
#include "widget_manager.h"
#include "widget_factory.h"
#include "widget.h"
#include "widgets/button_widget.h"
#include "page_widget.h"
#include "../core/sdl_includes.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>

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
    
    // Create widget manager (but don't create widgets yet)
    integration->widget_manager = widget_manager_create(renderer, 
                                                      integration->event_system,
                                                      integration->state_store);
    if (!integration->widget_manager) {
        log_error("Failed to create widget manager for integration");
        event_system_destroy(integration->event_system);
        state_store_destroy(integration->state_store);
        free(integration);
        return NULL;
    }
    
    // Create widget factory
    integration->widget_factory = widget_factory_create_default();
    if (!integration->widget_factory) {
        log_error("Failed to create widget factory for integration");
        widget_manager_destroy(integration->widget_manager);
        event_system_destroy(integration->event_system);
        state_store_destroy(integration->state_store);
        free(integration);
        return NULL;
    }
    
    // Start with minimal integration - events disabled to avoid interference
    integration->widget_system_enabled = false;
    integration->events_enabled = false;
    integration->state_tracking_enabled = true;  // Safe to start state tracking immediately
    integration->shadow_widgets_created = false;
    integration->num_pages = 2;  // PanelKit has 2 pages
    
    // Initialize widget arrays
    memset(integration->page_widgets, 0, sizeof(integration->page_widgets));
    memset(integration->button_widgets, 0, sizeof(integration->button_widgets));
    
    // Initialize application state in state store
    widget_integration_init_app_state(integration);
    
    log_info("Widget integration layer created (running in background)");
    return integration;
}

void widget_integration_destroy(WidgetIntegration* integration) {
    if (!integration) {
        return;
    }
    
    // Destroy shadow widgets (widget manager handles this)
    // Note: We don't individually destroy widgets as widget_manager owns them
    
    widget_factory_destroy(integration->widget_factory);
    widget_manager_destroy(integration->widget_manager);
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
    
    // Update current page in state store
    state_store_set(integration->state_store, "app", "current_page", &to_page, sizeof(int));
    
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

void widget_integration_create_shadow_widgets(WidgetIntegration* integration) {
    if (!integration || !integration->widget_manager || integration->shadow_widgets_created) {
        return;
    }
    
    log_info("Creating shadow widgets to mirror existing UI structure");
    
    // Create page widgets for each page
    for (int i = 0; i < integration->num_pages; i++) {
        char page_id[32];
        char page_title[64];
        snprintf(page_id, sizeof(page_id), "page_%d", i);
        snprintf(page_title, sizeof(page_title), "Page %d", i + 1);
        
        // Create a page widget (using base widget for now)
        Widget* page = widget_create(page_id, WIDGET_TYPE_CONTAINER);
        if (page) {
            widget_set_bounds(page, 0, 0, integration->screen_width, integration->screen_height);
            integration->page_widgets[i] = page;
            
            // Add to widget manager as a root widget
            widget_manager_add_root(integration->widget_manager, page, page_id);
            
            log_debug("Created shadow page widget: %s", page_id);
        }
    }
    
    // Create button widgets for existing buttons
    // Page 0 (Page 1 in UI) has 1 button
    // Page 1 (Page 2 in UI) has up to 9 buttons
    
    // Page 0 button
    if (integration->page_widgets[0]) {
        ButtonParams params = {"Change Color"};
        Widget* button = widget_factory_create_widget(integration->widget_factory,
                                                    "button", "page0_button0", &params);
        if (button) {
            widget_set_relative_bounds(button, 20, 100, 200, 50);
            widget_add_child(integration->page_widgets[0], button);
            integration->button_widgets[0][0] = button;
            log_debug("Created shadow button widget: page0_button0");
        }
    }
    
    // Page 1 buttons (color buttons)
    if (integration->page_widgets[1]) {
        const char* button_labels[] = {
            "Blue", "Green", "Gray", "Time", "Yellow", 
            "Fetch User", "", "", ""
        };
        
        for (int i = 0; i < 9; i++) {
            if (strlen(button_labels[i]) > 0) {
                char button_id[32];
                snprintf(button_id, sizeof(button_id), "page1_button%d", i);
                
                ButtonParams params = {button_labels[i]};
                Widget* button = widget_factory_create_widget(integration->widget_factory,
                                                            "button", button_id, &params);
                if (button) {
                    // Calculate position based on 3x3 grid
                    int row = i / 3;
                    int col = i % 3;
                    int button_width = integration->screen_width / 3 - 20;
                    int button_height = integration->screen_height / 3 - 20;
                    int x = col * (button_width + 10) + 10;
                    int y = row * (button_height + 10) + 10;
                    
                    widget_set_relative_bounds(button, x, y, button_width, button_height);
                    widget_add_child(integration->page_widgets[1], button);
                    integration->button_widgets[1][i] = button;
                    log_debug("Created shadow button widget: %s", button_id);
                }
            }
        }
    }
    
    // Set page 1 as initially active
    if (integration->widget_manager && integration->page_widgets[0]) {
        widget_manager_set_active_root(integration->widget_manager, "page_0");
    }
    
    integration->shadow_widgets_created = true;
    log_info("Shadow widget tree created successfully");
}

void widget_integration_sync_button_state(WidgetIntegration* integration, 
                                        int page, int button_index, 
                                        const char* text, bool enabled) {
    if (!integration || !integration->shadow_widgets_created || 
        page < 0 || page >= integration->num_pages ||
        button_index < 0 || button_index >= 9) {
        return;
    }
    
    Widget* button = integration->button_widgets[page][button_index];
    if (button && button->type == WIDGET_TYPE_BUTTON) {
        ButtonWidget* btn = (ButtonWidget*)button;
        
        // Update button text if provided
        if (text) {
            button_widget_set_label(btn, text);
        }
        
        // Update enabled state
        widget_set_enabled(button, enabled);
        
        log_debug("Synced button state: page=%d button=%d text='%s' enabled=%d",
                 page, button_index, text ? text : "", enabled);
    }
}

void widget_integration_sync_page_state(WidgetIntegration* integration,
                                       int page_index, bool is_active) {
    if (!integration || !integration->shadow_widgets_created || 
        !integration->widget_manager ||
        page_index < 0 || page_index >= integration->num_pages) {
        return;
    }
    
    if (is_active) {
        char page_id[32];
        snprintf(page_id, sizeof(page_id), "page_%d", page_index);
        widget_manager_set_active_root(integration->widget_manager, page_id);
        log_debug("Activated shadow page: %s", page_id);
    }
}

Widget* widget_integration_get_page_widget(WidgetIntegration* integration, int page) {
    if (!integration || page < 0 || page >= integration->num_pages) {
        return NULL;
    }
    return integration->page_widgets[page];
}

Widget* widget_integration_get_button_widget(WidgetIntegration* integration, int page, int button) {
    if (!integration || page < 0 || page >= integration->num_pages ||
        button < 0 || button >= 9) {
        return NULL;
    }
    return integration->button_widgets[page][button];
}

// Initialize application state in the state store
void widget_integration_init_app_state(WidgetIntegration* integration) {
    if (!integration || !integration->state_store) return;
    
    log_debug("Initializing application state in state store");
    
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
    
    // FPS data
    Uint32 fps = 0;
    state_store_set(integration->state_store, "app", "fps", &fps, sizeof(Uint32));
    
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

// Widget-based button click handler
static void widget_button_click_handler(const char* event_name, const void* data, size_t data_size, void* context) {
    WidgetIntegration* integration = (WidgetIntegration*)context;
    if (!integration || !data || data_size < sizeof(struct { int button_index; int page; })) {
        return;
    }
    
    struct {
        int button_index;
        int page;
        uint32_t timestamp;
        char button_text[32];
    } *button_data = (void*)data;
    
    log_debug("Widget button click handler: page=%d button=%d text='%s'",
             button_data->page, button_data->button_index, button_data->button_text);
    
    // Handle actions based on page and button
    if (button_data->page == 1) { // Page 2 (zero-indexed)
        switch (button_data->button_index) {
            case 0: { // Blue button
                SDL_Color blue_color = {41, 128, 185, 255};
                state_store_set(integration->state_store, "app", "bg_color", &blue_color, sizeof(SDL_Color));
                log_debug("Widget handler: Background color set to blue via state store");
                break;
            }
            case 2: { // Time toggle button  
                // Get current show_time state
                size_t size;
                time_t timestamp;
                bool* show_time = (bool*)state_store_get(integration->state_store, "app", "show_time", &size, &timestamp);
                bool new_show_time = show_time ? !(*show_time) : true;
                if (show_time) free(show_time);
                
                state_store_set(integration->state_store, "app", "show_time", &new_show_time, sizeof(bool));
                log_debug("Widget handler: Time display toggled to %s via state store", 
                         new_show_time ? "enabled" : "disabled");
                break;
            }
        }
    }
}

// Enable widget-based button handling
void widget_integration_enable_button_handling(WidgetIntegration* integration) {
    if (!integration || !integration->event_system) return;
    
    // Subscribe to button press events
    event_subscribe(integration->event_system, "ui.button_pressed", 
                   widget_button_click_handler, integration);
    
    log_debug("Enabled widget-based button handling");
}

// Sync state from widget store back to global variables (for gradual migration)
void widget_integration_sync_state_to_globals(WidgetIntegration* integration, 
                                             SDL_Color* bg_color, bool* show_time) {
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
}