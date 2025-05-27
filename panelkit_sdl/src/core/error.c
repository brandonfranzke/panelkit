#include "error.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* Thread-local error information */
typedef struct {
    PkError error;
    char context[256];
} ErrorInfo;

/* Thread-local storage for error info */
static pthread_key_t error_key;
static pthread_once_t error_key_once = PTHREAD_ONCE_INIT;

/* Cleanup function for thread-local storage */
static void free_error_info(void* ptr) {
    free(ptr);
}

/* Initialize thread-local storage key */
static void init_error_key(void) {
    pthread_key_create(&error_key, free_error_info);
}

/* Get or create thread-local error info */
static ErrorInfo* get_error_info(void) {
    pthread_once(&error_key_once, init_error_key);
    
    ErrorInfo* info = pthread_getspecific(error_key);
    if (info == NULL) {
        info = calloc(1, sizeof(ErrorInfo));
        if (info != NULL) {
            pthread_setspecific(error_key, info);
        }
    }
    return info;
}

/* Get error string for error code */
const char* pk_error_string(PkError error) {
    switch (error) {
        /* Success */
        case PK_OK:
            return "Success";
            
        /* Parameter errors */
        case PK_ERROR_NULL_PARAM:
            return "Null parameter";
        case PK_ERROR_INVALID_PARAM:
            return "Invalid parameter";
            
        /* Resource errors */
        case PK_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case PK_ERROR_RESOURCE_LIMIT:
            return "Resource limit exceeded";
        case PK_ERROR_NOT_FOUND:
            return "Not found";
        case PK_ERROR_ALREADY_EXISTS:
            return "Already exists";
            
        /* State errors */
        case PK_ERROR_INVALID_STATE:
            return "Invalid state";
        case PK_ERROR_NOT_INITIALIZED:
            return "Not initialized";
        case PK_ERROR_ALREADY_INITIALIZED:
            return "Already initialized";
            
        /* External errors */
        case PK_ERROR_SDL:
            return "SDL error";
        case PK_ERROR_SYSTEM:
            return "System error";
        case PK_ERROR_NETWORK:
            return "Network error";
        case PK_ERROR_TIMEOUT:
            return "Operation timed out";
            
        /* Data errors */
        case PK_ERROR_PARSE:
            return "Parse error";
        case PK_ERROR_INVALID_DATA:
            return "Invalid data";
        case PK_ERROR_INVALID_CONFIG:
            return "Invalid configuration";
            
        /* Widget-specific errors */
        case PK_ERROR_WIDGET_NOT_FOUND:
            return "Widget not found";
        case PK_ERROR_WIDGET_TREE_FULL:
            return "Widget tree full";
        case PK_ERROR_WIDGET_INVALID_TYPE:
            return "Invalid widget type";
            
        /* Event system errors */
        case PK_ERROR_EVENT_QUEUE_FULL:
            return "Event queue full";
        case PK_ERROR_EVENT_NOT_FOUND:
            return "Event not found";
        case PK_ERROR_EVENT_HANDLER_FAILED:
            return "Event handler failed";
            
        default:
            return "Unknown error";
    }
}

/* Get last error for current thread */
PkError pk_get_last_error(void) {
    ErrorInfo* info = get_error_info();
    return info ? info->error : PK_OK;
}

/* Set last error for current thread */
void pk_set_last_error(PkError error) {
    ErrorInfo* info = get_error_info();
    if (info) {
        info->error = error;
        info->context[0] = '\0';  // Clear context
    }
}

/* Set last error with context */
void pk_set_last_error_with_context(PkError error, const char* fmt, ...) {
    ErrorInfo* info = get_error_info();
    if (info) {
        info->error = error;
        
        if (fmt) {
            va_list args;
            va_start(args, fmt);
            vsnprintf(info->context, sizeof(info->context), fmt, args);
            va_end(args);
        } else {
            info->context[0] = '\0';
        }
    }
}

/* Get last error context */
const char* pk_get_last_error_context(void) {
    ErrorInfo* info = get_error_info();
    return info ? info->context : "";
}

/* Clear last error for current thread */
void pk_clear_last_error(void) {
    ErrorInfo* info = get_error_info();
    if (info) {
        info->error = PK_OK;
        info->context[0] = '\0';
    }
}