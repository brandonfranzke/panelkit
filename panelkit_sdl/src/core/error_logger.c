/**
 * @file error_logger.c
 * @brief Error logging implementation with rotating file support
 */

#include "error_logger.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

/* Internal state */
typedef struct {
    ErrorLogConfig config;
    FILE* current_file;
    char current_filename[512];
    size_t current_file_size;
    size_t total_errors;
    size_t errors_since_rotation;
    pthread_mutex_t mutex;
    bool initialized;
    error_log_callback callback;
    void* callback_user_data;
} ErrorLoggerState;

static ErrorLoggerState g_error_logger = {0};

/* Forward declarations */
static bool ensure_log_directory(const char* dir);
static bool open_new_log_file(void);
static void rotate_log_files(void);
static void write_log_header(FILE* file);
static const char* error_code_to_string(PkError error);

ErrorLogConfig error_logger_default_config(void) {
    ErrorLogConfig config = {
        .log_directory = ERROR_LOG_DEFAULT_DIR,
        .log_prefix = ERROR_LOG_DEFAULT_PREFIX,
        .max_file_size = ERROR_LOG_DEFAULT_MAX_SIZE,
        .max_files = ERROR_LOG_DEFAULT_MAX_FILES,
        .log_to_console = ERROR_LOG_DEFAULT_TO_CONSOLE,
        .include_context = true,
        .include_backtrace = false  /* Not implemented yet */
    };
    return config;
}

bool error_logger_init(const ErrorLogConfig* config) {
    if (g_error_logger.initialized) {
        log_warn("Error logger already initialized");
        return true;
    }
    
    /* Use provided config or defaults */
    if (config) {
        g_error_logger.config = *config;
    } else {
        g_error_logger.config = error_logger_default_config();
    }
    
    /* Initialize mutex */
    if (pthread_mutex_init(&g_error_logger.mutex, NULL) != 0) {
        log_error("Failed to initialize error logger mutex");
        return false;
    }
    
    /* Ensure log directory exists */
    if (!ensure_log_directory(g_error_logger.config.log_directory)) {
        pthread_mutex_destroy(&g_error_logger.mutex);
        return false;
    }
    
    /* Open initial log file */
    if (!open_new_log_file()) {
        pthread_mutex_destroy(&g_error_logger.mutex);
        return false;
    }
    
    g_error_logger.initialized = true;
    log_info("Error logger initialized: %s/%s_*.log (max %zu bytes, %d files)",
             g_error_logger.config.log_directory,
             g_error_logger.config.log_prefix,
             g_error_logger.config.max_file_size,
             g_error_logger.config.max_files);
    
    return true;
}

void error_logger_shutdown(void) {
    if (!g_error_logger.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_error_logger.mutex);
    
    if (g_error_logger.current_file) {
        /* Write shutdown marker */
        fprintf(g_error_logger.current_file, 
                "\n=== Error logger shutdown at %s ===\n",
                ctime(&(time_t){time(NULL)}));
        fclose(g_error_logger.current_file);
        g_error_logger.current_file = NULL;
    }
    
    pthread_mutex_unlock(&g_error_logger.mutex);
    pthread_mutex_destroy(&g_error_logger.mutex);
    
    g_error_logger.initialized = false;
    log_info("Error logger shutdown (logged %zu total errors)", 
             g_error_logger.total_errors);
}

void error_logger_log(PkError error, const char* file, int line, 
                     const char* function, const char* context) {
    if (!g_error_logger.initialized) {
        return;
    }
    
    /* Build log entry */
    ErrorLogEntry entry = {
        .timestamp = time(NULL),
        .error_code = error,
        .line = line,
        .pid = getpid(),
        .thread_id = pthread_self()
    };
    
    /* Copy strings safely */
    if (function) {
        strncpy(entry.function, function, sizeof(entry.function) - 1);
    }
    if (file) {
        /* Extract just filename from path */
        const char* filename = strrchr(file, '/');
        filename = filename ? filename + 1 : file;
        strncpy(entry.file, filename, sizeof(entry.file) - 1);
    }
    if (context) {
        strncpy(entry.context, context, sizeof(entry.context) - 1);
    }
    
    error_logger_log_entry(&entry);
}

