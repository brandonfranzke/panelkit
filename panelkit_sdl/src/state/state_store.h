/**
 * @file state_store.h
 * @brief Key-value state storage system
 * 
 * Thread-safe storage for application state with namespace support,
 * TTL, and change notifications.
 */

#ifndef STATE_STORE_H
#define STATE_STORE_H

#include <stddef.h>
#include <stdbool.h>
#include <time.h>

/** Opaque state store handle */
typedef struct StateStore StateStore;

/** Data type handle (opaque, assigned at runtime) */
typedef unsigned int DataType;

/** Reserved invalid type constant */
#define DATA_TYPE_INVALID 0

/**
 * Storage configuration for a data type.
 * Controls retention, versioning, and caching behavior.
 */
typedef struct {
    char type_name[64];         /**< Type identifier (max 63 chars) */
    size_t max_items_per_key;   /**< Max versions per compound key (0=unlimited) */
    time_t retention_seconds;   /**< Retention period in seconds (0=forever) */
    bool cache_enabled;         /**< If false, data passes through without storage */
} DataTypeConfig;

/**
 * Iterator callback for state store traversal.
 * 
 * @param type_name Data type name (borrowed reference)
 * @param id Item identifier within type (borrowed reference)
 * @param data Item data payload (borrowed reference, only valid during callback)
 * @param data_size Size of data in bytes
 * @param timestamp When item was stored
 * @param user_context User-provided context (optional)
 * @return true to continue iteration, false to stop
 */
typedef bool (*state_store_iterator)(const char* type_name, const char* id,
                                    const void* data, size_t data_size, 
                                    time_t timestamp, void* user_context);

// State store lifecycle

/**
 * Create a new state store.
 * 
 * @return New state store or NULL on error (caller owns)
 * @note Thread-safe after creation
 */
StateStore* state_store_create(void);

/**
 * Destroy a state store.
 * 
 * @param store Store to destroy (can be NULL)
 * @note Frees all stored data
 */
void state_store_destroy(StateStore* store);

// Configuration

/**
 * Configure storage behavior for a data type.
 * 
 * @param store State store (required)
 * @param config Type configuration (required)
 * @return true on success, false on error
 * @note Creates new type or updates existing configuration
 */
bool state_store_configure_type(StateStore* store, const DataTypeConfig* config);

/**
 * Get configuration for a data type.
 * 
 * @param store State store (required)
 * @param type_name Type identifier (required)
 * @return Type configuration (zeroed if type not found)
 */
DataTypeConfig state_store_get_type_config(StateStore* store, const char* type_name);

// Core operations

/**
 * Store data with compound key (type_name:id).
 * 
 * @param store State store (required)
 * @param type_name Data type identifier (required)
 * @param id Item identifier within type (required)
 * @param data Data payload (required)
 * @param data_size Size of data in bytes
 * @return true on success, false on error
 * @note Data is copied internally - caller can free after return
 */
bool state_store_set(StateStore* store, const char* type_name, const char* id,
                     const void* data, size_t data_size);

/**
 * Retrieve data by compound key.
 * 
 * @param store State store (required)
 * @param type_name Data type identifier (required)
 * @param id Item identifier within type (required)
 * @param size_out Receives data size (can be NULL)
 * @param timestamp_out Receives storage timestamp (can be NULL)
 * @return Data pointer or NULL if not found (caller must free)
 * @note Returns copy of data - caller owns memory
 */
void* state_store_get(StateStore* store, const char* type_name, const char* id,
                      size_t* size_out, time_t* timestamp_out);

/**
 * Check if data exists for compound key.
 * 
 * @param store State store (required)
 * @param type_name Data type identifier (required)
 * @param id Item identifier within type (required)
 * @return true if data exists, false otherwise
 */
bool state_store_has(StateStore* store, const char* type_name, const char* id);

/**
 * Remove data by compound key.
 * 
 * @param store State store (required)
 * @param type_name Data type identifier (required)
 * @param id Item identifier within type (required)
 * @return true if removed, false if not found
 */
bool state_store_remove(StateStore* store, const char* type_name, const char* id);

// Iteration and queries

/**
 * Iterate over all items in the store.
 * 
 * @param store State store (required)
 * @param callback Iterator function (required)
 * @param user_context Context passed to callback (can be NULL)
 * @return true if iteration completed, false if stopped early
 */
bool state_store_iterate_all(StateStore* store, state_store_iterator callback, void* user_context);

/**
 * Iterate over items of a specific type.
 * 
 * @param store State store (required)
 * @param type_name Type to iterate (required)
 * @param callback Iterator function (required)
 * @param user_context Context passed to callback (can be NULL)
 * @return true if iteration completed, false if stopped early
 */
bool state_store_iterate_by_type(StateStore* store, const char* type_name,
                                 state_store_iterator callback, void* user_context);

/**
 * Iterate over items matching wildcard pattern.
 * 
 * @param store State store (required)
 * @param pattern Wildcard pattern (required)
 * @param callback Iterator function (required)
 * @param user_context Context passed to callback (can be NULL)
 * @return true if iteration completed, false if stopped early
 * @note Pattern examples:
 *       - "weather_current:*" - all weather data
 *       - "*:91007" - all types for location 91007
 *       - "*:*" - all data (same as iterate_all)
 */
bool state_store_iterate_wildcard(StateStore* store, const char* pattern,
                                  state_store_iterator callback, void* user_context);

// Maintenance

/**
 * Remove expired items based on retention settings.
 * 
 * @param store State store (required)
 * @return Number of items removed
 */
size_t state_store_cleanup_expired(StateStore* store);

/**
 * Get total number of items in store.
 * 
 * @param store State store (required)
 * @return Total item count
 */
size_t state_store_get_total_items(StateStore* store);

/**
 * Get number of items for a specific type.
 * 
 * @param store State store (required)
 * @param type_name Type identifier (required)
 * @return Item count for type
 */
size_t state_store_get_items_by_type(StateStore* store, const char* type_name);

/**
 * @note Thread Safety: All functions are thread-safe.
 *       The state store uses internal locking to ensure safe
 *       concurrent access from multiple threads.
 */

#endif // STATE_STORE_H