/**
 * @file logger.h
 * @brief PanelKit logging facility built on zlog
 * 
 * This module provides a clean abstraction over zlog with additional
 * functionality for system information, panic handling, and common
 * error patterns.
 */

#ifndef PANELKIT_LOGGER_H
#define PANELKIT_LOGGER_H

#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

/**
 * Log severity levels.
 * Maps to underlying zlog severity levels.
 */
typedef enum {
    LOG_LEVEL_DEBUG = 0,    /**< Detailed debug information */
    LOG_LEVEL_INFO,         /**< General informational messages */
    LOG_LEVEL_NOTICE,       /**< Normal but significant events */
    LOG_LEVEL_WARN,         /**< Warning conditions */
    LOG_LEVEL_ERROR,        /**< Error conditions */
    LOG_LEVEL_FATAL         /**< Fatal errors requiring termination */
} LogLevel;

/**
 * Initialize the logging system.
 * 
 * @param config_path Path to zlog config file (NULL for default)
 * @param app_name Application name for log context
 * @return true on success, false on failure
 * @note Must be called before any logging functions
 */
bool logger_init(const char* config_path, const char* app_name);

/**
 * Shutdown the logging system.
 * 
 * @note Flushes all pending log messages
 */
void logger_shutdown(void);

// Core logging functions

/**
 * Log debug message.
 * 
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
void log_debug(const char* fmt, ...);

/**
 * Log informational message.
 * 
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
void log_info(const char* fmt, ...);

/**
 * Log notice message.
 * 
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
void log_notice(const char* fmt, ...);

/**
 * Log warning message.
 * 
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
void log_warn(const char* fmt, ...);

/**
 * Log error message.
 * 
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
void log_error(const char* fmt, ...);

/**
 * Log fatal error message.
 * 
 * @param fmt Printf-style format string
 * @param ... Format arguments
 * @note Does not terminate - use log_panic() for that
 */
void log_fatal(const char* fmt, ...);

// System information logging

/**
 * Log system information.
 * 
 * @note Logs OS, CPU, memory details
 */
void log_system_info(void);

/**
 * Log build information.
 * 
 * @note Logs version, compiler, build flags
 */
void log_build_info(void);

/**
 * Log display configuration.
 * 
 * @param width Display width in pixels
 * @param height Display height in pixels
 * @param backend Backend name string
 */
void log_display_info(int width, int height, const char* backend);

/** Log SDL error with context message */
#define LOG_SDL_ERROR(msg) log_error("%s: SDL Error: %s", msg, SDL_GetError())

/** Log system errno with context message */
#define LOG_ERRNO(msg) log_error("%s: System Error: %s", msg, strerror(errno))

/** Log DRM error code with context message */
#define LOG_DRM_ERROR(msg, err) log_error("%s: DRM Error: %d", msg, err)

/**
 * Log fatal error and terminate.
 * 
 * @param fmt Printf-style format string
 * @param ... Format arguments
 * @note Never returns - terminates process
 */
void log_panic(const char* fmt, ...) __attribute__((noreturn));

/** Assert condition with logging on failure */
#define LOG_ASSERT(cond, msg, ...) \
    do { \
        if (!(cond)) { \
            log_fatal("Assertion failed: " msg, ##__VA_ARGS__); \
            log_fatal("  Condition: %s", #cond); \
            log_fatal("  Location: %s:%d", __FILE__, __LINE__); \
            abort(); \
        } \
    } while(0)

// Performance monitoring

/**
 * Log frame timing information.
 * 
 * @param ms Frame time in milliseconds
 */
void log_frame_time(float ms);

/**
 * Log current memory usage.
 * 
 * @note Logs process memory statistics
 */
void log_memory_usage(void);

/**
 * Set logging level at runtime.
 * 
 * @param level New minimum log level
 * @return true if level changed, false otherwise
 * @note May not work depending on zlog configuration
 */
bool logger_set_level(LogLevel level);

// Structured logging

/**
 * Log structured event.
 * 
 * @param event Event name/type
 * @param fmt Printf-style format for details
 * @param ... Format arguments
 */
void log_event(const char* event, const char* fmt, ...);

/**
 * Log component state transition.
 * 
 * @param component Component name
 * @param old_state Previous state name
 * @param new_state New state name
 */
void log_state_change(const char* component, const char* old_state, const char* new_state);

#endif /* PANELKIT_LOGGER_H */