/**
 * @file logger.c
 * @brief PanelKit logging facility implementation
 */

#include "logger.h"
#include <zlog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <stdarg.h>
#include <math.h>

/* Version info - should be defined by build system */
#ifndef PANELKIT_VERSION
#define PANELKIT_VERSION "dev"
#endif

#ifndef PANELKIT_BUILD_DATE
#define PANELKIT_BUILD_DATE __DATE__ " " __TIME__
#endif

/* Global zlog category */
static zlog_category_t* g_cat = NULL;
static char g_app_name[64] = "panelkit";

/* Default configuration if file not found */
static const char* DEFAULT_ZLOG_CONFIG = 
"[global]\n"
"strict init = false\n"
"buffer min = 1024\n"
"buffer max = 2MB\n"
"rotate lock file = /tmp/panelkit_zlog.lock\n"
"default format = \"%d(%F %T.%ms) %-6V [%c] %m%n\"\n"
"\n"
"[formats]\n"
"simple = \"%d(%T) %-6V %m%n\"\n"
"detailed = \"%d(%F %T.%ms) %-6V [%c] [%f:%L] %m%n\"\n"
"\n"
"[rules]\n"
"*.DEBUG    \"/var/log/panelkit/panelkit.log\", 10MB * 5 ~ \"/var/log/panelkit/panelkit.#r.log\"\n"
"*.INFO     -\n"
"*.WARN     >stderr\n"
"*.ERROR    >stderr; \"/var/log/panelkit/error.log\", 5MB * 3 ~ \"/var/log/panelkit/error.#r.log\"\n"
"*.FATAL    >stderr; \"/var/log/panelkit/fatal.log\"\n";

/* Initialize logging system */
bool logger_init(const char* config_path, const char* app_name) {
    int rc;
    
    if (app_name) {
        strncpy(g_app_name, app_name, sizeof(g_app_name) - 1);
        g_app_name[sizeof(g_app_name) - 1] = '\0';
    }
    
    /* Create log directory if it doesn't exist */
    system("mkdir -p /var/log/panelkit");
    
    /* Try to initialize with provided config file */
    if (config_path) {
        rc = zlog_init(config_path);
        if (rc == 0) {
            g_cat = zlog_get_category(g_app_name);
            if (g_cat) {
                log_info("Logging initialized from config: %s", config_path);
                return true;
            }
        }
    }
    
    /* Fall back to default configuration */
    const char* temp_config = "/tmp/panelkit_zlog_default.conf";
    FILE* f = fopen(temp_config, "w");
    if (f) {
        fprintf(f, "%s", DEFAULT_ZLOG_CONFIG);
        fclose(f);
        
        rc = zlog_init(temp_config);
        if (rc == 0) {
            g_cat = zlog_get_category(g_app_name);
            if (g_cat) {
                log_info("Logging initialized with default configuration");
                return true;
            }
        }
    }
    
    /* Final fallback - stderr only */
    fprintf(stderr, "Failed to initialize zlog, falling back to stderr\n");
    return false;
}

/* Shutdown logging system */
void logger_shutdown(void) {
    if (g_cat) {
        log_info("Shutting down logging system");
        zlog_fini();
        g_cat = NULL;
    }
}

/* Core logging functions */
void log_debug(const char* fmt, ...) {
    if (!g_cat) {
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
        va_end(args);
        return;
    }
    
    va_list args;
    va_start(args, fmt);
    vzlog_debug(g_cat, fmt, args);
    va_end(args);
}

void log_info(const char* fmt, ...) {
    if (!g_cat) {
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
        va_end(args);
        return;
    }
    
    va_list args;
    va_start(args, fmt);
    vzlog_info(g_cat, fmt, args);
    va_end(args);
}

void log_notice(const char* fmt, ...) {
    if (!g_cat) {
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
        va_end(args);
        return;
    }
    
    va_list args;
    va_start(args, fmt);
    vzlog_notice(g_cat, fmt, args);
    va_end(args);
}

void log_warn(const char* fmt, ...) {
    if (!g_cat) {
        va_list args;
        va_start(args, fmt);
        fprintf(stderr, "WARN: ");
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
        va_end(args);
        return;
    }
    
    va_list args;
    va_start(args, fmt);
    vzlog_warn(g_cat, fmt, args);
    va_end(args);
}

