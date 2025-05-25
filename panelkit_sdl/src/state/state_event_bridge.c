#include "state_event_bridge.h"
#include "state_store.h"
#include "../events/event_system.h"
#include "../core/logger.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Bridge context passed to event handler
typedef struct {
    StateStore* store;
    StateEventBridgeConfig config;
} BridgeContext;

// Global bridge context (single bridge per process)
static BridgeContext* g_bridge_context = NULL;

// Event handler that stores data in state store
static void bridge_event_handler(const char* event_name, 
                               const void* data, 
                               size_t data_size,
                               void* context) {
    BridgeContext* bridge = (BridgeContext*)context;
    if (!bridge || !bridge->store) {
        log_error("Invalid bridge context in event handler");
        return;
    }
    
    // Determine how to map event to state store key
    char type_name[128];
    char id[128] = "default";
    
    if (bridge->config.use_event_as_type) {
        // Use full event name as type
        strncpy(type_name, event_name, sizeof(type_name) - 1);
        type_name[sizeof(type_name) - 1] = '\0';
        
        // Replace dots with underscores for state store compatibility
        for (char* p = type_name; *p; p++) {
            if (*p == '.') *p = '_';
        }
    } else {
        // Extract first part of event name as type
        const char* dot = strchr(event_name, '.');
        if (dot) {
            size_t len = dot - event_name;
            if (len >= sizeof(type_name)) len = sizeof(type_name) - 1;
            strncpy(type_name, event_name, len);
            type_name[len] = '\0';
        } else {
            strncpy(type_name, event_name, sizeof(type_name) - 1);
            type_name[sizeof(type_name) - 1] = '\0';
        }
    }
    
    // Check if we should cache this type
    DataTypeConfig type_config = state_store_get_type_config(bridge->store, type_name);
    if (!type_config.cache_enabled && !bridge->config.auto_cache_all) {
        log_debug("Caching disabled for event type '%s', not storing", type_name);
        return;
    }
    
    // Store the data
    if (!state_store_set(bridge->store, type_name, id, data, data_size)) {
        log_error("Failed to store event data for '%s'", event_name);
    } else {
        log_debug("Cached event '%s' as '%s:%s' (%zu bytes)", 
                 event_name, type_name, id, data_size);
    }
}

bool state_event_bridge_init(StateStore* store, 
                            EventSystem* events,
                            const StateEventBridgeConfig* config) {
    if (!store || !events) {
        log_error("Invalid parameters for state-event bridge init");
        return false;
    }
    
    // Clean up any existing bridge
    if (g_bridge_context) {
        state_event_bridge_cleanup(store, events);
    }
    
    // Create bridge context
    g_bridge_context = malloc(sizeof(BridgeContext));
    if (!g_bridge_context) {
        log_error("Failed to allocate bridge context");
        return false;
    }
    
    g_bridge_context->store = store;
    if (config) {
        g_bridge_context->config = *config;
    } else {
        g_bridge_context->config = state_event_bridge_default_config();
    }
    
    // Subscribe to all events using wildcard pattern
    // For now, we'll need to subscribe to specific events
    // TODO: Implement wildcard subscription in event system
    
    log_info("State-event bridge initialized (auto_cache=%s, event_as_type=%s)",
             g_bridge_context->config.auto_cache_all ? "yes" : "no",
             g_bridge_context->config.use_event_as_type ? "yes" : "no");
    
    return true;
}

void state_event_bridge_cleanup(StateStore* store, EventSystem* events) {
    if (!g_bridge_context) {
        return;
    }
    
    // TODO: Unsubscribe from all events
    
    free(g_bridge_context);
    g_bridge_context = NULL;
    
    log_info("State-event bridge cleaned up");
}

void state_event_bridge_make_key(char* buffer, size_t buffer_size,
                                const char* event_name, const char* id) {
    if (!buffer || !event_name || !id) {
        return;
    }
    
    // Convert event name to state type (replace dots with underscores)
    char type_name[128];
    strncpy(type_name, event_name, sizeof(type_name) - 1);
    type_name[sizeof(type_name) - 1] = '\0';
    
    for (char* p = type_name; *p; p++) {
        if (*p == '.') *p = '_';
    }
    
    // Create compound key
    snprintf(buffer, buffer_size, "%s:%s", type_name, id);
}

StateEventBridgeConfig state_event_bridge_default_config(void) {
    StateEventBridgeConfig config = {
        .auto_cache_all = false,      // Only cache configured types
        .use_event_as_type = true,    // Full event name as type
        .use_event_as_id = false,     // Need separate ID
        .id_separator = "."           // Split on dots
    };
    return config;
}

// Helper function to subscribe to specific events
bool state_event_bridge_subscribe(EventSystem* events, const char* event_name) {
    if (!g_bridge_context || !events || !event_name) {
        log_error("Cannot subscribe: bridge not initialized");
        return false;
    }
    
    bool success = event_subscribe(events, event_name, 
                                  bridge_event_handler, g_bridge_context);
    if (success) {
        log_info("State bridge subscribed to event '%s'", event_name);
    }
    
    return success;
}