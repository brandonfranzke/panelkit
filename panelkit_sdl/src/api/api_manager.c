#include "api_manager.h"
#include "api_client.h"
#include "../json/json_parser.h"
#include "../core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

struct ApiManager {
    ApiClient* client;
    ApiManagerConfig config;
    
    // State
    ApiState state;
    ApiError last_error;
    char error_message[256];
    UserData user_data;
    
    // Auto-refresh
    bool auto_refresh_enabled;
    uint32_t last_refresh_time;
    
    // Callbacks
    api_data_callback data_callback;
    void* data_context;
    api_error_callback error_callback;
    void* error_context;
    api_state_callback state_callback;
    void* state_context;
    
    // Thread safety
    pthread_mutex_t mutex;
};

// Internal functions
static void set_state(ApiManager* manager, ApiState new_state);
static void set_error(ApiManager* manager, ApiError error, const char* message);
static void handle_api_response(ApiResponse* response, void* user_data);
static bool parse_user_data(const char* json_data, UserData* user_data);

ApiManager* api_manager_create(const ApiManagerConfig* config) {
    ApiManager* manager = calloc(1, sizeof(ApiManager));
    if (!manager) {
        log_error("Failed to allocate API manager");
        return NULL;
    }
    
    // Copy configuration
    if (config) {
        manager->config = *config;
    } else {
        manager->config = api_manager_default_config();
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&manager->mutex, NULL) != 0) {
        log_error("Failed to initialize API manager mutex");
        free(manager);
        return NULL;
    }
    
    // Create API client
    ApiClientConfig client_config = {
        .timeout_seconds = manager->config.timeout_seconds,
        .connect_timeout_seconds = 5,
        .follow_redirects = true,
        .max_redirects = 3,
        .user_agent = "PanelKit/1.0"
    };
    
    manager->client = api_client_create(&client_config);
    if (!manager->client) {
        log_error("Failed to create API client");
        pthread_mutex_destroy(&manager->mutex);
        free(manager);
        return NULL;
    }
    
    // Initialize state
    manager->state = API_STATE_IDLE;
    manager->last_error = API_ERROR_NONE;
    manager->auto_refresh_enabled = manager->config.auto_refresh;
    
    log_info("API manager created with base URL: %s", manager->config.base_url);
    return manager;
}

void api_manager_destroy(ApiManager* manager) {
    if (!manager) {
        return;
    }
    
    pthread_mutex_lock(&manager->mutex);
    
    if (manager->client) {
        api_client_destroy(manager->client);
    }
    
    pthread_mutex_unlock(&manager->mutex);
    pthread_mutex_destroy(&manager->mutex);
    
    free(manager);
    log_debug("API manager destroyed");
}

void api_manager_set_data_callback(ApiManager* manager, api_data_callback callback, void* context) {
    if (!manager) return;
    
    pthread_mutex_lock(&manager->mutex);
    manager->data_callback = callback;
    manager->data_context = context;
    pthread_mutex_unlock(&manager->mutex);
}

void api_manager_set_error_callback(ApiManager* manager, api_error_callback callback, void* context) {
    if (!manager) return;
    
    pthread_mutex_lock(&manager->mutex);
    manager->error_callback = callback;
    manager->error_context = context;
    pthread_mutex_unlock(&manager->mutex);
}

void api_manager_set_state_callback(ApiManager* manager, api_state_callback callback, void* context) {
    if (!manager) return;
    
    pthread_mutex_lock(&manager->mutex);
    manager->state_callback = callback;
    manager->state_context = context;
    pthread_mutex_unlock(&manager->mutex);
}

