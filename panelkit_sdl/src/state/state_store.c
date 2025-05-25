#include "state_store.h"
#include "../core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>

#define MAX_COMPOUND_KEY_LENGTH 192  // "type_name:id"
#define INITIAL_STORE_CAPACITY 64
#define INITIAL_TYPE_CAPACITY 16

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
        return NULL;
    }
    
    // Initialize pthread rwlock
    if (pthread_rwlock_init(&store->lock, NULL) != 0) {
        log_error("Failed to initialize state store lock");
        free(store);
        return NULL;
    }
    
    // Initialize type configs array
    store->type_configs = calloc(INITIAL_TYPE_CAPACITY, sizeof(TypeConfigEntry));
    if (!store->type_configs) {
        log_error("Failed to allocate type configs array");
        pthread_rwlock_destroy(&store->lock);
        free(store);
        return NULL;
    }
    store->type_config_capacity = INITIAL_TYPE_CAPACITY;
    
    // Initialize items array
    store->items = calloc(INITIAL_STORE_CAPACITY, sizeof(StoredItem));
    if (!store->items) {
        log_error("Failed to allocate items array");
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
    
    pthread_rwlock_wrlock(&store->lock);
    
    // Free all stored data
    for (size_t i = 0; i < store->num_items; i++) {
        if (store->items[i].data) {
            free(store->items[i].data);
        }
    }
    
    free(store->items);
    free(store->type_configs);
    
    pthread_rwlock_unlock(&store->lock);
    pthread_rwlock_destroy(&store->lock);
    
    log_debug("State store destroyed");
    free(store);
}

// Configuration functions
bool state_store_configure_type(StateStore* store, const DataTypeConfig* config) {
    if (!store || !config || !config->type_name[0]) {
        log_error("Invalid parameters for state_store_configure_type");
        return false;
    }
    
    pthread_rwlock_wrlock(&store->lock);
    
    // Look for existing config
    for (size_t i = 0; i < store->num_type_configs; i++) {
        if (strcmp(store->type_configs[i].type_name, config->type_name) == 0) {
            // Update existing config
            store->type_configs[i].config = *config;
            pthread_rwlock_unlock(&store->lock);
            log_info("Updated config for type '%s'", config->type_name);
            return true;
        }
    }
    
    // Expand configs array if needed
    if (store->num_type_configs >= store->type_config_capacity) {
        size_t new_capacity = store->type_config_capacity * 2;
        TypeConfigEntry* new_configs = realloc(store->type_configs, 
                                              new_capacity * sizeof(TypeConfigEntry));
        if (!new_configs) {
            log_error("Failed to expand type configs array");
            pthread_rwlock_unlock(&store->lock);
            return false;
        }
        store->type_configs = new_configs;
        store->type_config_capacity = new_capacity;
    }
    
    // Add new config
    TypeConfigEntry* new_config = &store->type_configs[store->num_type_configs];
    strncpy(new_config->type_name, config->type_name, sizeof(new_config->type_name) - 1);
    new_config->type_name[sizeof(new_config->type_name) - 1] = '\0';
    new_config->config = *config;
    
    store->num_type_configs++;
    
    pthread_rwlock_unlock(&store->lock);
    log_info("Added config for new type '%s'", config->type_name);
    return true;
}

DataTypeConfig state_store_get_type_config(StateStore* store, const char* type_name) {
    DataTypeConfig default_config = DEFAULT_TYPE_CONFIG;
    
    if (!store || !type_name) {
        return default_config;
    }
    
    pthread_rwlock_rdlock(&store->lock);
    
    for (size_t i = 0; i < store->num_type_configs; i++) {
        if (strcmp(store->type_configs[i].type_name, type_name) == 0) {
            DataTypeConfig config = store->type_configs[i].config;
            pthread_rwlock_unlock(&store->lock);
            return config;
        }
    }
    
    pthread_rwlock_unlock(&store->lock);
    
    // Return default config with type name filled in
    strncpy(default_config.type_name, type_name, sizeof(default_config.type_name) - 1);
    default_config.type_name[sizeof(default_config.type_name) - 1] = '\0';
    return default_config;
}

// Core data operations
bool state_store_set(StateStore* store, const char* type_name, const char* id,
                     const void* data, size_t data_size) {
    if (!store || !type_name || !id || !data || data_size == 0) {
        log_error("Invalid parameters for state_store_set");
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
    return true;
}

void* state_store_get(StateStore* store, const char* type_name, const char* id,
                      size_t* size_out, time_t* timestamp_out) {
    if (!store || !type_name || !id) {
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
            log_debug("Retrieved item: %s (%zu bytes)", compound_key, item->data_size);
            return data_copy;
        }
    }
    
    pthread_rwlock_unlock(&store->lock);
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
            // Check if expired
            time_t now = time(NULL);
            if (store->items[i].expires_at > 0 && now > store->items[i].expires_at) {
                pthread_rwlock_unlock(&store->lock);
                return false;
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
            
            // Move last item to this position to fill gap
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