#include "state_store.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>
#include "core/logger.h"
#include "core/error.h"

#define MAX_COMPOUND_KEY_LENGTH 192  // "type_name:id"
#define INITIAL_STORE_CAPACITY 64
#define INITIAL_TYPE_CAPACITY 16
#define MAX_STATE_ITEM_SIZE (1024 * 1024)  // 1MB max per item for safety
#define MAX_TYPE_NAME_LENGTH 64
#define MAX_ID_LENGTH 128

// Type configuration storage
typedef struct {
    char type_name[64];
    DataTypeConfig config;
} TypeConfigEntry;

// Stored data item with compound key
typedef struct {
    char compound_key[MAX_COMPOUND_KEY_LENGTH];  // "type_name:id"
    void* data;
    size_t data_size;
    time_t timestamp;
    time_t expires_at;  // 0 means never expires
} StoredItem;

// Main state store structure
struct StateStore {
    pthread_rwlock_t lock;
    
    // Type configurations
    TypeConfigEntry* type_configs;
    size_t num_type_configs;
    size_t type_config_capacity;
    
    // Stored items
    StoredItem* items;
    size_t num_items;
    size_t item_capacity;
};

// Default configuration for new types
static const DataTypeConfig DEFAULT_TYPE_CONFIG = {
    .type_name = "",            // Will be filled in
    .max_items_per_key = 1,     // Keep only latest by default
    .retention_seconds = 3600,  // 1 hour default retention
    .cache_enabled = true
};

// Forward declarations
size_t state_store_cleanup_expired(StateStore* store);

// Helper function to create compound key
static void make_compound_key(char* buffer, size_t buffer_size, 
                            const char* type_name, const char* id) {
    snprintf(buffer, buffer_size, "%s:%s", type_name, id);
}

// Helper function to split compound key
static bool split_compound_key(const char* compound_key, 
                             char* type_name, size_t type_name_size,
                             char* id, size_t id_size) {
    const char* colon = strchr(compound_key, ':');
    if (!colon) {
        return false;
    }
    
    size_t type_len = colon - compound_key;
    if (type_len >= type_name_size) {
        return false;
    }
    
    strncpy(type_name, compound_key, type_len);
    type_name[type_len] = '\0';
    
    strncpy(id, colon + 1, id_size - 1);
    id[id_size - 1] = '\0';
    
    return true;
}

StateStore* state_store_create(void) {
    StateStore* store = calloc(1, sizeof(StateStore));
    if (!store) {
        log_error("Failed to allocate state store");
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate StateStore struct");
        return NULL;
    }
    
    // Initialize pthread rwlock
    if (pthread_rwlock_init(&store->lock, NULL) != 0) {
        log_error("Failed to initialize state store lock");
        pk_set_last_error_with_context(PK_ERROR_SYSTEM,
                                       "pthread_rwlock_init failed for state store");
        free(store);
        return NULL;
    }
    
    // Initialize type configs array
    store->type_configs = calloc(INITIAL_TYPE_CAPACITY, sizeof(TypeConfigEntry));
    if (!store->type_configs) {
        log_error("Failed to allocate type configs array");
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate %zu type configs",
                                       INITIAL_TYPE_CAPACITY);
        pthread_rwlock_destroy(&store->lock);
        free(store);
        return NULL;
    }
    store->type_config_capacity = INITIAL_TYPE_CAPACITY;
    
    // Initialize items array
    store->items = calloc(INITIAL_STORE_CAPACITY, sizeof(StoredItem));
    if (!store->items) {
        log_error("Failed to allocate items array");
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate %zu state items",
                                       INITIAL_STORE_CAPACITY);
        free(store->type_configs);
        pthread_rwlock_destroy(&store->lock);
        free(store);
        return NULL;
    }
    store->item_capacity = INITIAL_STORE_CAPACITY;
    
    log_info("State store created with capacity %zu items, %zu type configs", 
             store->item_capacity, store->type_config_capacity);
    return store;
}

