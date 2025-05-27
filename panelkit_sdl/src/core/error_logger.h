/**
 * @file error_logger.h
 * @brief Error logging subsystem for persistent error tracking
 * 
 * This module provides dedicated error logging functionality that integrates
 * with the main error system to capture all errors to rotating log files.
 * Designed for field debugging and post-mortem analysis.
 */

#ifndef PANELKIT_ERROR_LOGGER_H
#define PANELKIT_ERROR_LOGGER_H

#include "error.h"
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

/* Error log configuration */
typedef struct {
    const char* log_directory;      /* Directory for error logs */
    const char* log_prefix;         /* Filename prefix (e.g., "panelkit_errors") */
    size_t max_file_size;          /* Max size per log file in bytes */
    int max_files;                 /* Number of rotating files to keep */
    bool log_to_console;           /* Also output to console */
    bool include_context;          /* Include error context in logs */
    bool include_backtrace;        /* Include stack trace if available */
} ErrorLogConfig;

/* Error log entry for structured logging */
typedef struct {
    time_t timestamp;
    PkError error_code;
    char function[64];
    char file[64];
    int line;
    char context[256];
    pid_t pid;
    pthread_t thread_id;
} ErrorLogEntry;

/**
 * Initialize the error logging system.
 * 
 * @param config Configuration for error logging
 * @return true on success, false on failure
 * 
 * @note This should be called early in application startup
 * @note The logger will create the log directory if it doesn't exist
 */
bool error_logger_init(const ErrorLogConfig* config);

/**
 * Shutdown the error logging system.
 * 
 * Flushes any pending log entries and closes log files.
 */
void error_logger_shutdown(void);

/**
 * Log an error with context.
 * 
 * @param error The error code
 * @param file Source file where error occurred
 * @param line Line number where error occurred  
 * @param function Function name where error occurred
 * @param context Context string with details
 * 
 * @note This is typically called automatically by pk_set_last_error_with_context
 */
void error_logger_log(PkError error, const char* file, int line, 
                     const char* function, const char* context);

/**
 * Log an error entry (structured).
 * 
 * @param entry Pre-filled error log entry
 * 
 * @note This allows for more detailed error information
 */
void error_logger_log_entry(const ErrorLogEntry* entry);

/**
 * Force rotation of error log files.
 * 
 * @return true if rotation successful, false otherwise
 * 
 * @note Normally rotation happens automatically based on file size
 */
bool error_logger_rotate(void);

/**
 * Get the path to the current error log file.
 * 
 * @return Path to current log file, or NULL if not initialized
 * 
 * @note The returned string is owned by the logger, do not free
 */
const char* error_logger_get_current_file(void);

/**
 * Get error logging statistics.
 * 
 * @param total_errors Output: Total errors logged
 * @param errors_since_rotation Output: Errors in current file
 * @param current_file_size Output: Size of current log file
 */
void error_logger_get_stats(size_t* total_errors, 
                          size_t* errors_since_rotation,
                          size_t* current_file_size);

/**
 * Set a callback for error log events.
 * 
 * @param callback Function called when errors are logged
 * @param user_data User data passed to callback
 * 
 * @note Useful for updating UI when errors occur
 */
typedef void (*error_log_callback)(const ErrorLogEntry* entry, void* user_data);
void error_logger_set_callback(error_log_callback callback, void* user_data);

/**
 * Integration macro for automatic error logging.
 * 
 * This macro can be used to wrap pk_set_last_error_with_context
 * to automatically log errors.
 */
#define PK_LOG_ERROR_WITH_CONTEXT(error, fmt, ...) \
    do { \
        pk_set_last_error_with_context(error, fmt, ##__VA_ARGS__); \
        error_logger_log(error, __FILE__, __LINE__, __func__, \
                        pk_get_last_error_context()); \
    } while(0)

/* Default configuration values */
#define ERROR_LOG_DEFAULT_DIR "logs"
#define ERROR_LOG_DEFAULT_PREFIX "panelkit_errors"
#define ERROR_LOG_DEFAULT_MAX_SIZE (10 * 1024 * 1024)  /* 10MB */
#define ERROR_LOG_DEFAULT_MAX_FILES 5
#define ERROR_LOG_DEFAULT_TO_CONSOLE true

/**
 * Get default error log configuration.
 * 
 * @return Configuration with sensible defaults
 */
ErrorLogConfig error_logger_default_config(void);

#endif /* PANELKIT_ERROR_LOGGER_H */