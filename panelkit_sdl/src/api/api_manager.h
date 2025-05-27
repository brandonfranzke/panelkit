#ifndef API_MANAGER_H
#define API_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

// Forward declaration
typedef struct ApiManager ApiManager;

// User data structure (what the UI cares about)
typedef struct {
    char name[128];
    char email[128];
    char location[128];
    char phone[64];
    char picture_url[256];
    char nationality[32];
    int age;
    bool is_valid;              // Whether data is valid
} UserData;

// API states
typedef enum {
    API_STATE_IDLE,             // Not doing anything
    API_STATE_LOADING,          // Request in progress
    API_STATE_SUCCESS,          // Data loaded successfully
    API_STATE_ERROR             // Error occurred
} ApiState;

// Error types
typedef enum {
    API_ERROR_NONE = 0,
    API_ERROR_NETWORK,          // Network/HTTP error
    API_ERROR_PARSE,            // JSON parsing error
    API_ERROR_VALIDATION,       // Data validation error
    API_ERROR_TIMEOUT,          // Request timeout
    API_ERROR_MEMORY            // Memory allocation error
} ApiError;

// Configuration
typedef struct {
    const char* base_url;       // Base URL for API (borrowed reference - must remain valid)
    int timeout_seconds;        // Request timeout
    int retry_count;            // Number of retries
    int retry_delay_ms;         // Delay between retries
    bool auto_refresh;          // Auto-refresh data
    int refresh_interval_ms;    // Auto-refresh interval
} ApiManagerConfig;

// Callback function types
typedef void (*api_data_callback)(const UserData* data, void* context);
typedef void (*api_error_callback)(ApiError error, const char* message, void* context);
typedef void (*api_state_callback)(ApiState state, void* context);

// Create/destroy manager
ApiManager* api_manager_create(const ApiManagerConfig* config);
void api_manager_destroy(ApiManager* manager);

// Callback registration
void api_manager_set_data_callback(ApiManager* manager, api_data_callback callback, void* context);
void api_manager_set_error_callback(ApiManager* manager, api_error_callback callback, void* context);
void api_manager_set_state_callback(ApiManager* manager, api_state_callback callback, void* context);

// API operations
bool api_manager_fetch_user(ApiManager* manager);
bool api_manager_fetch_user_async(ApiManager* manager);

// State queries
ApiState api_manager_get_state(ApiManager* manager);

/**
 * Get current user data
 * @param manager API manager instance
 * @return Pointer to internal user data (borrowed reference, do not free)
 * @note The returned pointer is only valid until the next API call
 * @note Thread safety: Data may change if auto-refresh is enabled
 */
const UserData* api_manager_get_user_data(ApiManager* manager);

ApiError api_manager_get_last_error(ApiManager* manager);

/**
 * Get error message string
 * @param manager API manager instance  
 * @return Pointer to internal error buffer (borrowed reference, do not free)
 * @note The returned pointer is only valid until the next API call
 * @note Not thread-safe for concurrent access
 */
const char* api_manager_get_error_message(ApiManager* manager);

// Control functions
void api_manager_cancel_requests(ApiManager* manager);
void api_manager_set_auto_refresh(ApiManager* manager, bool enabled);
void api_manager_update(ApiManager* manager, uint32_t current_time_ms);

// Utility functions
const char* api_state_string(ApiState state);
const char* api_error_string(ApiError error);
ApiManagerConfig api_manager_default_config(void);

#endif // API_MANAGER_H