void state_store_destroy(StateStore* store) {
    if (!store) {
        return;
    }
    
    // Clean up all stored items
    for (size_t i = 0; i < store->num_items; i++) {
        free(store->items[i].data);
    }
    free(store->items);
    
    // Clean up type configs
    free(store->type_configs);
    
    // Destroy lock
    pthread_rwlock_destroy(&store->lock);
    
    log_info("State store destroyed (%zu items cleaned up)", store->num_items);
    free(store);
}

bool state_store_configure_type(StateStore* store, const DataTypeConfig* config) {
    if (!store || !config || !config->type_name[0]) {
        return false;
    }
    
    pthread_rwlock_wrlock(&store->lock);
    
    // Look for existing type config
    for (size_t i = 0; i < store->num_type_configs; i++) {
        if (strcmp(store->type_configs[i].type_name, config->type_name) == 0) {
            // Update existing config
            store->type_configs[i].config = *config;
            pthread_rwlock_unlock(&store->lock);
            log_debug("Updated type config for '%s'", config->type_name);
            return true;
        }
    }
    
    // Add new type config
    if (store->num_type_configs >= store->type_config_capacity) {
        // Expand array
        size_t new_capacity = store->type_config_capacity * 2;
        TypeConfigEntry* new_configs = realloc(store->type_configs,
                                              new_capacity * sizeof(TypeConfigEntry));
        if (!new_configs) {
            pthread_rwlock_unlock(&store->lock);
            return false;
        }
        store->type_configs = new_configs;
        store->type_config_capacity = new_capacity;
    }
    
    TypeConfigEntry* entry = &store->type_configs[store->num_type_configs];
    strncpy(entry->type_name, config->type_name, sizeof(entry->type_name) - 1);
    entry->config = *config;
    store->num_type_configs++;
    
    pthread_rwlock_unlock(&store->lock);
    log_info("Added type config for '%s'", config->type_name);
    return true;
}

DataTypeConfig state_store_get_type_config(StateStore* store, const char* type_name) {
    DataTypeConfig config = DEFAULT_TYPE_CONFIG;
    
    if (!store || !type_name) {
        return config;
    }
    
    pthread_rwlock_rdlock(&store->lock);
    
    for (size_t i = 0; i < store->num_type_configs; i++) {
        if (strcmp(store->type_configs[i].type_name, type_name) == 0) {
            config = store->type_configs[i].config;
            pthread_rwlock_unlock(&store->lock);
            return config;
        }
    }
    
    pthread_rwlock_unlock(&store->lock);
    
    // Return default config with type name filled in
    strncpy(config.type_name, type_name, sizeof(config.type_name) - 1);
    return config;
}