bool api_manager_fetch_user_async(ApiManager* manager) {
    if (!manager) {
        return false;
    }
    
    pthread_mutex_lock(&manager->mutex);
    
    if (manager->state == API_STATE_LOADING) {
        log_debug("API request already in progress");
        pthread_mutex_unlock(&manager->mutex);
        return false;
    }
    
    set_state(manager, API_STATE_LOADING);
    
    // Build URL - for now using randomuser.me as in original
    const char* url = "https://randomuser.me/api/";
    
    pthread_mutex_unlock(&manager->mutex);
    
    ApiClientError result = api_client_request_async(manager->client, HTTP_METHOD_GET, url, NULL,
                                                      handle_api_response, manager);
    
    if (result != API_CLIENT_SUCCESS) {
        pthread_mutex_lock(&manager->mutex);
        set_error(manager, API_ERROR_NETWORK, api_client_error_string(result));
        set_state(manager, API_STATE_ERROR);
        pthread_mutex_unlock(&manager->mutex);
        return false;
    }
    
    log_debug("User data fetch initiated");
    return true;
}

bool api_manager_fetch_user(ApiManager* manager) {
    if (!manager) {
        return false;
    }
    
    pthread_mutex_lock(&manager->mutex);
    
    if (manager->state == API_STATE_LOADING) {
        log_debug("API request already in progress");
        pthread_mutex_unlock(&manager->mutex);
        return false;
    }
    
    set_state(manager, API_STATE_LOADING);
    
    const char* url = "https://randomuser.me/api/";
    
    pthread_mutex_unlock(&manager->mutex);
    
    ApiResponse response;
    ApiClientError result = api_client_request(manager->client, HTTP_METHOD_GET, url, NULL, &response);
    
    if (result != API_CLIENT_SUCCESS) {
        pthread_mutex_lock(&manager->mutex);
        set_error(manager, API_ERROR_NETWORK, api_client_error_string(result));
        set_state(manager, API_STATE_ERROR);
        pthread_mutex_unlock(&manager->mutex);
        api_response_cleanup(&response);
        return false;
    }
    
    // Process response
    handle_api_response(&response, manager);
    api_response_cleanup(&response);
    
    return true;
}

void api_manager_update(ApiManager* manager, uint32_t current_time_ms) {
    if (!manager) {
        return;
    }
    
    pthread_mutex_lock(&manager->mutex);
    
    // Check for auto-refresh
    if (manager->auto_refresh_enabled && 
        manager->state == API_STATE_SUCCESS &&
        (current_time_ms - manager->last_refresh_time) >= (uint32_t)manager->config.refresh_interval_ms) {
        
        pthread_mutex_unlock(&manager->mutex);
        api_manager_fetch_user_async(manager);
        return;
    }
    
    pthread_mutex_unlock(&manager->mutex);
}

ApiState api_manager_get_state(ApiManager* manager) {
    if (!manager) {
        return API_STATE_ERROR;
    }
    
    pthread_mutex_lock(&manager->mutex);
    ApiState state = manager->state;
    pthread_mutex_unlock(&manager->mutex);
    
    return state;
}

const UserData* api_manager_get_user_data(ApiManager* manager) {
    if (!manager) {
        return NULL;
    }
    
    pthread_mutex_lock(&manager->mutex);
    const UserData* data = &manager->user_data;
    pthread_mutex_unlock(&manager->mutex);
    
    return data;
}

ApiError api_manager_get_last_error(ApiManager* manager) {
    if (!manager) {
        return API_ERROR_MEMORY;
    }
    
    pthread_mutex_lock(&manager->mutex);
    ApiError error = manager->last_error;
    pthread_mutex_unlock(&manager->mutex);
    
    return error;
}

const char* api_manager_get_error_message(ApiManager* manager) {
    if (!manager) {
        return "Invalid manager";
    }
    
    pthread_mutex_lock(&manager->mutex);
    const char* message = manager->error_message;
    pthread_mutex_unlock(&manager->mutex);
    
    return message;
}

void api_manager_set_auto_refresh(ApiManager* manager, bool enabled) {
    if (!manager) {
        return;
    }
    
    pthread_mutex_lock(&manager->mutex);
    manager->auto_refresh_enabled = enabled;
    pthread_mutex_unlock(&manager->mutex);
    
    log_debug("Auto-refresh %s", enabled ? "enabled" : "disabled");
}

