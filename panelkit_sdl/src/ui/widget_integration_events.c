#include "widget_integration.h"
#include "widget_integration_internal.h"
#include "../state/state_store.h"
#include "../events/event_system.h"
#include "../events/event_system_typed.h"
#include "../events/event_types.h"
#include "widgets/page_manager_widget.h"
#include "../core/sdl_includes.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "core/logger.h"
#include "core/error.h"

// Forward declarations for static event handlers
static void widget_button_click_handler(const ButtonEventData* data, void* context);
static void widget_page_transition_handler(int target_page, void* context);
static void widget_api_refresh_handler(uint32_t timestamp, void* context);

void widget_integration_mirror_touch_event(WidgetIntegration* integration, 
                                          int x, int y, bool is_down) {
    if (!integration) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM, "integration cannot be NULL");
        return;
    }
    if (!integration->events_enabled) {
        return;
    }
    
    // Mirror touch events to widget event system
    TouchEventData touch_data = {x, y, is_down, SDL_GetTicks()};
    
    if (is_down) {
        event_publish_touch_down(integration->event_system, &touch_data);
        log_debug("Mirrored touch event: touch_down at (%d, %d)", x, y);
    } else {
        event_publish_touch_up(integration->event_system, &touch_data);
        log_debug("Mirrored touch event: touch_up at (%d, %d)", x, y);
    }
}

void widget_integration_mirror_button_press(WidgetIntegration* integration,
                                           int button_index, const char* button_text) {
    if (!integration) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM, "integration cannot be NULL");
        return;
    }
    if (!integration->events_enabled) {
        return;
    }
    
    // Get current page from state store
    int current_page = 0;
    size_t size;
    time_t timestamp;
    int* stored_page = (int*)state_store_get(integration->state_store, "app", "current_page", &size, &timestamp);
    if (stored_page) {
        current_page = *stored_page;
        free(stored_page);
    }
    
    // Mirror button press to widget event system
    ButtonEventData button_data = {button_index, current_page, SDL_GetTicks(), {0}};
    
    if (button_text) {
        strncpy(button_data.button_text, button_text, sizeof(button_data.button_text) - 1);
    }
    
    event_publish_button_pressed(integration->event_system, &button_data);
    
    log_debug("Mirrored button press: page=%d index=%d text='%s'", current_page, button_index, button_text ? button_text : "");
}

void widget_integration_mirror_page_change(WidgetIntegration* integration,
                                          int from_page, int to_page) {
    if (!integration) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM, "integration cannot be NULL");
        return;
    }
    if (!integration->events_enabled) {
        return;
    }
    
    // Update current page in state store
    state_store_set(integration->state_store, "app", "current_page", &to_page, sizeof(int));
    
    // Mirror page change to widget event system
    PageChangeEventData page_data = {from_page, to_page, SDL_GetTicks()};
    
    event_publish_page_changed(integration->event_system, &page_data);
    
    log_debug("Mirrored page change: %d -> %d", from_page, to_page);
}

// Enable widget-based button handling
void widget_integration_enable_button_handling(WidgetIntegration* integration) {
    if (!integration) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM, "integration cannot be NULL");
        return;
    }
    if (!integration->event_system) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_STATE, "event system not initialized");
        return;
    }
    
    // Subscribe to button press events
    event_subscribe_button_pressed(integration->event_system, 
                                  widget_button_click_handler, integration);
    
    // Subscribe to page transition events
    event_subscribe_page_transition(integration->event_system, 
                                   widget_page_transition_handler, integration);
    
    // Subscribe to API refresh events
    event_subscribe_api_refresh_requested(integration->event_system, 
                                         widget_api_refresh_handler, integration);
    
    log_debug("Enabled widget-based button handling with event subscriptions");
}

