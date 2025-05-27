/**
 * @file api_manager.h
 * @brief High-level API service management
 * 
 * Manages multiple API endpoints, handles authentication, caching,
 * and provides a unified interface for API operations.
 */

#ifndef API_MANAGER_H
#define API_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

/** Opaque API manager handle */
typedef struct ApiManager ApiManager;

/**
 * User data structure.
 * Contains profile information fetched from API.
 */
typedef struct {
    char name[128];         /**< Full name */
    char email[128];        /**< Email address */
    char location[128];     /**< Location string */
    char phone[64];         /**< Phone number */
    char picture_url[256];  /**< Profile picture URL */
    char nationality[32];   /**< Nationality code */
    int age;                /**< Age in years */
    bool is_valid;          /**< Whether data is valid */
} UserData;

/**
 * API operation states.
 * Tracks current state of API requests.
 */
typedef enum {
    API_STATE_IDLE,         /**< Not doing anything */
    API_STATE_LOADING,      /**< Request in progress */
    API_STATE_SUCCESS,      /**< Data loaded successfully */
    API_STATE_ERROR         /**< Error occurred */
} ApiState;

/**
 * API error types.
 * Categorizes different failure modes.
 */
typedef enum {
    API_ERROR_NONE = 0,     /**< No error */
    API_ERROR_NETWORK,      /**< Network/HTTP error */
    API_ERROR_PARSE,        /**< JSON parsing error */
    API_ERROR_VALIDATION,   /**< Data validation error */
    API_ERROR_TIMEOUT,      /**< Request timeout */
    API_ERROR_MEMORY        /**< Memory allocation error */
} ApiError;

/**
 * API manager configuration.
 * Controls behavior of API requests and caching.
 */
typedef struct {
    const char* base_url;       /**< Base URL for API (borrowed - must remain valid) */
    int timeout_seconds;        /**< Request timeout in seconds */
    int retry_count;            /**< Number of retry attempts */
    int retry_delay_ms;         /**< Delay between retries in milliseconds */
    bool auto_refresh;          /**< Enable automatic data refresh */
    int refresh_interval_ms;    /**< Auto-refresh interval in milliseconds */
} ApiManagerConfig;

/**
 * Callback for successful data fetch.
 * 
 * @param data Fetched user data (required, borrowed reference)
 * @param context User-provided context (optional)
 */
typedef void (*api_data_callback)(const UserData* data, void* context);

/**
 * Callback for API errors.
 * 
 * @param error Error type
 * @param message Error description (optional, borrowed reference)
 * @param context User-provided context (optional)
 */
typedef void (*api_error_callback)(ApiError error, const char* message, void* context);

/**
 * Callback for state changes.
 * 
 * @param state New API state
 * @param context User-provided context (optional)
 */
typedef void (*api_state_callback)(ApiState state, void* context);

// Lifecycle

/**
 * Create API manager with configuration.
 * 
 * @param config API configuration (required)
 * @return New manager or NULL on error (caller owns)
 * @note config->base_url must remain valid for manager lifetime
 */
ApiManager* api_manager_create(const ApiManagerConfig* config);

/**
 * Destroy API manager.
 * 
 * @param manager Manager to destroy (can be NULL)
 * @note Cancels pending requests before destruction
 */
void api_manager_destroy(ApiManager* manager);

// Callbacks

/**
 * Set callback for successful data fetch.
 * 
 * @param manager API manager (required)
 * @param callback Callback function (can be NULL to clear)
 * @param context User context passed to callback (optional)
 */
void api_manager_set_data_callback(ApiManager* manager, api_data_callback callback, void* context);

/**
 * Set callback for API errors.
 * 
 * @param manager API manager (required)
 * @param callback Callback function (can be NULL to clear)
 * @param context User context passed to callback (optional)
 */
void api_manager_set_error_callback(ApiManager* manager, api_error_callback callback, void* context);

/**
 * Set callback for state changes.
 * 
 * @param manager API manager (required)
 * @param callback Callback function (can be NULL to clear)
 * @param context User context passed to callback (optional)
 */
void api_manager_set_state_callback(ApiManager* manager, api_state_callback callback, void* context);

// Operations

/**
 * Fetch user data synchronously.
 * 
 * @param manager API manager (required)
 * @return true on success, false on error
 * @note Blocks until request completes
 */
bool api_manager_fetch_user(ApiManager* manager);

/**
 * Fetch user data asynchronously.
 * 
 * @param manager API manager (required)
 * @return true if request started, false on error
 * @note Non-blocking - callbacks fired on completion
 */
bool api_manager_fetch_user_async(ApiManager* manager);

// State queries

/**
 * Get current API state.
 * 
 * @param manager API manager (required)
 * @return Current state
 */
ApiState api_manager_get_state(ApiManager* manager);

/**
 * Get current user data.
 * 
 * @param manager API manager (required)
 * @return Internal user data (borrowed - do not free)
 * @note Valid only until next API call
 * @note Thread safety: Data may change if auto-refresh enabled
 */
const UserData* api_manager_get_user_data(ApiManager* manager);

/**
 * Get last error code.
 * 
 * @param manager API manager (required)
 * @return Last error or API_ERROR_NONE
 */
ApiError api_manager_get_last_error(ApiManager* manager);

/**
 * Get error message string.
 * 
 * @param manager API manager (required)
 * @return Internal error buffer (borrowed - do not free)
 * @note Valid only until next API call
 * @note Not thread-safe for concurrent access
 */
const char* api_manager_get_error_message(ApiManager* manager);

// Control

/**
 * Cancel all pending API requests.
 * 
 * @param manager API manager (required)
 */
void api_manager_cancel_requests(ApiManager* manager);

/**
 * Enable or disable auto-refresh.
 * 
 * @param manager API manager (required)
 * @param enabled true to enable, false to disable
 */
void api_manager_set_auto_refresh(ApiManager* manager, bool enabled);

/**
 * Update manager state and handle auto-refresh.
 * 
 * @param manager API manager (required)
 * @param current_time_ms Current time in milliseconds
 * @note Call periodically for auto-refresh to work
 */
void api_manager_update(ApiManager* manager, uint32_t current_time_ms);

// Utilities

/**
 * Get string representation of API state.
 * 
 * @param state API state enum value
 * @return Static string name (never NULL)
 */
const char* api_state_string(ApiState state);

/**
 * Get string representation of API error.
 * 
 * @param error API error enum value
 * @return Static string name (never NULL)
 */
const char* api_error_string(ApiError error);

/**
 * Get default API manager configuration.
 * 
 * @return Configuration with sensible defaults
 * @note Caller must set base_url before use
 */
ApiManagerConfig api_manager_default_config(void);

#endif // API_MANAGER_H