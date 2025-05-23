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

/* Log levels matching zlog's levels */
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_NOTICE,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} LogLevel;

/* Initialize logging system
 * @param config_path Path to zlog configuration file (NULL for default)
 * @param app_name Application name for logging context
 * @return true on success, false on failure
 */
bool logger_init(const char* config_path, const char* app_name);

/* Shutdown logging system */
void logger_shutdown(void);

/* Core logging functions */
void log_debug(const char* fmt, ...);
void log_info(const char* fmt, ...);
void log_notice(const char* fmt, ...);
void log_warn(const char* fmt, ...);
void log_error(const char* fmt, ...);
void log_fatal(const char* fmt, ...);

/* System information logging */
void log_system_info(void);
void log_build_info(void);
void log_display_info(int width, int height, const char* backend);

/* Error helper macros */
#define LOG_SDL_ERROR(msg) log_error("%s: SDL Error: %s", msg, SDL_GetError())
#define LOG_ERRNO(msg) log_error("%s: System Error: %s", msg, strerror(errno))
#define LOG_DRM_ERROR(msg, err) log_error("%s: DRM Error: %d", msg, err)

/* Panic handler - logs fatal error and terminates */
void log_panic(const char* fmt, ...) __attribute__((noreturn));

/* Assert with logging */
#define LOG_ASSERT(cond, msg, ...) \
    do { \
        if (!(cond)) { \
            log_fatal("Assertion failed: " msg, ##__VA_ARGS__); \
            log_fatal("  Condition: %s", #cond); \
            log_fatal("  Location: %s:%d", __FILE__, __LINE__); \
            abort(); \
        } \
    } while(0)

/* Performance/metrics logging */
void log_frame_time(float ms);
void log_memory_usage(void);

/* Set log level at runtime (if supported by configuration) */
bool logger_set_level(LogLevel level);

/* Structured logging helpers */
void log_event(const char* event, const char* fmt, ...);
void log_state_change(const char* component, const char* old_state, const char* new_state);

#endif /* PANELKIT_LOGGER_H */