void log_error(const char* fmt, ...) {
    if (!g_cat) {
        va_list args;
        va_start(args, fmt);
        fprintf(stderr, "ERROR: ");
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
        va_end(args);
        return;
    }
    
    va_list args;
    va_start(args, fmt);
    vzlog_error(g_cat, fmt, args);
    va_end(args);
}

void log_fatal(const char* fmt, ...) {
    if (!g_cat) {
        va_list args;
        va_start(args, fmt);
        fprintf(stderr, "FATAL: ");
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
        va_end(args);
        return;
    }
    
    va_list args;
    va_start(args, fmt);
    vzlog_fatal(g_cat, fmt, args);
    va_end(args);
}

/* System information logging */
void log_system_info(void) {
    struct utsname sys_info;
    char hostname[256];
    
    log_info("=== System Information ===");
    
    if (uname(&sys_info) == 0) {
        log_info("System: %s %s %s", sys_info.sysname, sys_info.release, sys_info.machine);
        log_info("Node: %s", sys_info.nodename);
    }
    
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        log_info("Hostname: %s", hostname);
    }
    
    log_info("Process ID: %d", getpid());
    log_info("User ID: %d", getuid());
}

void log_build_info(void) {
    log_info("=== Build Information ===");
    log_info("PanelKit Version: %s", PANELKIT_VERSION);
    log_info("Build Date: %s", PANELKIT_BUILD_DATE);
    
#ifdef __VERSION__
    log_info("Compiler: %s", __VERSION__);
#endif
    
#ifdef CMAKE_BUILD_TYPE
    log_info("Build Type: %s", CMAKE_BUILD_TYPE);
#endif
}

void log_display_info(int width, int height, const char* backend) {
    log_info("=== Display Configuration ===");
    log_info("Resolution: %dx%d", width, height);
    log_info("Backend: %s", backend ? backend : "unknown");
}

/* Panic handler */
void log_panic(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    if (g_cat) {
        vzlog_fatal(g_cat, fmt, args);
        /* zlog automatically flushes fatal messages */
    } else {
        fprintf(stderr, "PANIC: ");
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    
    va_end(args);
    abort();
}

/* Performance/metrics logging */
void log_frame_time(float ms) {
    static float min_ms = 1000.0f;
    static float max_ms = 0.0f;
    static float sum_ms = 0.0f;
    static int count = 0;
    static time_t last_report = 0;
    
    /* Update stats */
    if (ms < min_ms) min_ms = ms;
    if (ms > max_ms) max_ms = ms;
    sum_ms += ms;
    count++;
    
    /* Report every 5 seconds */
    time_t now = time(NULL);
    if (now - last_report >= 5) {
        float avg_ms = sum_ms / count;
        log_debug("Frame time stats: min=%.2fms avg=%.2fms max=%.2fms (%.1f FPS)", 
                  min_ms, avg_ms, max_ms, 1000.0f / avg_ms);
        
        /* Reset stats */
        min_ms = 1000.0f;
        max_ms = 0.0f;
        sum_ms = 0.0f;
        count = 0;
        last_report = now;
    }
}

void log_memory_usage(void) {
    FILE* f = fopen("/proc/self/status", "r");
    if (!f) {
        log_warn("Cannot read memory usage from /proc/self/status");
        return;
    }
    
    char line[256];
    long vmsize = 0, vmrss = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "VmSize: %ld kB", &vmsize) == 1) continue;
        if (sscanf(line, "VmRSS: %ld kB", &vmrss) == 1) continue;
    }
    fclose(f);
    
    log_info("Memory usage: VmSize=%ld MB, VmRSS=%ld MB", vmsize/1024, vmrss/1024);
}

/* Set log level at runtime */
bool logger_set_level(LogLevel level) {
    (void)level; /* Unused for now */
    
    if (!g_cat) return false;
    
    /* zlog doesn't support runtime level changes easily,
     * would need to reload config. For now, return false */
    log_warn("Runtime log level change not implemented");
    return false;
}

/* Structured logging helpers */
void log_event(const char* event, const char* fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    log_info("[EVENT:%s] %s", event, buffer);
}

void log_state_change(const char* component, const char* old_state, const char* new_state) {
    log_info("[STATE:%s] %s -> %s", component, old_state, new_state);
}