void error_logger_log_entry(const ErrorLogEntry* entry) {
    if (!g_error_logger.initialized || !entry) {
        return;
    }
    
    pthread_mutex_lock(&g_error_logger.mutex);
    
    /* Check if rotation needed */
    if (g_error_logger.current_file_size >= g_error_logger.config.max_file_size) {
        rotate_log_files();
        open_new_log_file();
    }
    
    /* Format timestamp */
    char timestamp[64];
    time_t entry_time = entry->timestamp;
    struct tm* tm_info = localtime(&entry_time);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    /* Write to file */
    if (g_error_logger.current_file) {
        int bytes_written = fprintf(g_error_logger.current_file,
            "[%s] ERROR %d (%s) in %s:%d %s()\n",
            timestamp,
            entry->error_code,
            error_code_to_string(entry->error_code),
            entry->file,
            entry->line,
            entry->function);
        
        if (g_error_logger.config.include_context && entry->context[0]) {
            bytes_written += fprintf(g_error_logger.current_file,
                "  Context: %s\n", entry->context);
        }
        
        bytes_written += fprintf(g_error_logger.current_file,
            "  Process: %d, Thread: %lu\n\n",
            entry->pid,
            (unsigned long)entry->thread_id);
        
        fflush(g_error_logger.current_file);
        g_error_logger.current_file_size += bytes_written;
    }
    
    /* Also log to console if configured */
    if (g_error_logger.config.log_to_console) {
        log_error("[ERRLOG] %s in %s:%d - %s",
                  error_code_to_string(entry->error_code),
                  entry->file,
                  entry->line,
                  entry->context);
    }
    
    /* Update statistics */
    g_error_logger.total_errors++;
    g_error_logger.errors_since_rotation++;
    
    /* Call callback if registered */
    if (g_error_logger.callback) {
        g_error_logger.callback(entry, g_error_logger.callback_user_data);
    }
    
    pthread_mutex_unlock(&g_error_logger.mutex);
}

bool error_logger_rotate(void) {
    if (!g_error_logger.initialized) {
        return false;
    }
    
    pthread_mutex_lock(&g_error_logger.mutex);
    rotate_log_files();
    bool success = open_new_log_file();
    pthread_mutex_unlock(&g_error_logger.mutex);
    
    return success;
}

const char* error_logger_get_current_file(void) {
    if (!g_error_logger.initialized) {
        return NULL;
    }
    return g_error_logger.current_filename;
}

void error_logger_get_stats(size_t* total_errors, 
                          size_t* errors_since_rotation,
                          size_t* current_file_size) {
    if (!g_error_logger.initialized) {
        if (total_errors) *total_errors = 0;
        if (errors_since_rotation) *errors_since_rotation = 0;
        if (current_file_size) *current_file_size = 0;
        return;
    }
    
    pthread_mutex_lock(&g_error_logger.mutex);
    if (total_errors) *total_errors = g_error_logger.total_errors;
    if (errors_since_rotation) *errors_since_rotation = g_error_logger.errors_since_rotation;
    if (current_file_size) *current_file_size = g_error_logger.current_file_size;
    pthread_mutex_unlock(&g_error_logger.mutex);
}

void error_logger_set_callback(error_log_callback callback, void* user_data) {
    pthread_mutex_lock(&g_error_logger.mutex);
    g_error_logger.callback = callback;
    g_error_logger.callback_user_data = user_data;
    pthread_mutex_unlock(&g_error_logger.mutex);
}

/* Internal functions */