// Widget-based button click handler
static void widget_button_click_handler(const ButtonEventData* button_data, void* context) {
    WidgetIntegration* integration = (WidgetIntegration*)context;
    if (!integration || !button_data) {
        return;
    }
    
    log_debug("Widget button click handler: page=%d button=%d text='%s'",
             button_data->page, button_data->button_index, button_data->button_text);
    
    // Handle actions based on page and button
    if (button_data->page == 0) { // Page 0 (first page)
        switch (button_data->button_index) {
            case 0: { // "Change Color" button - should transition to page 1
                int target_page = 1;
                event_publish_page_transition(integration->event_system, target_page);
                log_debug("Widget handler: Page transition to %d requested via event system", target_page);
                break;
            }
        }
    }
    else if (button_data->page == 1) { // Page 2 (zero-indexed)
        switch (button_data->button_index) {
            case 0: { // Blue button
                SDL_Color blue_color = {41, 128, 185, 255};
                state_store_set(integration->state_store, "app", "bg_color", &blue_color, sizeof(SDL_Color));
                log_debug("Widget handler: Background color set to blue via state store");
                break;
            }
            case 1: { // Random color button
                SDL_Color random_color = {
                    .r = rand() % 256,
                    .g = rand() % 256, 
                    .b = rand() % 256,
                    .a = 255
                };
                state_store_set(integration->state_store, "app", "bg_color", &random_color, sizeof(SDL_Color));
                log_debug("Widget handler: Background color set to random RGB(%d,%d,%d) via state store",
                         random_color.r, random_color.g, random_color.b);
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
            case 3: { // Go to Page 1 button
                int target_page = 0;
                event_publish_page_transition(integration->event_system, target_page);
                log_debug("Widget handler: Page transition to %d requested via event system", target_page);
                break;
            }
            case 4: { // Refresh API data button
                // Trigger API refresh via event
                uint32_t timestamp = SDL_GetTicks();
                event_publish_api_refresh_requested(integration->event_system, timestamp);
                log_debug("Widget handler: API refresh requested via event system");
                break;
            }
            case 5: { // Exit button
                bool quit = true;
                state_store_set(integration->state_store, "app", "quit", &quit, sizeof(bool));
                log_debug("Widget handler: Application exit requested via state store");
                break;
            }
        }
    }
}

// Handle page transition requests from widget system
static void widget_page_transition_handler(int target_page, void* context) {
    WidgetIntegration* integration = (WidgetIntegration*)context;
    if (!integration) return;
    log_debug("Widget page transition handler: transitioning to page %d", target_page);
    
    // Update state store with new page
    state_store_set(integration->state_store, "app", "current_page", &target_page, sizeof(int));
    
    // Use page manager widget
    if (integration->page_manager) {
        page_manager_transition_to(integration->page_manager, target_page);
    }
    
    // Update completed - log the transition
    log_debug("Page transition request: -> %d", target_page);
}

// Handle API refresh requests from widget system  
static void widget_api_refresh_handler(uint32_t timestamp, void* context) {
    WidgetIntegration* integration = (WidgetIntegration*)context;
    if (!integration || !integration->event_system) return;
    
    log_debug("Widget API refresh handler: publishing system event");
    
    // In widget mode, we could handle API refresh directly
    // For now, still publish the event as API manager is shared between modes
    ApiRefreshData api_event;
    
    api_event.timestamp = SDL_GetTicks();
    strcpy(api_event.source, "widget_system");
    
    event_publish_api_refresh(integration->event_system, &api_event);
    log_debug("Published system.api_refresh event from %s", api_event.source);
}

// Callback when page changes in page manager widget
void widget_integration_page_changed_callback(int from_page, int to_page, void* user_data) {
    WidgetIntegration* integration = (WidgetIntegration*)user_data;
    if (!integration) return;
    
    log_debug("Page manager widget changed page: %d -> %d", from_page, to_page);
    
    // Update state store with new page
    state_store_set(integration->state_store, "app", "current_page", &to_page, sizeof(int));
}