// Core data operations
bool state_store_set(StateStore* store, const char* type_name, const char* id,
                     const void* data, size_t data_size) {
    if (!store || !type_name || !id || !data || data_size == 0) {
        log_error("Invalid parameters for state_store_set");
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "state_store_set: store=%p, type=%s, id=%s, data=%p, size=%zu",
                                       store, type_name ? type_name : "NULL",
                                       id ? id : "NULL", data, data_size);
        return false;
    }
    
    // Phase 3: Validate input sizes
    if (strlen(type_name) >= MAX_TYPE_NAME_LENGTH) {
        log_error("Type name too long: %s", type_name);
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Type name '%s' exceeds maximum length %d",
                                       type_name, MAX_TYPE_NAME_LENGTH);
        return false;
    }
    
    if (strlen(id) >= MAX_ID_LENGTH) {
        log_error("ID too long: %s", id);
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "ID '%s' exceeds maximum length %d",
                                       id, MAX_ID_LENGTH);
        return false;
    }
    
    if (data_size > MAX_STATE_ITEM_SIZE) {
        log_error("Data size too large: %zu bytes", data_size);
        pk_set_last_error_with_context(PK_ERROR_RESOURCE_LIMIT,
                                       "Data size %zu exceeds maximum %d bytes",
                                       data_size, MAX_STATE_ITEM_SIZE);
        return false;
    }
    
    char compound_key[MAX_COMPOUND_KEY_LENGTH];
    make_compound_key(compound_key, sizeof(compound_key), type_name, id);
    
    pthread_rwlock_wrlock(&store->lock);
    
    // Check if caching is enabled for this type
    DataTypeConfig config = state_store_get_type_config(store, type_name);
    if (!config.cache_enabled) {
        pthread_rwlock_unlock(&store->lock);
        log_debug("Caching disabled for type '%s', data not stored", type_name);
        return true; // Success but not stored
    }
    
    time_t now = time(NULL);
    time_t expires_at = (config.retention_seconds > 0) ? 
                       now + config.retention_seconds : 0;
    
    // Look for existing item with same compound key
    for (size_t i = 0; i < store->num_items; i++) {
        if (strcmp(store->items[i].compound_key, compound_key) == 0) {
            // Replace existing item
            StoredItem* item = &store->items[i];
            
            // Free old data and allocate new
            free(item->data);
            item->data = malloc(data_size);
            if (!item->data) {
                log_error("Failed to allocate data for existing item");
                pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                               "Failed to allocate %zu bytes for state update of '%s'",
                                               data_size, compound_key);
                pthread_rwlock_unlock(&store->lock);
                return false;
            }
            
            memcpy(item->data, data, data_size);
            item->data_size = data_size;
            item->timestamp = now;
            item->expires_at = expires_at;
            
            pthread_rwlock_unlock(&store->lock);
            log_debug("Updated existing item: %s", compound_key);
            return true;
        }
    }
    
    // Expand items array if needed
    if (store->num_items >= store->item_capacity) {
        size_t new_capacity = store->item_capacity * 2;
        StoredItem* new_items = realloc(store->items, new_capacity * sizeof(StoredItem));
        if (!new_items) {
            log_error("Failed to expand items array");
            pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                           "Failed to expand state items from %zu to %zu",
                                           store->item_capacity, new_capacity);
            pthread_rwlock_unlock(&store->lock);
            return false;
        }
        store->items = new_items;
        store->item_capacity = new_capacity;
    }
    
    // Add new item
    StoredItem* new_item = &store->items[store->num_items];
    
    strncpy(new_item->compound_key, compound_key, MAX_COMPOUND_KEY_LENGTH - 1);
    new_item->compound_key[MAX_COMPOUND_KEY_LENGTH - 1] = '\0';
    
    new_item->data = malloc(data_size);
    if (!new_item->data) {
        log_error("Failed to allocate data for new item");
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate %zu bytes for new state item '%s'",
                                       data_size, compound_key);
        pthread_rwlock_unlock(&store->lock);
        return false;
    }
    
    memcpy(new_item->data, data, data_size);
    new_item->data_size = data_size;
    new_item->timestamp = now;
    new_item->expires_at = expires_at;
    
    store->num_items++;
    
    pthread_rwlock_unlock(&store->lock);
    
    log_debug("Added new item: %s (%zu bytes)", compound_key, data_size);
    
    // Phase 3: Opportunistic garbage collection of expired items
    // Only run occasionally to avoid overhead
    static int set_count = 0;
    if (++set_count % 100 == 0) {
        state_store_cleanup_expired(store);
    }
    
    return true;
}

void* state_store_get(StateStore* store, const char* type_name, const char* id,
                      size_t* size_out, time_t* timestamp_out) {
    if (!store || !type_name || !id) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "state_store_get: store=%p, type=%s, id=%s",
                                       store, type_name ? type_name : "NULL",
                                       id ? id : "NULL");
        return NULL;
    }
    
    char compound_key[MAX_COMPOUND_KEY_LENGTH];
    make_compound_key(compound_key, sizeof(compound_key), type_name, id);
    
    pthread_rwlock_rdlock(&store->lock);
    
    for (size_t i = 0; i < store->num_items; i++) {
        if (strcmp(store->items[i].compound_key, compound_key) == 0) {
            StoredItem* item = &store->items[i];
            
            // Check if expired
            time_t now = time(NULL);
            if (item->expires_at > 0 && now > item->expires_at) {
                pthread_rwlock_unlock(&store->lock);
                log_debug("Item expired: %s", compound_key);
                return NULL;
            }
            
            // Allocate copy of data to return (caller must free)
            void* data_copy = malloc(item->data_size);
            if (!data_copy) {
                pthread_rwlock_unlock(&store->lock);
                log_error("Failed to allocate copy of data");
                pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                               "Failed to allocate %zu bytes for state data copy of '%s'",
                                               item->data_size, compound_key);
                return NULL;
            }
            
            memcpy(data_copy, item->data, item->data_size);
            
            if (size_out) {
                *size_out = item->data_size;
            }
            if (timestamp_out) {
                *timestamp_out = item->timestamp;
            }
            
            pthread_rwlock_unlock(&store->lock);
            // log_debug("Retrieved item: %s (%zu bytes)", compound_key, item->data_size); // Too verbose
            return data_copy;
        }
    }
    
    pthread_rwlock_unlock(&store->lock);
    pk_set_last_error_with_context(PK_ERROR_NOT_FOUND,
                                   "State item '%s' not found", compound_key);
    return NULL;
}