// Internal helper functions
static void set_state(ApiManager* manager, ApiState new_state) {
    ApiState old_state = manager->state;
    manager->state = new_state;
    
    log_debug("API state: %s -> %s", api_state_string(old_state), api_state_string(new_state));
    
    // Call state callback
    if (manager->state_callback) {
        manager->state_callback(new_state, manager->state_context);
    }
}

static void set_error(ApiManager* manager, ApiError error, const char* message) {
    manager->last_error = error;
    
    if (message) {
        strncpy(manager->error_message, message, sizeof(manager->error_message) - 1);
        manager->error_message[sizeof(manager->error_message) - 1] = '\0';
    } else {
        manager->error_message[0] = '\0';
    }
    
    log_error("API error: %s - %s", api_error_string(error), manager->error_message);
    
    // Call error callback
    if (manager->error_callback) {
        manager->error_callback(error, manager->error_message, manager->error_context);
    }
}

static void handle_api_response(ApiResponse* response, void* user_data) {
    ApiManager* manager = (ApiManager*)user_data;
    
    pthread_mutex_lock(&manager->mutex);
    
    if (response->http_code != 200) {
        set_error(manager, API_ERROR_NETWORK, "HTTP request failed");
        set_state(manager, API_STATE_ERROR);
        pthread_mutex_unlock(&manager->mutex);
        return;
    }
    
    if (!response->data || response->size == 0) {
        set_error(manager, API_ERROR_NETWORK, "Empty response");
        set_state(manager, API_STATE_ERROR);
        pthread_mutex_unlock(&manager->mutex);
        return;
    }
    
    // Parse user data
    UserData new_user_data = {0};
    if (!parse_user_data(response->data, &new_user_data)) {
        set_error(manager, API_ERROR_PARSE, "Failed to parse user data");
        set_state(manager, API_STATE_ERROR);
        pthread_mutex_unlock(&manager->mutex);
        return;
    }
    
    // Update stored data
    manager->user_data = new_user_data;
    manager->user_data.is_valid = true;
    manager->last_refresh_time = 0; // Will be updated by caller
    
    set_state(manager, API_STATE_SUCCESS);
    
    // Call data callback
    if (manager->data_callback) {
        manager->data_callback(&manager->user_data, manager->data_context);
    }
    
    pthread_mutex_unlock(&manager->mutex);
    
    log_info("User data updated: %s", manager->user_data.name);
}

