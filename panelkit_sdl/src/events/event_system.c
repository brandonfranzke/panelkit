#include "event_system.h"
#include "event_system_typed.h"
#include "event_types.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include "core/logger.h"
#include "core/error.h"

#define MAX_EVENT_NAME_LENGTH 128
#define INITIAL_SUBSCRIPTIONS_CAPACITY 32
#define INITIAL_EVENTS_CAPACITY 16

// Subscription entry - maps handler+context to an event
typedef struct {
    char event_name[MAX_EVENT_NAME_LENGTH];
    event_handler_func handler;
    void* context;
    bool owns_context;  // True if context should be freed on unsubscribe
} Subscription;

// Event data for deferred/queued events (future enhancement)
typedef struct {
    char event_name[MAX_EVENT_NAME_LENGTH];
    void* data;
    size_t data_size;
} EventData;

// Main event system structure
struct EventSystem {
    pthread_rwlock_t lock;
    
    // Subscriptions array - simple linear search for now
    // Could optimize with hash table if needed
    Subscription* subscriptions;
    size_t num_subscriptions;
    size_t subscription_capacity;
    
    // Statistics
    size_t total_events_published;
};

EventSystem* event_system_create(void) {
    EventSystem* system = calloc(1, sizeof(EventSystem));
    if (!system) {
        log_error("Failed to allocate event system");
        return NULL;
    }
    
    // Initialize rwlock
    if (pthread_rwlock_init(&system->lock, NULL) != 0) {
        log_error("Failed to initialize event system lock");
        free(system);
        return NULL;
    }
    
    // Initialize subscriptions array
    system->subscriptions = calloc(INITIAL_SUBSCRIPTIONS_CAPACITY, sizeof(Subscription));
    if (!system->subscriptions) {
        log_error("Failed to allocate subscriptions array");
        pthread_rwlock_destroy(&system->lock);
        free(system);
        return NULL;
    }
    system->subscription_capacity = INITIAL_SUBSCRIPTIONS_CAPACITY;
    
    log_info("Event system created with capacity for %zu subscriptions", 
             system->subscription_capacity);
    return system;
}

void event_system_destroy(EventSystem* system) {
    if (!system) {
        return;
    }
    
    pthread_rwlock_wrlock(&system->lock);
    
    // Free any owned contexts (from typed event adapters)
    for (size_t i = 0; i < system->num_subscriptions; i++) {
        if (system->subscriptions[i].owns_context && system->subscriptions[i].context) {
            free(system->subscriptions[i].context);
        }
    }
    free(system->subscriptions);
    
    pthread_rwlock_unlock(&system->lock);
    pthread_rwlock_destroy(&system->lock);
    
    log_debug("Event system destroyed after %zu total events", 
              system->total_events_published);
    free(system);
}

// Internal function that allows specifying context ownership
static bool event_subscribe_internal(EventSystem* system, 
                                   const char* event_name,
                                   event_handler_func handler, 
                                   void* context,
                                   bool owns_context) {
    if (!system || !event_name || !handler) {
        log_error("Invalid parameters for event subscription");
        pk_set_last_error(PK_ERROR_NULL_PARAM);
        return false;
    }
    
    if (strlen(event_name) >= MAX_EVENT_NAME_LENGTH) {
        log_error("Event name too long: %s", event_name);
        pk_set_last_error(PK_ERROR_INVALID_PARAM);
        return false;
    }
    
    pthread_rwlock_wrlock(&system->lock);
    
    // Expand array if needed
    if (system->num_subscriptions >= system->subscription_capacity) {
        size_t new_capacity = system->subscription_capacity * 2;
        Subscription* new_subs = realloc(system->subscriptions, 
                                        new_capacity * sizeof(Subscription));
        if (!new_subs) {
            log_error("Failed to expand subscriptions array");
            pthread_rwlock_unlock(&system->lock);
            pk_set_last_error(PK_ERROR_OUT_OF_MEMORY);
            return false;
        }
        system->subscriptions = new_subs;
        system->subscription_capacity = new_capacity;
    }
    
    // Add new subscription
    Subscription* sub = &system->subscriptions[system->num_subscriptions];
    strncpy(sub->event_name, event_name, MAX_EVENT_NAME_LENGTH - 1);
    sub->event_name[MAX_EVENT_NAME_LENGTH - 1] = '\0';
    sub->handler = handler;
    sub->context = context;
    sub->owns_context = owns_context;
    
    system->num_subscriptions++;
    
    pthread_rwlock_unlock(&system->lock);
    
    log_debug("Subscribed handler %p to event '%s' (total subs: %zu, owns_context: %s)", 
              (void*)handler, event_name, system->num_subscriptions,
              owns_context ? "true" : "false");
    return true;
}

