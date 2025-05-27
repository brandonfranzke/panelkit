#ifndef PK_ERROR_H
#define PK_ERROR_H

/**
 * PanelKit Error Handling System
 * 
 * This module provides consistent error handling across the codebase.
 * 
 * Design principles:
 * - Zero is success (PK_OK = 0)
 * - Negative values for errors (future expansion)
 * - Functions return error codes directly when possible
 * - NULL/false returns can use pk_get_last_error() for details
 * - Thread-safe error context via thread-local storage
 */

/* Error codes for PanelKit operations */
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
    PK_ERROR_SDL = -30,          /* SDL library error */
    PK_ERROR_SYSTEM = -31,       /* System call failed */
    PK_ERROR_NETWORK = -32,      /* Network/API error */
    PK_ERROR_TIMEOUT = -33,      /* Operation timed out */
    
    /* Data errors */
    PK_ERROR_PARSE = -40,        /* Parse error (JSON, YAML, etc) */
    PK_ERROR_INVALID_DATA = -41, /* Data validation failed */
    PK_ERROR_INVALID_CONFIG = -42,/* Configuration error */
    
    /* Widget-specific errors */
    PK_ERROR_WIDGET_NOT_FOUND = -50,
    PK_ERROR_WIDGET_TREE_FULL = -51,
    PK_ERROR_WIDGET_INVALID_TYPE = -52,
    
    /* Event system errors */
    PK_ERROR_EVENT_QUEUE_FULL = -60,
    PK_ERROR_EVENT_NOT_FOUND = -61,
    PK_ERROR_EVENT_HANDLER_FAILED = -62,
    
    /* Rendering errors */
    PK_ERROR_RENDER_FAILED = -70,
} PkError;

/**
 * Get human-readable error string.
 * 
 * @param error Error code from PkError enum
 * @return Static string describing the error (never NULL)
 * @note Do not free the returned string
 */
const char* pk_error_string(PkError error);

/**
 * Get last error for current thread.
 * 
 * @return Last error code set in this thread
 * @note Useful when function returns NULL/false without error code
 * @note Thread-local - each thread has independent error state
 */
PkError pk_get_last_error(void);

/**
 * Set last error for current thread.
 * 
 * @param error Error code to set
 * @note Call before returning NULL/false to provide error context
 * @note Thread-local - only affects current thread
 */
void pk_set_last_error(PkError error);

/**
 * Clear last error for current thread.
 * 
 * @note Resets error state to PK_OK
 * @note Call before operations that might fail to ensure clean state
 */
void pk_clear_last_error(void);

/**
 * Set last error with context information.
 * 
 * @param error Error code to set
 * @param fmt Printf-style format string for context
 * @param ... Format arguments
 * @note Context helps debugging (e.g., "Widget ID: button1")
 * @note Thread-local - only affects current thread
 */
void pk_set_last_error_with_context(PkError error, const char* fmt, ...);

/**
 * Get last error context string.
 * 
 * @return Context string or empty string if none (never NULL)
 * @note Thread-local - returns context for current thread
 * @note String remains valid until next error is set
 */
const char* pk_get_last_error_context(void);

/* Convenience macros for common patterns */

/**
 * Return on error with logging
 * Usage: PK_CHECK_RETURN(widget != NULL, PK_ERROR_NULL_PARAM);
 */
#define PK_CHECK_RETURN(condition, error) \
    do { \
        if (!(condition)) { \
            pk_set_last_error(error); \
            return error; \
        } \
    } while(0)

/**
 * Return NULL on error with logging
 * Usage: PK_CHECK_NULL(size > 0, PK_ERROR_INVALID_PARAM);
 */
#define PK_CHECK_NULL(condition, error) \
    do { \
        if (!(condition)) { \
            pk_set_last_error(error); \
            return NULL; \
        } \
    } while(0)

/**
 * Return false on error with logging
 * Usage: PK_CHECK_FALSE(ptr != NULL, PK_ERROR_NULL_PARAM);
 */
#define PK_CHECK_FALSE(condition, error) \
    do { \
        if (!(condition)) { \
            pk_set_last_error(error); \
            return false; \
        } \
    } while(0)

/**
 * Propagate error if not PK_OK
 * Usage: PK_PROPAGATE_ERROR(some_function());
 */
#define PK_PROPAGATE_ERROR(err) \
    do { \
        PkError _err = (err); \
        if (_err != PK_OK) { \
            return _err; \
        } \
    } while(0)

/**
 * Check condition and set error with context
 * Usage: PK_CHECK_NULL_WITH_CONTEXT(widget, PK_ERROR_NULL_PARAM, "widget_id=%s", id);
 */
#define PK_CHECK_NULL_WITH_CONTEXT(condition, error, fmt, ...) \
    do { \
        if (!(condition)) { \
            pk_set_last_error_with_context(error, fmt, ##__VA_ARGS__); \
            return NULL; \
        } \
    } while(0)

/**
 * Check condition and return false with context
 * Usage: PK_CHECK_FALSE_WITH_CONTEXT(ptr, PK_ERROR_NULL_PARAM, "function=%s", __func__);
 */
#define PK_CHECK_FALSE_WITH_CONTEXT(condition, error, fmt, ...) \
    do { \
        if (!(condition)) { \
            pk_set_last_error_with_context(error, fmt, ##__VA_ARGS__); \
            return false; \
        } \
    } while(0)

/**
 * Check condition and return error with context
 * Usage: PK_CHECK_ERROR_WITH_CONTEXT(ptr != NULL, PK_ERROR_NULL_PARAM, "widget is NULL");
 */
#define PK_CHECK_ERROR_WITH_CONTEXT(condition, error, fmt, ...) \
    do { \
        if (!(condition)) { \
            pk_set_last_error_with_context(error, fmt, ##__VA_ARGS__); \
            return error; \
        } \
    } while(0)

#endif /* PK_ERROR_H */