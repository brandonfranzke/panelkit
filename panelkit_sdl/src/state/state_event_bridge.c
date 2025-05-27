#include "state_event_bridge.h"
#include "state_store.h"
#include "../events/event_system.h"
#include "../core/logger.h"
#include "../core/error.h"
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
        // Can't use pk_set_last_error here as we're in event handler callback
        return;
    }
    
    // Determine how to map event to state store key
    char type_name[128];
    char id[128] = "default";
    
    if (bridge->config.use_event_as_type) {
        // Use full event name as type
        size_t event_len = strlen(event_name);
        if (event_len >= sizeof(type_name)) {
            log_error("Event name too long for state storage: '%s' (%zu chars, max %zu)",
                     event_name, event_len, sizeof(type_name) - 1);
            return;
        }
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
        log_error("Failed to store event data for '%s' (type='%s', id='%s', size=%zu)",
                 event_name, type_name, id, data_size);
        // Continue processing - non-fatal error
    } else {
        log_debug("Cached event '%s' as '%s:%s' (%zu bytes)", 
                 event_name, type_name, id, data_size);
    }
}

PkError state_event_bridge_init(StateStore* store, 
                               EventSystem* events,
                               const StateEventBridgeConfig* config) {
    if (!store || !events) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
            "state_event_bridge_init: store=%p, events=%p", store, events);
        return PK_ERROR_NULL_PARAM;
    }
    
    // Clean up any existing bridge
    if (g_bridge_context) {
        state_event_bridge_cleanup(store, events);
    }
    
    // Create bridge context
    g_bridge_context = malloc(sizeof(BridgeContext));
    if (!g_bridge_context) {
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
            "state_event_bridge_init: Failed to allocate bridge context (%zu bytes)",
            sizeof(BridgeContext));
        return PK_ERROR_OUT_OF_MEMORY;
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
    
    return PK_OK;
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

PkError state_event_bridge_make_key(char* buffer, size_t buffer_size,
                                   const char* event_name, const char* id) {
    if (!buffer || !event_name || !id) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
            "state_event_bridge_make_key: buffer=%p, event_name=%p, id=%p",
            buffer, event_name, id);
        return PK_ERROR_NULL_PARAM;
    }
    
    // Convert event name to state type (replace dots with underscores)
    char type_name[128];
    size_t event_len = strlen(event_name);
    if (event_len >= sizeof(type_name)) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
            "state_event_bridge_make_key: Event name too long '%s' (%zu chars, max %zu)",
            event_name, event_len, sizeof(type_name) - 1);
        return PK_ERROR_INVALID_PARAM;
    }
    strncpy(type_name, event_name, sizeof(type_name) - 1);
    type_name[sizeof(type_name) - 1] = '\0';
    
    for (char* p = type_name; *p; p++) {
        if (*p == '.') *p = '_';
    }
    
    // Create compound key
    int result = snprintf(buffer, buffer_size, "%s:%s", type_name, id);
    if (result < 0 || (size_t)result >= buffer_size) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
            "state_event_bridge_make_key: Buffer too small for key '%s:%s' (need %d, have %zu)",
            type_name, id, result, buffer_size);
        return PK_ERROR_INVALID_PARAM;
    }
    
    return PK_OK;
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
PkError state_event_bridge_subscribe(EventSystem* events, const char* event_name) {
    if (!g_bridge_context) {
        pk_set_last_error_with_context(PK_ERROR_NOT_INITIALIZED,
            "state_event_bridge_subscribe: Bridge not initialized");
        return PK_ERROR_NOT_INITIALIZED;
    }
    if (!events || !event_name) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
            "state_event_bridge_subscribe: events=%p, event_name=%p",
            events, event_name);
        return PK_ERROR_NULL_PARAM;
    }
    
    bool success = event_subscribe(events, event_name, 
                                  bridge_event_handler, g_bridge_context);
    if (!success) {
        PkError last_error = pk_get_last_error();
        if (last_error != PK_OK) {
            // Event system set a specific error
            return last_error;
        }
        // Generic subscription failure
        pk_set_last_error_with_context(PK_ERROR_SYSTEM,
            "state_event_bridge_subscribe: Failed to subscribe to event '%s'",
            event_name);
        return PK_ERROR_SYSTEM;
    }
    
    log_info("State bridge subscribed to event '%s'", event_name);
    return PK_OK;
}