bool event_subscribe(EventSystem* system, 
                     const char* event_name,
                     event_handler_func handler, 
                     void* context) {
    // Regular subscriptions don't own their context (Pattern 4: Borrowed Reference)
    return event_subscribe_internal(system, event_name, handler, context, false);
}

bool event_unsubscribe(EventSystem* system, 
                       const char* event_name,
                       event_handler_func handler) {
    if (!system || !event_name || !handler) {
        return false;
    }
    
    pthread_rwlock_wrlock(&system->lock);
    
    // Find and remove first matching subscription
    for (size_t i = 0; i < system->num_subscriptions; i++) {
        Subscription* sub = &system->subscriptions[i];
        if (strcmp(sub->event_name, event_name) == 0 && sub->handler == handler) {
            // Free owned context if needed (Pattern 1: Parent Owns Child)
            if (sub->owns_context && sub->context) {
                free(sub->context);
            }
            
            // Move last subscription to this position
            if (i < system->num_subscriptions - 1) {
                system->subscriptions[i] = 
                    system->subscriptions[system->num_subscriptions - 1];
            }
            system->num_subscriptions--;
            
            pthread_rwlock_unlock(&system->lock);
            log_debug("Unsubscribed handler %p from event '%s'", 
                     (void*)handler, event_name);
            return true;
        }
    }
    
    pthread_rwlock_unlock(&system->lock);
    return false;
}

bool event_unsubscribe_all(EventSystem* system, const char* event_name) {
    if (!system || !event_name) {
        return false;
    }
    
    pthread_rwlock_wrlock(&system->lock);
    
    size_t removed = 0;
    size_t write_idx = 0;
    
    // Compact array, keeping non-matching subscriptions
    for (size_t read_idx = 0; read_idx < system->num_subscriptions; read_idx++) {
        if (strcmp(system->subscriptions[read_idx].event_name, event_name) != 0) {
            if (write_idx != read_idx) {
                system->subscriptions[write_idx] = system->subscriptions[read_idx];
            }
            write_idx++;
        } else {
            // Free owned context if needed (Pattern 1: Parent Owns Child)
            if (system->subscriptions[read_idx].owns_context && 
                system->subscriptions[read_idx].context) {
                free(system->subscriptions[read_idx].context);
            }
            removed++;
        }
    }
    
    system->num_subscriptions = write_idx;
    
    pthread_rwlock_unlock(&system->lock);
    
    if (removed > 0) {
        log_debug("Unsubscribed %zu handlers from event '%s'", removed, event_name);
    }
    
    return removed > 0;
}

bool event_publish(EventSystem* system, 
                   const char* event_name, 
                   const void* data, 
                   size_t data_size) {
    if (!system || !event_name || strlen(event_name) >= MAX_EVENT_NAME_LENGTH) {
        log_error("Invalid parameters for event publish");
        return false;
    }
    
    // Allow NULL data for notification-only events
    if (data_size > 0 && !data) {
        log_error("Data size specified but data is NULL");
        return false;
    }
    
    pthread_rwlock_rdlock(&system->lock);
    
    // Count matching subscriptions first
    size_t match_count = 0;
    for (size_t i = 0; i < system->num_subscriptions; i++) {
        if (strcmp(system->subscriptions[i].event_name, event_name) == 0) {
            match_count++;
        }
    }
    
    if (match_count == 0) {
        pthread_rwlock_unlock(&system->lock);
        log_debug("No subscribers for event '%s', skipping", event_name);
        return true; // Not an error - just no subscribers
    }
    
    // Collect matching subscriptions to avoid holding lock during callbacks
    Subscription* matches = malloc(match_count * sizeof(Subscription));
    if (!matches) {
        pthread_rwlock_unlock(&system->lock);
        log_error("Failed to allocate memory for event dispatch");
        return false;
    }
    
    size_t match_idx = 0;
    for (size_t i = 0; i < system->num_subscriptions; i++) {
        if (strcmp(system->subscriptions[i].event_name, event_name) == 0) {
            matches[match_idx++] = system->subscriptions[i];
        }
    }
    
    // Update statistics while we have the lock
    ((EventSystem*)system)->total_events_published++;
    
    pthread_rwlock_unlock(&system->lock);
    
    // Call handlers outside of lock to prevent deadlocks
    log_debug("Publishing event '%s' to %zu subscribers (%zu bytes)", 
              event_name, match_count, data_size);
    
    for (size_t i = 0; i < match_count; i++) {
        matches[i].handler(event_name, data, data_size, matches[i].context);
    }
    
    free(matches);
    return true;
}