bool state_store_has(StateStore* store, const char* type_name, const char* id) {
    if (!store || !type_name || !id) {
        return false;
    }
    
    char compound_key[MAX_COMPOUND_KEY_LENGTH];
    make_compound_key(compound_key, sizeof(compound_key), type_name, id);
    
    pthread_rwlock_rdlock(&store->lock);
    
    for (size_t i = 0; i < store->num_items; i++) {
        if (strcmp(store->items[i].compound_key, compound_key) == 0) {
            // Check expiration
            time_t now = time(NULL);
            if (store->items[i].expires_at > 0 && now > store->items[i].expires_at) {
                pthread_rwlock_unlock(&store->lock);
                return false;  // Expired
            }
            pthread_rwlock_unlock(&store->lock);
            return true;
        }
    }
    
    pthread_rwlock_unlock(&store->lock);
    return false;
}

bool state_store_remove(StateStore* store, const char* type_name, const char* id) {
    if (!store || !type_name || !id) {
        return false;
    }
    
    char compound_key[MAX_COMPOUND_KEY_LENGTH];
    make_compound_key(compound_key, sizeof(compound_key), type_name, id);
    
    pthread_rwlock_wrlock(&store->lock);
    
    for (size_t i = 0; i < store->num_items; i++) {
        if (strcmp(store->items[i].compound_key, compound_key) == 0) {
            // Free data
            free(store->items[i].data);
            
            // Move last item to this position if not already last
            if (i < store->num_items - 1) {
                store->items[i] = store->items[store->num_items - 1];
            }
            store->num_items--;
            
            pthread_rwlock_unlock(&store->lock);
            log_debug("Removed item: %s", compound_key);
            return true;
        }
    }
    
    pthread_rwlock_unlock(&store->lock);
    return false;
}

bool state_store_clear_type(StateStore* store, const char* type_name) {
    if (!store || !type_name) {
        return false;
    }
    
    pthread_rwlock_wrlock(&store->lock);
    
    char type_prefix[MAX_TYPE_NAME_LENGTH + 2];
    snprintf(type_prefix, sizeof(type_prefix), "%s:", type_name);
    size_t prefix_len = strlen(type_prefix);
    
    size_t removed = 0;
    size_t write_idx = 0;
    
    // Remove all items matching type prefix
    for (size_t read_idx = 0; read_idx < store->num_items; read_idx++) {
        if (strncmp(store->items[read_idx].compound_key, type_prefix, prefix_len) == 0) {
            // Item matches type - free and skip
            free(store->items[read_idx].data);
            removed++;
        } else {
            // Keep item - move to write position
            if (write_idx != read_idx) {
                store->items[write_idx] = store->items[read_idx];
            }
            write_idx++;
        }
    }
    
    store->num_items = write_idx;
    pthread_rwlock_unlock(&store->lock);
    
    if (removed > 0) {
        log_info("Cleared %zu items of type '%s'", removed, type_name);
    }
    return removed > 0;
}