static bool ensure_log_directory(const char* dir) {
    struct stat st;
    if (stat(dir, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    
    /* Create directory */
    if (mkdir(dir, 0755) != 0) {
        log_error("Failed to create log directory %s: %s", dir, strerror(errno));
        return false;
    }
    
    log_info("Created log directory: %s", dir);
    return true;
}

static bool open_new_log_file(void) {
    /* Close current file if open */
    if (g_error_logger.current_file) {
        fclose(g_error_logger.current_file);
        g_error_logger.current_file = NULL;
    }
    
    /* Generate filename with timestamp */
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);
    
    snprintf(g_error_logger.current_filename, 
             sizeof(g_error_logger.current_filename),
             "%s/%s_%s.log",
             g_error_logger.config.log_directory,
             g_error_logger.config.log_prefix,
             timestamp);
    
    /* Open new file */
    g_error_logger.current_file = fopen(g_error_logger.current_filename, "w");
    if (!g_error_logger.current_file) {
        log_error("Failed to open error log file %s: %s",
                  g_error_logger.current_filename, strerror(errno));
        return false;
    }
    
    /* Write header */
    write_log_header(g_error_logger.current_file);
    
    /* Reset counters */
    g_error_logger.current_file_size = ftell(g_error_logger.current_file);
    g_error_logger.errors_since_rotation = 0;
    
    log_info("Opened new error log: %s", g_error_logger.current_filename);
    return true;
}

static void rotate_log_files(void) {
    if (!g_error_logger.current_file) {
        return;
    }
    
    /* Close current file */
    fprintf(g_error_logger.current_file, 
            "\n=== Log rotated at %s ===\n",
            ctime(&(time_t){time(NULL)}));
    fclose(g_error_logger.current_file);
    g_error_logger.current_file = NULL;
    
    /* TODO: Implement deletion of old files beyond max_files */
    /* For now, we just create new files */
}

static void write_log_header(FILE* file) {
    time_t now = time(NULL);
    fprintf(file, "=== PanelKit Error Log ===\n");
    fprintf(file, "Started: %s", ctime(&now));
    fprintf(file, "Process ID: %d\n", getpid());
    fprintf(file, "Configuration:\n");
    fprintf(file, "  Max file size: %zu bytes\n", g_error_logger.config.max_file_size);
    fprintf(file, "  Max files: %d\n", g_error_logger.config.max_files);
    fprintf(file, "  Include context: %s\n", g_error_logger.config.include_context ? "yes" : "no");
    fprintf(file, "===========================\n\n");
}

static const char* error_code_to_string(PkError error) {
    switch (error) {
        case PK_OK: return "OK";
        case PK_ERROR_NULL_PARAM: return "NULL_PARAM";
        case PK_ERROR_INVALID_PARAM: return "INVALID_PARAM";
        case PK_ERROR_OUT_OF_MEMORY: return "OUT_OF_MEMORY";
        case PK_ERROR_RESOURCE_LIMIT: return "RESOURCE_LIMIT";
        case PK_ERROR_NOT_FOUND: return "NOT_FOUND";
        case PK_ERROR_ALREADY_EXISTS: return "ALREADY_EXISTS";
        case PK_ERROR_INVALID_STATE: return "INVALID_STATE";
        case PK_ERROR_NOT_INITIALIZED: return "NOT_INITIALIZED";
        case PK_ERROR_ALREADY_INITIALIZED: return "ALREADY_INITIALIZED";
        case PK_ERROR_SDL: return "SDL_ERROR";
        case PK_ERROR_SYSTEM: return "SYSTEM_ERROR";
        case PK_ERROR_NETWORK: return "NETWORK_ERROR";
        case PK_ERROR_TIMEOUT: return "TIMEOUT";
        case PK_ERROR_PARSE: return "PARSE_ERROR";
        case PK_ERROR_INVALID_DATA: return "INVALID_DATA";
        case PK_ERROR_INVALID_CONFIG: return "INVALID_CONFIG";
        case PK_ERROR_WIDGET_NOT_FOUND: return "WIDGET_NOT_FOUND";
        case PK_ERROR_WIDGET_TREE_FULL: return "WIDGET_TREE_FULL";
        case PK_ERROR_WIDGET_INVALID_TYPE: return "WIDGET_INVALID_TYPE";
        case PK_ERROR_EVENT_QUEUE_FULL: return "EVENT_QUEUE_FULL";
        case PK_ERROR_EVENT_NOT_FOUND: return "EVENT_NOT_FOUND";
        case PK_ERROR_EVENT_HANDLER_FAILED: return "EVENT_HANDLER_FAILED";
        case PK_ERROR_RENDER_FAILED: return "RENDER_FAILED";
        case PK_ERROR_INPUT_SOURCE_FAILED: return "INPUT_SOURCE_FAILED";
        case PK_ERROR_INPUT_DEVICE_NOT_FOUND: return "INPUT_DEVICE_NOT_FOUND";
        case PK_ERROR_INPUT_INIT_FAILED: return "INPUT_INIT_FAILED";
        case PK_ERROR_DISPLAY_INIT_FAILED: return "DISPLAY_INIT_FAILED";
        case PK_ERROR_DISPLAY_MODE_FAILED: return "DISPLAY_MODE_FAILED";
        case PK_ERROR_DISPLAY_DISCONNECTED: return "DISPLAY_DISCONNECTED";
        default: return "UNKNOWN_ERROR";
    }
}