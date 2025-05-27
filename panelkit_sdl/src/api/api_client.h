/**
 * @file api_client.h
 * @brief Low-level HTTP client for API communication
 * 
 * Provides HTTP request/response handling with connection pooling,
 * timeout management, and retry capabilities.
 */

#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <stddef.h>
#include <stdbool.h>

// Forward declarations
typedef struct ApiClient ApiClient;
typedef struct ApiResponse ApiResponse;

// HTTP methods
typedef enum {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE
} HttpMethod;

// Error codes
typedef enum {
    API_CLIENT_SUCCESS = 0,
    API_CLIENT_ERROR_INIT = -1,
    API_CLIENT_ERROR_NETWORK = -2,
    API_CLIENT_ERROR_TIMEOUT = -3,
    API_CLIENT_ERROR_MEMORY = -4,
    API_CLIENT_ERROR_INVALID_URL = -5
} ApiClientError;

// Configuration
typedef struct {
    int timeout_seconds;        // Request timeout
    int connect_timeout_seconds;// Connection timeout
    bool follow_redirects;      // Follow HTTP redirects
    int max_redirects;          // Maximum number of redirects
    const char* user_agent;     // User-Agent header
    
    // Retry configuration
    int max_retries;            // Maximum retry attempts (0 = no retry)
    int initial_backoff_ms;     // Initial backoff in milliseconds
    int max_backoff_ms;         // Maximum backoff in milliseconds
    float backoff_multiplier;   // Backoff multiplier (e.g., 2.0 for exponential)
} ApiClientConfig;

// Response structure
struct ApiResponse {
    char* data;                 // Response body
    size_t size;                // Response size
    long http_code;             // HTTP status code
    char* error_message;        // Error message if any
};

// Request completion callback
typedef void (*api_client_callback)(ApiResponse* response, void* user_data);

// Create/destroy client
ApiClient* api_client_create(const ApiClientConfig* config);
void api_client_destroy(ApiClient* client);

// Synchronous requests
ApiClientError api_client_request(ApiClient* client, 
                                  HttpMethod method,
                                  const char* url,
                                  const char* body,
                                  ApiResponse* response);

// Asynchronous requests (threaded)
ApiClientError api_client_request_async(ApiClient* client,
                                         HttpMethod method, 
                                         const char* url,
                                         const char* body,
                                         api_client_callback callback,
                                         void* user_data);

// Response management
void api_response_init(ApiResponse* response);
void api_response_cleanup(ApiResponse* response);

// Utility functions
const char* api_client_error_string(ApiClientError error);
const char* http_method_string(HttpMethod method);

// Default configuration
ApiClientConfig api_client_default_config(void);

#endif // API_CLIENT_H