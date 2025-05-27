# API Integration

## Overview

The API subsystem provides HTTP communication with external services, JSON parsing, and asynchronous data delivery. It's designed with clean separation between network operations, data parsing, and UI updates.

## Architecture

```
┌─────────────────────────────────────────┐
│          Application Layer              │
│     (Callbacks, UI updates)             │
└─────────────────┬───────────────────────┘
                  │ Callbacks
┌─────────────────▼───────────────────────┐
│            API Manager                  │
│  - Request orchestration                │
│  - State management                     │
│  - Callback coordination                │
│  - Retry logic                          │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│            API Client                   │
│  - HTTP operations (CURL)               │
│  - Thread management                    │
│  - Response handling                    │
│  - Connection pooling                   │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│           API Parsers                   │
│  - JSON parsing (jsmn)                  │
│  - Type-safe extraction                 │
│  - Validation                           │
└─────────────────────────────────────────┘
```

## Components

### API Manager (`api_manager.h`)

High-level orchestration of API operations.

**Responsibilities**:
- Request lifecycle management
- State machine (IDLE → LOADING → SUCCESS/ERROR)
- Callback coordination
- User-facing API

**Key Functions**:
```c
// Lifecycle
ApiManager* api_manager_create(const ApiManagerConfig* config);
void api_manager_destroy(ApiManager* manager);

// Callbacks
void api_manager_set_data_callback(ApiManager* manager, 
                                  api_data_callback callback, 
                                  void* context);

// Operations
bool api_manager_fetch_user(ApiManager* manager);        // Sync
bool api_manager_fetch_user_async(ApiManager* manager);  // Async

// State
ApiState api_manager_get_state(ApiManager* manager);
```

### API Client (`api_client.h`)

Low-level HTTP operations using CURL.

**Features**:
- Synchronous and asynchronous requests
- Connection reuse
- Timeout management
- Error handling with retry
- Thread-safe operations

**Key Functions**:
```c
// HTTP request
ApiClientError api_client_request(ApiClient* client, 
                                 HttpMethod method,
                                 const char* url,
                                 const char* body,
                                 ApiResponse* response);

// Async request
ApiClientError api_client_request_async(ApiClient* client,
                                       HttpMethod method, 
                                       const char* url,
                                       const char* body,
                                       api_client_callback callback,
                                       void* user_data);
```

### API Parsers (`api_parsers.h`)

JSON parsing and data extraction.

**Features**:
- Type-safe parsing
- Null-safe string copying
- Nested object navigation
- Array handling

**Example Parser**:
```c
bool parse_user_api_response(const char* json_string, 
                            UserData* user_data) {
    jsmn_parser parser;
    jsmntok_t tokens[256];
    
    jsmn_init(&parser);
    int token_count = jsmn_parse(&parser, json_string, 
                                strlen(json_string), 
                                tokens, 256);
    
    // Extract data from tokens
    // ... parsing logic ...
    
    return true;
}
```

## Data Flow

### Synchronous Flow
1. UI calls `api_manager_fetch_user()`
2. Manager calls `api_client_request()`
3. Client performs HTTP GET
4. Parser extracts UserData
5. Callbacks invoked with data
6. UI updates

### Asynchronous Flow
1. UI calls `api_manager_fetch_user_async()`
2. Manager creates thread
3. Thread calls `api_client_request()`
4. Parser processes in thread
5. Callbacks invoked on completion
6. Thread cleaned up

## Configuration

```yaml
api:
  default_base_url: "https://randomuser.me/api/"
  default_timeout_ms: 5000
  default_retry_count: 3
  default_retry_delay_ms: 1000
  max_retries: 5
  
  services:
    - name: "randomuser"
      base_url: "https://randomuser.me/api/"
      endpoints:
        - name: "user"
          path: "/"
          method: "GET"
```

## Error Handling

### Error Types
```c
typedef enum {
    API_CLIENT_SUCCESS = 0,
    API_CLIENT_ERROR_NETWORK,
    API_CLIENT_ERROR_TIMEOUT,
    API_CLIENT_ERROR_HTTP,
    API_CLIENT_ERROR_MEMORY,
    API_CLIENT_ERROR_INVALID_PARAM
} ApiClientError;
```

### Retry Logic
- Exponential backoff: delay * 2^attempt
- Skip retry on 4xx errors (except 429)
- Maximum retry limit from config
- Timeout increases with retries

## Callbacks

### Data Callback
```c
void on_api_data_received(const UserData* data, void* context) {
    // Update UI with new data
    update_display(data);
    
    // Store in state store
    state_store_set(store, "api_data", "user", 
                    data, sizeof(*data));
}
```

### Error Callback
```c
void on_api_error(ApiError error, const char* message, 
                  void* context) {
    log_error("API error: %s", message);
    show_error_dialog(message);
}
```

### State Callback
```c
void on_api_state_changed(ApiState state, void* context) {
    switch (state) {
        case API_STATE_LOADING:
            show_loading_spinner();
            break;
        case API_STATE_SUCCESS:
            hide_loading_spinner();
            break;
    }
}
```

## Integration with Event System

The API manager can publish events for state changes:

```c
// When data is received
event_publish_api_user_data_updated(event_system, &user_data);

// When state changes
ApiStateChangeData state_data = {
    .state_name = "loading",
    .value = "true"
};
event_publish_api_state_changed(event_system, &state_data);
```

## Thread Safety

- API Manager: Thread-safe state access
- API Client: Thread-safe request handling
- Callbacks: Invoked on request thread
- Event publishing: Thread-safe

## Best Practices

1. **Always set callbacks** before making requests
2. **Check state** before making new requests
3. **Handle all error cases** in error callback
4. **Free UserData** when no longer needed
5. **Use async** for UI responsiveness
6. **Configure timeouts** appropriately
7. **Test with network failures** during development