// Statistics functions
size_t event_system_get_subscription_count(EventSystem* system) {
    if (!system) {
        return 0;
    }
    
    pthread_rwlock_rdlock(&system->lock);
    size_t count = system->num_subscriptions;
    pthread_rwlock_unlock(&system->lock);
    
    return count;
}

size_t event_system_get_event_count(EventSystem* system, const char* event_name) {
    if (!system || !event_name) {
        return 0;
    }
    
    pthread_rwlock_rdlock(&system->lock);
    
    size_t count = 0;
    for (size_t i = 0; i < system->num_subscriptions; i++) {
        if (strcmp(system->subscriptions[i].event_name, event_name) == 0) {
            count++;
        }
    }
    
    pthread_rwlock_unlock(&system->lock);
    return count;
}

bool event_system_has_subscribers(EventSystem* system, const char* event_name) {
    return event_system_get_event_count(system, event_name) > 0;
}

// ============================================================================
// Strongly Typed Event API Implementation
// ============================================================================

// Helper macros to reduce boilerplate
#define IMPLEMENT_TYPED_PUBLISH(name, event_name, data_type) \
    bool event_publish_##name(EventSystem* system, const data_type* data) { \
        return event_publish(system, event_name, data, sizeof(data_type)); \
    }

#define IMPLEMENT_TYPED_SUBSCRIBE(name, event_name, handler_type) \
    typedef struct { \
        handler_type typed_handler; \
        void* context; \
    } name##_adapter_data; \
    \
    static void name##_adapter(const char* event_name, const void* data, \
                              size_t data_size, void* adapter_context) { \
        name##_adapter_data* adapter = (name##_adapter_data*)adapter_context; \
        adapter->typed_handler((const void*)data, adapter->context); \
    } \
    \
    bool event_subscribe_##name(EventSystem* system, handler_type handler, void* context) { \
        name##_adapter_data* adapter = malloc(sizeof(name##_adapter_data)); \
        if (!adapter) return false; \
        adapter->typed_handler = handler; \
        adapter->context = context; \
        /* Adapter is owned by event system (Pattern 1: Parent Owns Child) */ \
        return event_subscribe_internal(system, event_name, name##_adapter, adapter, true); \
    }

// Button Pressed Event
static void button_pressed_adapter(const char* event_name, const void* data,
                                  size_t data_size, void* adapter_context) {
    typedef struct {
        button_pressed_handler typed_handler;
        void* context;
    } adapter_data;
    adapter_data* adapter = (adapter_data*)adapter_context;
    adapter->typed_handler((const ButtonEventData*)data, adapter->context);
}

bool event_publish_button_pressed(EventSystem* system, const ButtonEventData* data) {
    return event_publish(system, "ui.button_pressed", data, sizeof(ButtonEventData));
}

bool event_subscribe_button_pressed(EventSystem* system, button_pressed_handler handler, void* context) {
    typedef struct {
        button_pressed_handler typed_handler;
        void* context;
    } adapter_data;
    adapter_data* adapter = malloc(sizeof(adapter_data));
    if (!adapter) return false;
    adapter->typed_handler = handler;
    adapter->context = context;
    /* Adapter is owned by event system (Pattern 1: Parent Owns Child) */
    return event_subscribe_internal(system, "ui.button_pressed", button_pressed_adapter, adapter, true);
}

bool event_unsubscribe_button_pressed(EventSystem* system, button_pressed_handler handler) {
    // Note: This is simplified - in production you'd need to track adapters
    return event_unsubscribe(system, "ui.button_pressed", button_pressed_adapter);
}

