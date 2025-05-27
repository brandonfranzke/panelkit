/**
 * @file error_notification.h
 * @brief Stub interface for future error notification widgets
 * 
 * This provides the hooks and interfaces for error notifications
 * that can be implemented when the UI layout system is complete.
 */

#ifndef PANELKIT_ERROR_NOTIFICATION_H
#define PANELKIT_ERROR_NOTIFICATION_H

#include "../core/error.h"
#include <stdbool.h>
#include <time.h>

/* Error severity for UI display */
typedef enum {
    ERROR_SEVERITY_INFO,
    ERROR_SEVERITY_WARNING,
    ERROR_SEVERITY_ERROR,
    ERROR_SEVERITY_CRITICAL
} ErrorSeverity;

/* Error notification entry */
typedef struct {
    PkError code;
    ErrorSeverity severity;
    char message[256];
    char context[256];
    time_t timestamp;
    bool acknowledged;
    int display_duration_ms;  /* 0 = persistent */
} ErrorNotification;

/* Global error notification callback type */
typedef void (*error_notification_callback)(const ErrorNotification* notification, void* user_data);

/**
 * Initialize error notification system.
 * 
 * @return true on success
 * 
 * @note This is a stub that prepares the infrastructure
 */
bool error_notification_init(void);

/**
 * Shutdown error notification system.
 */
void error_notification_shutdown(void);

/**
 * Queue an error for UI display.
 * 
 * @param error Error code
 * @param severity Display severity
 * @param message User-friendly message
 * @param duration_ms Display duration (0 = persistent)
 * 
 * @note This is a stub that logs the error for now
 */
void error_notification_queue(PkError error, ErrorSeverity severity,
                            const char* message, int duration_ms);

/**
 * Set callback for error notifications.
 * 
 * @param callback Function to call when errors are queued
 * @param user_data User data for callback
 * 
 * @note This allows UI widgets to hook into the error system
 */
void error_notification_set_callback(error_notification_callback callback, void* user_data);

/**
 * Get severity for an error code.
 * 
 * @param error Error code
 * @return Appropriate severity level
 */
ErrorSeverity error_notification_get_severity(PkError error);

/**
 * Convenience macro to queue error with auto-severity.
 */
#define ERROR_NOTIFY(error, message) \
    error_notification_queue(error, error_notification_get_severity(error), message, 0)

/**
 * Convenience macro for transient notifications.
 */
#define ERROR_NOTIFY_TRANSIENT(error, message, duration) \
    error_notification_queue(error, error_notification_get_severity(error), message, duration)

#endif /* PANELKIT_ERROR_NOTIFICATION_H */