static bool parse_user_data(const char* json_data, UserData* user_data) {
    JsonParser* parser = json_parser_create();
    if (!parser) {
        log_error("Failed to create JSON parser");
        return false;
    }
    
    JsonError error = json_parser_parse(parser, json_data, strlen(json_data));
    if (error != JSON_SUCCESS) {
        log_error("JSON parsing failed: %s", json_error_string(error));
        json_parser_destroy(parser);
        return false;
    }
    
    JsonValue* root = json_parser_get_root(parser);
    if (!root) {
        log_error("No JSON root object");
        json_parser_destroy(parser);
        return false;
    }
    
    // Get results array
    JsonValue* results = json_object_get(root, "results");
    if (!results || json_array_size(results) == 0) {
        log_error("No results in API response");
        json_parser_destroy(parser);
        return false;
    }
    
    // Get first user
    JsonValue* user = json_array_get(results, 0);
    if (!user) {
        log_error("No user data in results");
        json_parser_destroy(parser);
        return false;
    }
    
    // Parse name
    JsonValue* name_obj = json_object_get(user, "name");
    if (name_obj) {
        JsonValue* first = json_object_get(name_obj, "first");
        JsonValue* last = json_object_get(name_obj, "last");
        
        char first_name[64] = {0};
        char last_name[64] = {0};
        
        if (first) {
            JsonError err = json_value_get_string(first, first_name, sizeof(first_name));
            if (err != JSON_SUCCESS) {
                log_error("Failed to parse first name: %s", json_error_string(err));
            }
        }
        if (last) {
            JsonError err = json_value_get_string(last, last_name, sizeof(last_name));
            if (err != JSON_SUCCESS) {
                log_error("Failed to parse last name: %s", json_error_string(err));
            }
        }
        
        snprintf(user_data->name, sizeof(user_data->name), "%s %s", first_name, last_name);
    }
    
    // Parse email
    JsonValue* email = json_object_get(user, "email");
    if (email) {
        JsonError err = json_value_get_string(email, user_data->email, sizeof(user_data->email));
        if (err != JSON_SUCCESS) {
            log_error("Failed to parse email: %s", json_error_string(err));
        }
    }
    
    // Parse location
    JsonValue* location_obj = json_object_get(user, "location");
    if (location_obj) {
        JsonValue* city = json_object_get(location_obj, "city");
        JsonValue* country = json_object_get(location_obj, "country");
        
        char city_name[64] = {0};
        char country_name[64] = {0};
        
        if (city) {
            JsonError err = json_value_get_string(city, city_name, sizeof(city_name));
            if (err != JSON_SUCCESS) {
                log_error("Failed to parse city: %s", json_error_string(err));
            }
        }
        if (country) {
            JsonError err = json_value_get_string(country, country_name, sizeof(country_name));
            if (err != JSON_SUCCESS) {
                log_error("Failed to parse country: %s", json_error_string(err));
            }
        }
        
        snprintf(user_data->location, sizeof(user_data->location), "%s, %s", city_name, country_name);
    }
    
    // Parse phone
    JsonValue* phone = json_object_get(user, "phone");
    if (phone) {
        JsonError err = json_value_get_string(phone, user_data->phone, sizeof(user_data->phone));
        if (err != JSON_SUCCESS) {
            log_error("Failed to parse phone: %s", json_error_string(err));
        }
    }
    
    // Parse nationality
    JsonValue* nat = json_object_get(user, "nat");
    if (nat) {
        JsonError err = json_value_get_string(nat, user_data->nationality, sizeof(user_data->nationality));
        if (err != JSON_SUCCESS) {
            log_error("Failed to parse nationality: %s", json_error_string(err));
        }
    }
    
    // Parse age
    JsonValue* dob_obj = json_object_get(user, "dob");
    if (dob_obj) {
        JsonValue* age = json_object_get(dob_obj, "age");
        if (age) {
            JsonError err = json_value_get_int(age, &user_data->age);
            if (err != JSON_SUCCESS) {
                log_error("Failed to parse age: %s", json_error_string(err));
            }
        }
    }
    
    // Parse picture URL
    JsonValue* picture_obj = json_object_get(user, "picture");
    if (picture_obj) {
        JsonValue* large = json_object_get(picture_obj, "large");
        if (large) {
            JsonError err = json_value_get_string(large, user_data->picture_url, sizeof(user_data->picture_url));
            if (err != JSON_SUCCESS) {
                log_error("Failed to parse picture URL: %s", json_error_string(err));
            }
        }
    }
    
    json_parser_destroy(parser);
    
    log_debug("Parsed user data: %s, %d years old from %s", 
             user_data->name, user_data->age, user_data->location);
    
    return true;
}

// Utility functions
const char* api_state_string(ApiState state) {
    switch (state) {
        case API_STATE_IDLE:
            return "IDLE";
        case API_STATE_LOADING:
            return "LOADING";
        case API_STATE_SUCCESS:
            return "SUCCESS";
        case API_STATE_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

const char* api_error_string(ApiError error) {
    switch (error) {
        case API_ERROR_NONE:
            return "No error";
        case API_ERROR_NETWORK:
            return "Network error";
        case API_ERROR_PARSE:
            return "Parse error";
        case API_ERROR_VALIDATION:
            return "Validation error";
        case API_ERROR_TIMEOUT:
            return "Timeout";
        case API_ERROR_MEMORY:
            return "Memory error";
        default:
            return "Unknown error";
    }
}

ApiManagerConfig api_manager_default_config(void) {
    ApiManagerConfig config = {
        .base_url = "https://randomuser.me/api/",
        .timeout_seconds = 10,
        .retry_count = 3,
        .retry_delay_ms = 1000,
        .auto_refresh = false,
        .refresh_interval_ms = 30000  // 30 seconds
    };
    return config;
}