// Page Changed Event
IMPLEMENT_TYPED_PUBLISH(page_changed, "ui.page_changed", PageChangeEventData)

static void page_changed_adapter(const char* event_name, const void* data,
                                size_t data_size, void* adapter_context) {
    typedef struct {
        page_changed_handler typed_handler;
        void* context;
    } adapter_data;
    adapter_data* adapter = (adapter_data*)adapter_context;
    adapter->typed_handler((const PageChangeEventData*)data, adapter->context);
}

bool event_subscribe_page_changed(EventSystem* system, page_changed_handler handler, void* context) {
    typedef struct {
        page_changed_handler typed_handler;
        void* context;
    } adapter_data;
    adapter_data* adapter = malloc(sizeof(adapter_data));
    if (!adapter) return false;
    adapter->typed_handler = handler;
    adapter->context = context;
    /* Adapter is owned by event system (Pattern 1: Parent Owns Child) */
    return event_subscribe_internal(system, "ui.page_changed", page_changed_adapter, adapter, true);
}

// Page Transition Event (simple int parameter)
bool event_publish_page_transition(EventSystem* system, int target_page) {
    return event_publish(system, "app.page_transition", &target_page, sizeof(int));
}

static void page_transition_adapter(const char* event_name, const void* data,
                                   size_t data_size, void* adapter_context) {
    typedef struct {
        page_transition_handler typed_handler;
        void* context;
    } adapter_data;
    adapter_data* adapter = (adapter_data*)adapter_context;
    adapter->typed_handler(*(const int*)data, adapter->context);
}

bool event_subscribe_page_transition(EventSystem* system, page_transition_handler handler, void* context) {
    typedef struct {
        page_transition_handler typed_handler;
        void* context;
    } adapter_data;
    adapter_data* adapter = malloc(sizeof(adapter_data));
    if (!adapter) return false;
    adapter->typed_handler = handler;
    adapter->context = context;
    /* Adapter is owned by event system (Pattern 1: Parent Owns Child) */
    return event_subscribe_internal(system, "app.page_transition", page_transition_adapter, adapter, true);
}

// API Refresh Requested Event
bool event_publish_api_refresh_requested(EventSystem* system, uint32_t timestamp) {
    return event_publish(system, "api.refresh_requested", &timestamp, sizeof(uint32_t));
}

static void api_refresh_requested_adapter(const char* event_name, const void* data,
                                         size_t data_size, void* adapter_context) {
    typedef struct {
        api_refresh_requested_handler typed_handler;
        void* context;
    } adapter_data;
    adapter_data* adapter = (adapter_data*)adapter_context;
    adapter->typed_handler(*(const uint32_t*)data, adapter->context);
}

bool event_subscribe_api_refresh_requested(EventSystem* system, api_refresh_requested_handler handler, void* context) {
    typedef struct {
        api_refresh_requested_handler typed_handler;
        void* context;
    } adapter_data;
    adapter_data* adapter = malloc(sizeof(adapter_data));
    if (!adapter) return false;
    adapter->typed_handler = handler;
    adapter->context = context;
    /* Adapter is owned by event system (Pattern 1: Parent Owns Child) */
    return event_subscribe_internal(system, "api.refresh_requested", api_refresh_requested_adapter, adapter, true);
}

// API State Changed Event
IMPLEMENT_TYPED_PUBLISH(api_state_changed, "api.state_changed", ApiStateChangeData)

// API User Data Updated Event
bool event_publish_api_user_data_updated(EventSystem* system, const void* user_data, size_t size) {
    return event_publish(system, "api.user_data_updated", user_data, size);
}

// API Refresh Event
IMPLEMENT_TYPED_PUBLISH(api_refresh, "system.api_refresh", ApiRefreshData)

// Weather Request Event
bool event_publish_weather_request(EventSystem* system, const char* location) {
    return event_publish(system, "weather.request", location, strlen(location) + 1);
}

// Touch Events
IMPLEMENT_TYPED_PUBLISH(touch_down, "input.touch_down", TouchEventData)
IMPLEMENT_TYPED_PUBLISH(touch_up, "input.touch_up", TouchEventData)

// Note: Additional subscribe/unsubscribe implementations would follow the same pattern
// For brevity, showing the key ones that are actually used in the codebase