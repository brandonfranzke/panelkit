#include "api_client.h"
#include "../core/logger.h"
#include "../core/error.h"
#include "../core/sdl_includes.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

struct ApiClient {
    CURL* curl;
    ApiClientConfig config;
    pthread_mutex_t mutex;
};

// Async request data
typedef struct {
    ApiClient* client;
    HttpMethod method;
    char* url;
    char* body;
    api_client_callback callback;
    void* user_data;
} AsyncRequest;

// CURL write callback
static size_t write_callback(void* contents, size_t size, size_t nmemb, ApiResponse* response) {
    size_t realsize = size * nmemb;
    
    char* new_data = realloc(response->data, response->size + realsize + 1);
    if (!new_data) {
        log_error("Failed to allocate memory for API response");
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
            "write_callback: Failed to allocate %zu bytes for response",
            response->size + realsize + 1);
        return 0;
    }
    
    response->data = new_data;
    memcpy(&response->data[response->size], contents, realsize);
    response->size += realsize;
    response->data[response->size] = '\0';
    
    return realsize;
}

ApiClient* api_client_create(const ApiClientConfig* config) {
    ApiClient* client = calloc(1, sizeof(ApiClient));
    if (!client) {
        log_error("Failed to allocate API client");
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
            "api_client_create: Failed to allocate %zu bytes", sizeof(ApiClient));
        return NULL;
    }
    
    // Initialize CURL
    client->curl = curl_easy_init();
    if (!client->curl) {
        log_error("Failed to initialize CURL");
        pk_set_last_error_with_context(PK_ERROR_SYSTEM,
            "api_client_create: curl_easy_init failed");
        free(client);
        return NULL;
    }
    
    // Copy configuration
    if (config) {
        client->config = *config;
    } else {
        client->config = api_client_default_config();
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&client->mutex, NULL) != 0) {
        log_error("Failed to initialize mutex");
        pk_set_last_error_with_context(PK_ERROR_SYSTEM,
            "api_client_create: pthread_mutex_init failed");
        curl_easy_cleanup(client->curl);
        free(client);
        return NULL;
    }
    
    // Set basic CURL options
    curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(client->curl, CURLOPT_TIMEOUT, (long)client->config.timeout_seconds);
    curl_easy_setopt(client->curl, CURLOPT_CONNECTTIMEOUT, (long)client->config.connect_timeout_seconds);
    curl_easy_setopt(client->curl, CURLOPT_FOLLOWLOCATION, client->config.follow_redirects ? 1L : 0L);
    curl_easy_setopt(client->curl, CURLOPT_MAXREDIRS, (long)client->config.max_redirects);
    
    if (client->config.user_agent) {
        curl_easy_setopt(client->curl, CURLOPT_USERAGENT, client->config.user_agent);
    }
    
    log_info("API client initialized with %ds timeout", client->config.timeout_seconds);
    return client;
}

void api_client_destroy(ApiClient* client) {
    if (!client) {
        return;
    }
    
    pthread_mutex_lock(&client->mutex);
    
    if (client->curl) {
        curl_easy_cleanup(client->curl);
    }
    
    pthread_mutex_unlock(&client->mutex);
    pthread_mutex_destroy(&client->mutex);
    
    free(client);
    log_debug("API client destroyed");
}

