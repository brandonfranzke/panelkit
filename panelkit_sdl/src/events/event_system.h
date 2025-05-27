#ifndef EVENT_SYSTEM_H
#define EVENT_SYSTEM_H

#include <stdbool.h>
#include <stddef.h>
#include "../core/error.h"

/** Opaque event system handle */
typedef struct EventSystem EventSystem;

/**
 * Event handler function signature.
 * 
 * @param event_name Name of the event being fired (borrowed reference)
 * @param data Event data payload (borrowed reference, only valid during callback)
 * @param data_size Size of the data payload in bytes
 * @param context User-provided context from subscription (optional)
 */
typedef void (*event_handler_func)(const char* event_name, 
                                  const void* data, 
                                  size_t data_size,
                                  void* context);

// Event system lifecycle

/**
 * Create a new event system.
 * 
 * @return New event system or NULL on error (caller owns)
 * @note Thread-safe after creation
 */
EventSystem* event_system_create(void);

/**
 * Destroy an event system.
 * 
 * @param system System to destroy (can be NULL)
 * @note Unsubscribes all handlers before destruction
 */
void event_system_destroy(EventSystem* system);

// Publishing events

/**
 * Emit an event to all subscribers.
 * 
 * @param system Event system (required)
 * @param event_name Event identifier (required)
 * @param data Event data payload (can be NULL)
 * @param data_size Size of data in bytes (0 if data is NULL)
 * @return PK_OK on success, error code on failure
 * @note Data is copied internally - caller can free after return
 * @note Handlers are called synchronously in subscription order
 * @note If a handler fails, event continues to other handlers
 */
PkError event_emit(EventSystem* system, 
                   const char* event_name, 
                   const void* data, 
                   size_t data_size);

/**
 * Publish an event (compatibility wrapper).
 * @deprecated Use event_emit instead
 */
bool event_publish(EventSystem* system, 
                   const char* event_name, 
                   const void* data, 
                   size_t data_size);

// Subscription management

/**
 * Subscribe to an event.
 * 
 * @param system Event system (required)
 * @param event_name Event identifier to listen for (required)
 * @param handler Callback function (required)
 * @param context User data passed to handler (can be NULL)
 * @return true on success, false on error
 * @note Multiple handlers can subscribe to same event
 * @note Same handler can be registered multiple times
 */
bool event_subscribe(EventSystem* system, 
                     const char* event_name,
                     event_handler_func handler, 
                     void* context);

/**
 * Unsubscribe a handler from an event.
 * 
 * @param system Event system (required)
 * @param event_name Event identifier (required)
 * @param handler Handler to remove (required)
 * @return true if found and removed, false otherwise
 * @note Only removes first matching handler/context pair
 */
bool event_unsubscribe(EventSystem* system, 
                       const char* event_name,
                       event_handler_func handler);

/**
 * Unsubscribe all handlers from an event.
 * 
 * @param system Event system (required)
 * @param event_name Event identifier (required)
 * @return true if any handlers removed, false otherwise
 */
bool event_unsubscribe_all(EventSystem* system, const char* event_name);

// Statistics and debugging

/**
 * Get total number of active subscriptions.
 * 
 * @param system Event system (required)
 * @return Total subscription count across all events
 */
size_t event_system_get_subscription_count(EventSystem* system);

/**
 * Get number of handlers for a specific event.
 * 
 * @param system Event system (required)
 * @param event_name Event identifier (required)
 * @return Number of handlers subscribed to event
 */
size_t event_system_get_event_count(EventSystem* system, const char* event_name);

/**
 * Check if an event has any subscribers.
 * 
 * @param system Event system (required)
 * @param event_name Event identifier (required)
 * @return true if event has subscribers, false otherwise
 */
bool event_system_has_subscribers(EventSystem* system, const char* event_name);

/**
 * @note Thread Safety: All functions are thread-safe.
 *       The event system uses internal locking to ensure safe
 *       concurrent access from multiple threads.
 */

#endif // EVENT_SYSTEM_H