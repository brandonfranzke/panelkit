#include "event_system.h"
#include "../core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_EVENT_NAME_LENGTH 128
#define INITIAL_SUBSCRIPTIONS_CAPACITY 32
#define INITIAL_EVENTS_CAPACITY 16

// Subscription entry - maps handler+context to an event
typedef struct {
    char event_name[MAX_EVENT_NAME_LENGTH];
    event_handler_func handler;
    void* context;
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
    
    // No need to free individual subscriptions - they don't own any memory
    free(system->subscriptions);
    
    pthread_rwlock_unlock(&system->lock);
    pthread_rwlock_destroy(&system->lock);
    
    log_debug("Event system destroyed after %zu total events", 
              system->total_events_published);
    free(system);
}

bool event_subscribe(EventSystem* system, 
                     const char* event_name,
                     event_handler_func handler, 
                     void* context) {
    if (!system || !event_name || !handler || 
        strlen(event_name) >= MAX_EVENT_NAME_LENGTH) {
        log_error("Invalid parameters for event subscription");
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
    
    system->num_subscriptions++;
    
    pthread_rwlock_unlock(&system->lock);
    
    log_debug("Subscribed handler %p to event '%s' (total subs: %zu)", 
              (void*)handler, event_name, system->num_subscriptions);
    return true;
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