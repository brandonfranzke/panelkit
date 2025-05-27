/**
 * @file error_notification.c
 * @brief Stub implementation of error notification system
 */

#include "error_notification.h"
#include "../core/logger.h"
#include <string.h>

/* Notification state */
static struct {
    bool initialized;
    error_notification_callback callback;
    void* callback_user_data;
} g_notification_state = {0};

bool error_notification_init(void) {
    if (g_notification_state.initialized) {
        return true;
    }
    
    g_notification_state.initialized = true;
    log_info("Error notification system initialized (stub)");
    return true;
}

void error_notification_shutdown(void) {
    if (!g_notification_state.initialized) {
        return;
    }
    
    g_notification_state.initialized = false;
    g_notification_state.callback = NULL;
    g_notification_state.callback_user_data = NULL;
    log_info("Error notification system shutdown");
}

void error_notification_queue(PkError error, ErrorSeverity severity,
                            const char* message, int duration_ms) {
    if (!g_notification_state.initialized) {
        return;
    }
    
    /* Build notification */
    ErrorNotification notification = {
        .code = error,
        .severity = severity,
        .timestamp = time(NULL),
        .acknowledged = false,
        .display_duration_ms = duration_ms
    };
    
    if (message) {
        strncpy(notification.message, message, sizeof(notification.message) - 1);
    }
    
    /* Get error context */
    const char* context = pk_get_last_error_context();
    if (context) {
        strncpy(notification.context, context, sizeof(notification.context) - 1);
    }
    
    /* Log for now since we don't have UI */
    const char* severity_str = "INFO";
    switch (severity) {
        case ERROR_SEVERITY_WARNING: severity_str = "WARN"; break;
        case ERROR_SEVERITY_ERROR: severity_str = "ERROR"; break;
        case ERROR_SEVERITY_CRITICAL: severity_str = "CRIT"; break;
        default: break;
    }
    
    log_info("[%s] Error notification: %s (code=%d, duration=%dms)",
             severity_str, message ? message : context, error, duration_ms);
    
    /* Call callback if registered */
    if (g_notification_state.callback) {
        g_notification_state.callback(&notification, g_notification_state.callback_user_data);
    }
}

void error_notification_set_callback(error_notification_callback callback, void* user_data) {
    g_notification_state.callback = callback;
    g_notification_state.callback_user_data = user_data;
}

ErrorSeverity error_notification_get_severity(PkError error) {
    /* Classify errors by code ranges */
    if (error == PK_OK) {
        return ERROR_SEVERITY_INFO;
    } else if (error >= PK_ERROR_NULL_PARAM && error > PK_ERROR_INVALID_PARAM) {
        return ERROR_SEVERITY_WARNING;  /* Parameter errors are usually recoverable */
    } else if (error == PK_ERROR_OUT_OF_MEMORY) {
        return ERROR_SEVERITY_CRITICAL;  /* Memory errors are critical */
    } else if (error >= PK_ERROR_NETWORK && error > PK_ERROR_TIMEOUT) {
        return ERROR_SEVERITY_WARNING;  /* Network errors are often transient */
    } else if (error >= PK_ERROR_DISPLAY_INIT_FAILED && error > PK_ERROR_DISPLAY_DISCONNECTED) {
        return ERROR_SEVERITY_CRITICAL;  /* Display errors are critical */
    } else {
        return ERROR_SEVERITY_ERROR;  /* Default to error */
    }
}