ApiClientError api_client_request(ApiClient* client,
                                  HttpMethod method,
                                  const char* url,
                                  const char* body,
                                  ApiResponse* response) {
    if (!client || !url || !response) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
            "api_client_request: client=%p, url=%p, response=%p",
            (void*)client, (void*)url, (void*)response);
        return API_CLIENT_ERROR_INVALID_URL;
    }
    
    pthread_mutex_lock(&client->mutex);
    
    // Initialize response
    api_response_init(response);
    
    // Set URL
    curl_easy_setopt(client->curl, CURLOPT_URL, url);
    curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, response);
    
    // Set HTTP method
    switch (method) {
        case HTTP_METHOD_GET:
            curl_easy_setopt(client->curl, CURLOPT_HTTPGET, 1L);
            break;
        case HTTP_METHOD_POST:
            curl_easy_setopt(client->curl, CURLOPT_POST, 1L);
            if (body) {
                curl_easy_setopt(client->curl, CURLOPT_POSTFIELDS, body);
            }
            break;
        case HTTP_METHOD_PUT:
            curl_easy_setopt(client->curl, CURLOPT_CUSTOMREQUEST, "PUT");
            if (body) {
                curl_easy_setopt(client->curl, CURLOPT_POSTFIELDS, body);
            }
            break;
        case HTTP_METHOD_DELETE:
            curl_easy_setopt(client->curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            break;
    }
    
    // Retry logic with exponential backoff
    int max_attempts = client->config.max_retries + 1;  // Initial attempt + retries
    int backoff_ms = client->config.initial_backoff_ms;
    ApiClientError result = API_CLIENT_SUCCESS;
    CURLcode curl_result = CURLE_OK;
    
    for (int attempt = 1; attempt <= max_attempts; attempt++) {
        // Clear previous response data if retrying
        if (attempt > 1) {
            if (response->data) {
                free(response->data);
                response->data = NULL;
                response->size = 0;
            }
            if (response->error_message) {
                free(response->error_message);
                response->error_message = NULL;
            }
            
            // Wait with backoff
            log_info("Retrying API request (attempt %d/%d) after %dms backoff...", 
                     attempt, max_attempts, backoff_ms);
            SDL_Delay(backoff_ms);
            
            // Calculate next backoff (exponential with max limit)
            backoff_ms = (int)(backoff_ms * client->config.backoff_multiplier);
            if (backoff_ms > client->config.max_backoff_ms) {
                backoff_ms = client->config.max_backoff_ms;
            }
        }
        
        // Perform request
        curl_result = curl_easy_perform(client->curl);
        
        // Get HTTP response code
        curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE, &response->http_code);
        
        if (curl_result == CURLE_OK) {
            // Check if HTTP response indicates success or permanent failure
            if (response->http_code >= 200 && response->http_code < 300) {
                result = API_CLIENT_SUCCESS;
                break;  // Success - no need to retry
            } else if (response->http_code >= 400 && response->http_code < 500 &&
                      response->http_code != 429) {  // 429 is "Too Many Requests" - retryable
                // Client error (4xx except 429) - don't retry
                result = API_CLIENT_ERROR_NETWORK;
                log_error("API request failed with HTTP %ld - not retrying", response->http_code);
                break;
            }
            // Server errors (5xx) and 429 - continue retry loop
            result = API_CLIENT_ERROR_NETWORK;
        } else {
            // CURL error - check if retryable
            switch (curl_result) {
                case CURLE_OUT_OF_MEMORY:
                    result = API_CLIENT_ERROR_MEMORY;
                    pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                        "api_client_request: CURL out of memory on attempt %d/%d",
                        attempt, max_attempts);
                    break;  // Don't retry memory errors
                case CURLE_OPERATION_TIMEDOUT:
                    result = API_CLIENT_ERROR_TIMEOUT;
                    pk_set_last_error_with_context(PK_ERROR_TIMEOUT,
                        "api_client_request: Request to %s timed out (attempt %d/%d)",
                        url, attempt, max_attempts);
                    // Continue retry loop for timeouts
                    break;
                default:
                    result = API_CLIENT_ERROR_NETWORK;
                    pk_set_last_error_with_context(PK_ERROR_NETWORK,
                        "api_client_request: CURL error %d: %s (attempt %d/%d)",
                        curl_result, curl_easy_strerror(curl_result), attempt, max_attempts);
                    // Continue retry loop for network errors
                    break;
            }
            
            // Store error message
            const char* error_msg = curl_easy_strerror(curl_result);
            response->error_message = malloc(strlen(error_msg) + 32);
            if (response->error_message) {
                sprintf(response->error_message, "%s (attempt %d/%d)", 
                        error_msg, attempt, max_attempts);
            } else {
                pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                    "api_client_request: Failed to allocate error message");
            }
            
            // Don't retry memory errors
            if (result == API_CLIENT_ERROR_MEMORY) {
                break;
            }
        }
    }
    
    // Log final result
    if (result == API_CLIENT_SUCCESS) {
        log_debug("API request successful: %s -> %ld (%zu bytes)", 
                 url, response->http_code, response->size);
    } else {
        log_error("API request failed after %d attempts: %s", 
                 max_attempts, response->error_message ? response->error_message : "Unknown error");
    }
    
    pthread_mutex_unlock(&client->mutex);
    return result;
}

