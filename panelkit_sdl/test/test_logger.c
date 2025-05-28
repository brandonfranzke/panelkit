/**
 * @file test_logger.c
 * @brief Minimal logger implementation for tests
 */

#include "core/logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

static LogLevel g_log_level = LOG_LEVEL_DEBUG;

bool logger_init(const char* config_path, const char* app_name) {
    (void)config_path;
    (void)app_name;
    return true;
}

void logger_shutdown(void) {
    // Nothing to do
}

bool logger_set_level(LogLevel level) {
    g_log_level = level;
    return true;
}

static void log_message(LogLevel level, const char* fmt, va_list args) {
    if (level < g_log_level) return;
    
    const char* level_str = "";
    switch (level) {
        case LOG_LEVEL_DEBUG:  level_str = "DEBUG"; break;
        case LOG_LEVEL_INFO:   level_str = "INFO"; break;
        case LOG_LEVEL_NOTICE: level_str = "NOTICE"; break;
        case LOG_LEVEL_WARN:   level_str = "WARN"; break;
        case LOG_LEVEL_ERROR:  level_str = "ERROR"; break;
        case LOG_LEVEL_FATAL:  level_str = "FATAL"; break;
    }
    
    fprintf(stderr, "[%s] ", level_str);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

void log_debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_LEVEL_DEBUG, fmt, args);
    va_end(args);
}

void log_info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_LEVEL_INFO, fmt, args);
    va_end(args);
}

void log_notice(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_LEVEL_NOTICE, fmt, args);
    va_end(args);
}

void log_warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_LEVEL_WARN, fmt, args);
    va_end(args);
}

void log_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_LEVEL_ERROR, fmt, args);
    va_end(args);
}

void log_fatal(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_LEVEL_FATAL, fmt, args);
    va_end(args);
}

void log_errno(const char* msg) {
    fprintf(stderr, "[ERROR] %s: %s\n", msg, strerror(errno));
}

void log_system_info(void) {
    fprintf(stderr, "[INFO] System info logging (stub)\n");
}

void log_state_change(const char* component, const char* old_state, 
                     const char* new_state) {
    fprintf(stderr, "[STATE] %s: %s -> %s\n", component, old_state, new_state);
}

void logger_flush(void) {
    fflush(stderr);
}

void log_panic(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[PANIC] ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    abort();
}

// Error logger interface for error.c
void error_logger_log(const char* file, int line, const char* func,
                     const char* error_name, const char* context) {
    fprintf(stderr, "[ERROR] %s:%d in %s(): %s - %s\n", 
            file, line, func, error_name, context ? context : "");
}