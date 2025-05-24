# API Module Architecture Design

## Overview
Modular, loosely-coupled API system with clean separation of concerns.

## Architecture Layers

```
┌─────────────────────────────────────────┐
│              Application Layer          │
│         (app.c, UI components)          │
└─────────────────┬───────────────────────┘
                  │ Callbacks
┌─────────────────▼───────────────────────┐
│              API Manager                │
│        (api_manager.c/h)                │
│  - Request orchestration                │
│  - State management                     │
│  - UI update callbacks                  │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│             API Client                  │
│         (api_client.c/h)                │
│  - HTTP requests (CURL)                 │
│  - Response handling                    │
│  - Error management                     │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│            JSON Parser                  │
│         (json_parser.c/h)               │
│  - jsmn integration                     │
│  - Type-safe parsing                    │
│  - Error validation                     │
└─────────────────────────────────────────┘
```

## Key Principles

### 1. Separation of Concerns
- **API Manager**: Orchestrates requests, manages state
- **API Client**: Handles HTTP communication
- **JSON Parser**: Parses and validates responses
- **Application**: Renders data, handles UI events

### 2. Loose Coupling
- **Callback interfaces**: API → Application communication
- **Abstract data types**: No direct struct access
- **Configuration-driven**: URLs, timeouts, etc. externalized

### 3. Thread Safety
- **Async operations**: Non-blocking API calls
- **Mutex protection**: Shared state properly locked
- **Callback queuing**: UI updates on main thread

### 4. Error Handling
- **Typed errors**: Network, parsing, validation errors
- **Graceful degradation**: App continues if API fails
- **Retry logic**: Configurable retry with backoff

## API Manager Interface

```c
// Opaque handle
typedef struct ApiManager ApiManager;

// User data structure (populated by API)
typedef struct {
    char name[128];
    char email[128];
    char location[128];
    char phone[64];
    char picture_url[256];
    char nationality[32];
    int age;
} UserData;

// API states
typedef enum {
    API_STATE_IDLE,
    API_STATE_LOADING,
    API_STATE_SUCCESS,
    API_STATE_ERROR
} ApiState;

// Callback types
typedef void (*api_data_callback)(const UserData* data, void* context);
typedef void (*api_error_callback)(const char* error, void* context);
typedef void (*api_state_callback)(ApiState state, void* context);

// API Manager functions
ApiManager* api_manager_create(void);
void api_manager_destroy(ApiManager* manager);

void api_manager_set_data_callback(ApiManager* manager, api_data_callback callback, void* context);
void api_manager_set_error_callback(ApiManager* manager, api_error_callback callback, void* context);
void api_manager_set_state_callback(ApiManager* manager, api_state_callback callback, void* context);

bool api_manager_fetch_user(ApiManager* manager);
ApiState api_manager_get_state(ApiManager* manager);
const UserData* api_manager_get_data(ApiManager* manager);
```

## Benefits

1. **Testability**: Each layer can be unit tested independently
2. **Maintainability**: Clear responsibilities, easy to modify
3. **Extensibility**: Easy to add new API endpoints
4. **Reusability**: API module can be used in other projects
5. **Performance**: Async operations don't block UI
6. **Reliability**: Proper error handling and retry logic