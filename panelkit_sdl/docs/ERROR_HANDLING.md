# Error Handling System

## Overview

PanelKit implements a comprehensive error handling system with thread-local error context, structured error codes, and automatic logging integration. The system is designed for embedded devices where robust error recovery is critical.

## Error Code System

### Error Types (PkError)
```c
typedef enum {
    /* Success */
    PK_OK = 0,
    
    /* Parameter errors */
    PK_ERROR_NULL_PARAM = -1,
    PK_ERROR_INVALID_PARAM = -2,
    
    /* Resource errors */
    PK_ERROR_OUT_OF_MEMORY = -10,
    PK_ERROR_RESOURCE_LIMIT = -11,
    PK_ERROR_NOT_FOUND = -12,
    PK_ERROR_ALREADY_EXISTS = -13,
    
    /* State errors */
    PK_ERROR_INVALID_STATE = -20,
    PK_ERROR_NOT_INITIALIZED = -21,
    PK_ERROR_ALREADY_INITIALIZED = -22,
    
    /* External errors */
    PK_ERROR_SDL = -30,
    PK_ERROR_SYSTEM = -31,
    PK_ERROR_NETWORK = -32,
    PK_ERROR_TIMEOUT = -33,
    
    /* Data errors */
    PK_ERROR_PARSE = -40,
    PK_ERROR_INVALID_DATA = -41,
    PK_ERROR_INVALID_CONFIG = -42
} PkError;
```

## Error Context

Thread-local storage maintains error context with formatted messages:

```c
// Set error with context
pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
    "widget_create: Failed to allocate widget '%s' (%zu bytes)",
    id, sizeof(Widget));

// Retrieve last error
PkError error = pk_get_last_error();
const char* context = pk_get_last_error_context();
```

## Error Propagation Patterns

### Direct Return
```c
PkError widget_set_bounds(Widget* widget, int x, int y, int w, int h) {
    if (!widget) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
            "widget_set_bounds: widget is NULL");
        return PK_ERROR_NULL_PARAM;
    }
    
    widget->bounds = (SDL_Rect){x, y, w, h};
    return PK_OK;
}
```

### NULL Return with Error Context
```c
Widget* widget_create(const char* id, WidgetType type) {
    if (!id) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
            "widget_create: id is NULL");
        return NULL;
    }
    
    Widget* widget = calloc(1, sizeof(Widget));
    if (!widget) {
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
            "widget_create: Failed to allocate widget '%s'", id);
        return NULL;
    }
    
    return widget;
}
```

### Error Checking Macros
```c
// Check condition and return error
PK_CHECK_ERROR(widget != NULL, PK_ERROR_NULL_PARAM);

// Check with custom context
PK_CHECK_ERROR_WITH_CONTEXT(size <= MAX_SIZE, 
    PK_ERROR_INVALID_PARAM,
    "Size %zu exceeds maximum %zu", size, MAX_SIZE);

// Check pointer and return NULL
PK_CHECK_NULL(config, "Configuration is required");

// Propagate error from function call
PK_RETURN_IF_ERROR(validate_config(config));
```

## Recovery Strategies

### Network Failures
```c
// Exponential backoff retry
for (int attempt = 1; attempt <= max_retries; attempt++) {
    error = api_client_request(client, method, url, body, response);
    
    if (error == PK_OK) break;
    
    if (error == PK_ERROR_NETWORK || error == PK_ERROR_TIMEOUT) {
        int delay = retry_delay * (1 << (attempt - 1));
        log_warn("Request failed, retrying in %dms (attempt %d/%d)",
                 delay, attempt, max_retries);
        usleep(delay * 1000);
    } else {
        break; // Non-retryable error
    }
}
```

### Resource Allocation
```c
// Cleanup on partial initialization
Widget* widget = calloc(1, sizeof(Widget));
if (!widget) {
    pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
        "Failed to allocate widget");
    return NULL;
}

if (widget_init(widget, id, type) != PK_OK) {
    free(widget);  // Cleanup on failure
    return NULL;
}
```

### Configuration Errors
```c
// Fallback to defaults
Config* config = config_load(path);
if (!config) {
    log_warn("Failed to load config, using defaults");
    config = config_create_defaults();
}
```

## Error Logging

### Automatic Integration
```c
// Error logger configuration
ErrorLogConfig log_config = {
    .enabled = true,
    .log_file_path = "/var/log/panelkit/errors.log",
    .max_file_size = 10 * 1024 * 1024,  // 10MB
    .max_files = 5,
    .min_severity = ERROR_SEVERITY_WARNING
};

error_logger_init(&log_config);
```

### Log Entry Format
```
[2024-01-15 14:32:45.123] [ERROR] [widget.c:145] [widget_create] 
    PK_ERROR_OUT_OF_MEMORY: Failed to allocate widget 'main_button' (1024 bytes)
```

## Component-Specific Patterns

### Widget System
- Return NULL on creation failure
- Set error context with widget ID
- Clean up partial initialization
- Validate bounds and parameters

### Event System
- Check subscription limits
- Validate event names
- Handle duplicate subscriptions
- Clean up on failure

### State Store
- Enforce size limits (1MB items)
- Validate keys and namespaces
- Handle concurrent access
- Report storage failures

### Display Backend
- Fallback backend selection
- Resolution negotiation
- Capability detection
- Graceful degradation

## Best Practices

### DO:
- Set error context immediately at point of detection
- Include relevant parameters in error messages
- Clean up resources on error paths
- Check return values from all functions
- Use appropriate error codes

### DON'T:
- Ignore error returns
- Use generic error codes
- Leave error context from previous operations
- Leak resources on error paths
- Print errors to stdout (use logger)

## Error UI (Future)

### Planned Components
- Error notification widget
- Toast-style notifications
- Error queue management
- Debug overlay integration

### Design Principles
- Non-blocking display
- Auto-dismiss for warnings
- Persistent display for errors
- Touch to dismiss
- Severity-based styling