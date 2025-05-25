#ifndef STATE_STORE_H
#define STATE_STORE_H

#include <stddef.h>
#include <stdbool.h>
#include <time.h>

// Forward declaration
typedef struct StateStore StateStore;

// Data type handle (opaque, assigned at runtime)
typedef unsigned int DataType;

#define DATA_TYPE_INVALID 0  // Reserved invalid type

// Storage configuration per data type
typedef struct {
    char type_name[64];
    size_t max_items_per_key;   // Maximum versions to keep per compound key (0 = unlimited)
    time_t retention_seconds;   // How long to keep items (0 = forever)
    bool cache_enabled;         // If false, data is not stored (just passed through)
} DataTypeConfig;

// Iterator callback function type
// Return true to continue iteration, false to stop
typedef bool (*state_store_iterator)(const char* type_name, const char* id,
                                    const void* data, size_t data_size, 
                                    time_t timestamp, void* user_context);

// State store lifecycle
StateStore* state_store_create(void);
void state_store_destroy(StateStore* store);

// Configuration
bool state_store_configure_type(StateStore* store, const DataTypeConfig* config);
DataTypeConfig state_store_get_type_config(StateStore* store, const char* type_name);

// Core operations (compound key: type_name:id)
bool state_store_set(StateStore* store, const char* type_name, const char* id,
                     const void* data, size_t data_size);
void* state_store_get(StateStore* store, const char* type_name, const char* id,
                      size_t* size_out, time_t* timestamp_out);
bool state_store_has(StateStore* store, const char* type_name, const char* id);
bool state_store_remove(StateStore* store, const char* type_name, const char* id);

// Iteration and queries
bool state_store_iterate_all(StateStore* store, state_store_iterator callback, void* user_context);
bool state_store_iterate_by_type(StateStore* store, const char* type_name,
                                 state_store_iterator callback, void* user_context);
bool state_store_iterate_wildcard(StateStore* store, const char* pattern,
                                  state_store_iterator callback, void* user_context);

// Wildcard pattern examples:
// "weather_current:*"     - all weather data regardless of location
// "*:91007"              - all data types for location 91007  
// "*:*"                  - all data (same as iterate_all)

// Maintenance
size_t state_store_cleanup_expired(StateStore* store);
size_t state_store_get_total_items(StateStore* store);
size_t state_store_get_items_by_type(StateStore* store, const char* type_name);

// Thread safety note: All functions are thread-safe

#endif // STATE_STORE_H