// Thread function for async requests
static void* async_request_thread(void* arg) {
    AsyncRequest* req = (AsyncRequest*)arg;
    
    ApiResponse response;
    ApiClientError error = api_client_request(req->client, req->method, req->url, req->body, &response);
    
    // Call callback with result
    if (req->callback) {
        req->callback(&response, req->user_data);
    }
    
    // Cleanup
    api_response_cleanup(&response);
    free(req->url);
    free(req->body);
    free(req);
    
    return NULL;
}

ApiClientError api_client_request_async(ApiClient* client,
                                         HttpMethod method,
                                         const char* url,
                                         const char* body,
                                         api_client_callback callback,
                                         void* user_data) {
    if (!client || !url) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
            "api_client_request_async: client=%p, url=%p",
            (void*)client, (void*)url);
        return API_CLIENT_ERROR_INVALID_URL;
    }
    
    // Create async request structure
    AsyncRequest* req = malloc(sizeof(AsyncRequest));
    if (!req) {
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
            "api_client_request_async: Failed to allocate %zu bytes for request",
            sizeof(AsyncRequest));
        return API_CLIENT_ERROR_MEMORY;
    }
    
    req->client = client;
    req->method = method;
    req->url = malloc(strlen(url) + 1);
    req->body = body ? malloc(strlen(body) + 1) : NULL;
    req->callback = callback;
    req->user_data = user_data;
    
    if (!req->url || (body && !req->body)) {
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
            "api_client_request_async: Failed to allocate URL/body strings");
        free(req->url);
        free(req->body);
        free(req);
        return API_CLIENT_ERROR_MEMORY;
    }
    
    strcpy(req->url, url);
    if (body) {
        strcpy(req->body, body);
    }
    
    // Create thread
    pthread_t thread;
    if (pthread_create(&thread, NULL, async_request_thread, req) != 0) {
        log_error("Failed to create async request thread");
        pk_set_last_error_with_context(PK_ERROR_SYSTEM,
            "api_client_request_async: pthread_create failed for URL %s", url);
        free(req->url);
        free(req->body);
        free(req);
        return API_CLIENT_ERROR_NETWORK;
    }
    
    // Detach thread so it cleans up automatically
    pthread_detach(thread);
    
    log_debug("Async request started: %s", url);
    return API_CLIENT_SUCCESS;
}

void api_response_init(ApiResponse* response) {
    if (response) {
        memset(response, 0, sizeof(ApiResponse));
        response->data = malloc(1);
        if (response->data) {
            response->data[0] = '\0';
        }
    }
}

void api_response_cleanup(ApiResponse* response) {
    if (response) {
        free(response->data);
        free(response->error_message);
        memset(response, 0, sizeof(ApiResponse));
    }
}

const char* api_client_error_string(ApiClientError error) {
    switch (error) {
        case API_CLIENT_SUCCESS:
            return "Success";
        case API_CLIENT_ERROR_INIT:
            return "Initialization error";
        case API_CLIENT_ERROR_NETWORK:
            return "Network error";
        case API_CLIENT_ERROR_TIMEOUT:
            return "Request timeout";
        case API_CLIENT_ERROR_MEMORY:
            return "Memory allocation error";
        case API_CLIENT_ERROR_INVALID_URL:
            return "Invalid URL";
        default:
            return "Unknown error";
    }
}

const char* http_method_string(HttpMethod method) {
    switch (method) {
        case HTTP_METHOD_GET:
            return "GET";
        case HTTP_METHOD_POST:
            return "POST";
        case HTTP_METHOD_PUT:
            return "PUT";
        case HTTP_METHOD_DELETE:
            return "DELETE";
        default:
            return "UNKNOWN";
    }
}

ApiClientConfig api_client_default_config(void) {
    ApiClientConfig config = {
        .timeout_seconds = 10,
        .connect_timeout_seconds = 5,
        .follow_redirects = true,
        .max_redirects = 3,
        .user_agent = "PanelKit/1.0",
        
        // Retry configuration defaults
        .max_retries = 3,
        .initial_backoff_ms = 100,
        .max_backoff_ms = 5000,
        .backoff_multiplier = 2.0f
    };
    return config;
}