void state_store_clear_all(StateStore* store) {
    if (!store) {
        return;
    }
    
    pthread_rwlock_wrlock(&store->lock);
    
    // Free all data
    for (size_t i = 0; i < store->num_items; i++) {
        free(store->items[i].data);
    }
    
    size_t count = store->num_items;
    store->num_items = 0;
    
    pthread_rwlock_unlock(&store->lock);
    log_info("Cleared all %zu items from state store", count);
}

// Iteration
bool state_store_iterate(StateStore* store, const char* type_name,
                        state_store_iterator iterator, void* context) {
    if (!store || !iterator) {
        return false;
    }
    
    pthread_rwlock_rdlock(&store->lock);
    
    // If type_name specified, build prefix to match
    char type_prefix[MAX_TYPE_NAME_LENGTH + 2];
    size_t prefix_len = 0;
    if (type_name) {
        snprintf(type_prefix, sizeof(type_prefix), "%s:", type_name);
        prefix_len = strlen(type_prefix);
    }
    
    bool continue_iteration = true;
    time_t now = time(NULL);
    
    for (size_t i = 0; i < store->num_items && continue_iteration; i++) {
        StoredItem* item = &store->items[i];
        
        // Skip if doesn't match type filter
        if (type_name && strncmp(item->compound_key, type_prefix, prefix_len) != 0) {
            continue;
        }
        
        // Skip if expired
        if (item->expires_at > 0 && now > item->expires_at) {
            continue;
        }
        
        // Split compound key
        char item_type[MAX_TYPE_NAME_LENGTH];
        char item_id[MAX_ID_LENGTH];
        if (!split_compound_key(item->compound_key, 
                              item_type, sizeof(item_type),
                              item_id, sizeof(item_id))) {
            continue;
        }
        
        // Call iterator
        continue_iteration = iterator(item_type, item_id, 
                                    item->data, item->data_size,
                                    item->timestamp, context);
    }
    
    pthread_rwlock_unlock(&store->lock);
    return true;
}

// Statistics
size_t state_store_get_item_count(StateStore* store, const char* type_name) {
    if (!store) {
        return 0;
    }
    
    pthread_rwlock_rdlock(&store->lock);
    
    if (!type_name) {
        // Count all non-expired items
        size_t count = 0;
        time_t now = time(NULL);
        for (size_t i = 0; i < store->num_items; i++) {
            if (store->items[i].expires_at == 0 || now <= store->items[i].expires_at) {
                count++;
            }
        }
        pthread_rwlock_unlock(&store->lock);
        return count;
    }
    
    // Count items of specific type
    char type_prefix[MAX_TYPE_NAME_LENGTH + 2];
    snprintf(type_prefix, sizeof(type_prefix), "%s:", type_name);
    size_t prefix_len = strlen(type_prefix);
    
    size_t count = 0;
    time_t now = time(NULL);
    
    for (size_t i = 0; i < store->num_items; i++) {
        if (strncmp(store->items[i].compound_key, type_prefix, prefix_len) == 0) {
            if (store->items[i].expires_at == 0 || now <= store->items[i].expires_at) {
                count++;
            }
        }
    }
    
    pthread_rwlock_unlock(&store->lock);
    return count;
}

// Phase 3: Remove expired items
size_t state_store_cleanup_expired(StateStore* store) {
    if (!store) return 0;
    
    pthread_rwlock_wrlock(&store->lock);
    
    time_t now = time(NULL);
    size_t removed = 0;
    size_t write_idx = 0;
    
    // Compact array, removing expired items
    for (size_t read_idx = 0; read_idx < store->num_items; read_idx++) {
        StoredItem* item = &store->items[read_idx];
        
        if (item->expires_at > 0 && now > item->expires_at) {
            // Item expired - free and skip
            free(item->data);
            removed++;
            log_debug("Cleaned up expired item: %s", item->compound_key);
        } else {
            // Keep item - move to write position if needed
            if (write_idx != read_idx) {
                store->items[write_idx] = store->items[read_idx];
            }
            write_idx++;
        }
    }
    
    store->num_items = write_idx;
    pthread_rwlock_unlock(&store->lock);
    
    if (removed > 0) {
        log_info("Cleaned up %zu expired state items", removed);
    }
    
    return removed;
}