#ifndef EVENT_SYSTEM_H
#define EVENT_SYSTEM_H

#include <stdbool.h>
#include <stddef.h>

// Forward declaration
typedef struct EventSystem EventSystem;

// Event handler function signature
// Receives event name, data payload, and user-provided context
// Data is only valid during callback execution
typedef void (*event_handler_func)(const char* event_name, 
                                  const void* data, 
                                  size_t data_size,
                                  void* context);

// Event system lifecycle
EventSystem* event_system_create(void);
void event_system_destroy(EventSystem* system);

// Publishing events
// Copies data internally - caller can free after call returns
bool event_publish(EventSystem* system, 
                   const char* event_name, 
                   const void* data, 
                   size_t data_size);

// Subscription management
// Multiple handlers can subscribe to the same event
// Same handler can be registered multiple times with different contexts
bool event_subscribe(EventSystem* system, 
                     const char* event_name,
                     event_handler_func handler, 
                     void* context);

// Unsubscribe a specific handler
// Only removes the first matching handler/context pair
bool event_unsubscribe(EventSystem* system, 
                       const char* event_name,
                       event_handler_func handler);

// Unsubscribe all handlers for an event
bool event_unsubscribe_all(EventSystem* system, const char* event_name);

// Statistics and debugging
size_t event_system_get_subscription_count(EventSystem* system);
size_t event_system_get_event_count(EventSystem* system, const char* event_name);
bool event_system_has_subscribers(EventSystem* system, const char* event_name);

// Thread safety note: All functions are thread-safe

#endif // EVENT_SYSTEM_H