/**
 * @file state_event_bridge.h
 * @brief Bridge between event system and state store
 * 
 * Automatically caches events in the state store based on configuration.
 */

#ifndef STATE_EVENT_BRIDGE_H
#define STATE_EVENT_BRIDGE_H

#include <stdbool.h>
#include <stddef.h>
#include "../core/error.h"

// Forward declarations
typedef struct StateStore StateStore;
typedef struct EventSystem EventSystem;

// Bridge configuration
typedef struct {
    bool auto_cache_all;        // Cache all events automatically
    bool use_event_as_type;     // Use event name as state type
    bool use_event_as_id;       // Use full event name as id (vs splitting)
    const char* id_separator;   // Separator for extracting id from event (default ".")
} StateEventBridgeConfig;

// Initialize the bridge between state store and event system
// This makes the state store listen to all events and cache based on config
// Returns PK_OK on success, error code on failure
PkError state_event_bridge_init(StateStore* store, 
                               EventSystem* events,
                               const StateEventBridgeConfig* config);

// Cleanup bridge (unsubscribes from events)
void state_event_bridge_cleanup(StateStore* store, EventSystem* events);

// Utility function to convert event names to state store keys
// Example: "weather.temperature" with id "91007" -> "weather_temperature:91007"
// Returns PK_OK on success, PK_ERROR_INVALID_PARAM if name would be truncated
PkError state_event_bridge_make_key(char* buffer, size_t buffer_size,
                                   const char* event_name, const char* id);

// Default configuration
StateEventBridgeConfig state_event_bridge_default_config(void);

// Subscribe to a specific event for bridging
// Returns PK_OK on success, error code on failure
PkError state_event_bridge_subscribe(EventSystem* events, const char* event_name);

#endif // STATE_EVENT_BRIDGE_H