#include "error.h"
#include <pthread.h>

/* Thread-local storage for last error */
static pthread_key_t error_key;
static pthread_once_t error_key_once = PTHREAD_ONCE_INIT;

/* Initialize thread-local storage key */
static void init_error_key(void) {
    pthread_key_create(&error_key, NULL);
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
    pthread_once(&error_key_once, init_error_key);
    
    void* error_ptr = pthread_getspecific(error_key);
    if (error_ptr == NULL) {
        return PK_OK;
    }
    
    return (PkError)(intptr_t)error_ptr;
}

/* Set last error for current thread */
void pk_set_last_error(PkError error) {
    pthread_once(&error_key_once, init_error_key);
    pthread_setspecific(error_key, (void*)(intptr_t)error);
}

/* Clear last error for current thread */
void pk_clear_last_error(void) {
    pk_set_last_error(PK_OK);
}