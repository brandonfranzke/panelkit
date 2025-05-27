#include "widget_integration.h"
#include "widget_integration_internal.h"
#include "../state/state_store.h"
#include "../events/event_system.h"
#include "widget_manager.h"
#include "widget_factory.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "core/logger.h"
#include "core/error.h"

WidgetIntegration* widget_integration_create(SDL_Renderer* renderer) {
    if (!renderer) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM, "renderer cannot be NULL");
        return NULL;
    }
    
    WidgetIntegration* integration = calloc(1, sizeof(WidgetIntegration));
    if (!integration) {
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY, 
            "widget_integration_create: Failed to allocate widget integration");
        log_error("Failed to allocate widget integration");
        return NULL;
    }
    
    integration->renderer = renderer;
    
    // Create state store (always available)
    integration->state_store = state_store_create();
    if (!integration->state_store) {
        pk_set_last_error_with_context(PK_ERROR_SYSTEM, 
            "widget_integration_create: Failed to create state store for integration");
        log_error("Failed to create state store for integration");
        free(integration);
        return NULL;
    }
    
    // Create event system (always available)
    integration->event_system = event_system_create();
    if (!integration->event_system) {
        pk_set_last_error_with_context(PK_ERROR_SYSTEM, 
            "widget_integration_create: Failed to create event system for integration");
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
        pk_set_last_error_with_context(PK_ERROR_SYSTEM, 
            "widget_integration_create: Failed to create widget manager for integration");
        log_error("Failed to create widget manager for integration");
        event_system_destroy(integration->event_system);
        state_store_destroy(integration->state_store);
        free(integration);
        return NULL;
    }
    
    // Create widget factory
    integration->widget_factory = widget_factory_create_default();
    if (!integration->widget_factory) {
        pk_set_last_error_with_context(PK_ERROR_SYSTEM, 
            "widget_integration_create: Failed to create widget factory for integration");
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
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM, "integration cannot be NULL");
        return;
    }
    
    integration->screen_width = width;
    integration->screen_height = height;
    
    log_debug("Set integration dimensions: %dx%d", width, height);
}

void widget_integration_set_fonts(WidgetIntegration* integration, 
                                  TTF_Font* regular, TTF_Font* large, TTF_Font* small) {
    if (!integration) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM, "integration cannot be NULL");
        return;
    }
    
    integration->font_regular = regular;
    integration->font_large = large;
    integration->font_small = small;
    
    log_debug("Set integration fonts: regular=%p, large=%p, small=%p", 
              (void*)regular, (void*)large, (void*)small);
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

void widget_integration_update(WidgetIntegration* integration) {
    if (!integration) {
        return;
    }
    
    // Perform any periodic widget system updates
    // For now, this is just a placeholder for future widget updates
    // The existing UI system continues to handle all rendering and updates
}

EventSystem* widget_integration_get_event_system(WidgetIntegration* integration) {
    return integration ? integration->event_system : NULL;
}