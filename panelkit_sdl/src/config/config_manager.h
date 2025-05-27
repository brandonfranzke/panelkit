#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "config_schema.h"
#include <stdbool.h>

/** Opaque configuration manager handle */
typedef struct ConfigManager ConfigManager;

/**
 * Configuration source priority levels.
 * Higher values override lower values.
 */
typedef enum {
    CONFIG_SOURCE_DEFAULT,  /**< Built-in defaults */
    CONFIG_SOURCE_SYSTEM,   /**< System-wide config (/etc) */
    CONFIG_SOURCE_USER,     /**< User config (~/.config) */
    CONFIG_SOURCE_LOCAL,    /**< Local config (./config.yaml) */
    CONFIG_SOURCE_CLI       /**< Command-line overrides */
} ConfigSource;

/**
 * Configuration manager initialization options.
 * Controls which config files are loaded and from where.
 */
typedef struct {
    const char* system_config_path;  /**< System config path (default: /etc/panelkit/config.yaml) */
    const char* user_config_path;    /**< User config path (default: ~/.config/panelkit/config.yaml) */
    const char* local_config_path;   /**< Local config path (default: ./config.yaml) */
    bool skip_system_config;         /**< Skip loading system config */
    bool skip_user_config;           /**< Skip loading user config */
    bool skip_local_config;          /**< Skip loading local config */
    bool use_fallback_on_error;      /**< Use defaults when config parse fails */
} ConfigManagerOptions;

/**
 * Configuration validation result.
 * Contains validation status and error details if invalid.
 */
typedef struct {
    bool valid;                 /**< Validation passed */
    char error_message[256];    /**< Error description if invalid */
    int error_line;             /**< Line number of error (-1 if N/A) */
    int error_column;           /**< Column number of error (-1 if N/A) */
} ValidationResult;

// Lifecycle

/**
 * Create a configuration manager with options.
 * 
 * @param options Initialization options (can be NULL for defaults)
 * @return New config manager or NULL on error (caller owns)
 */
ConfigManager* config_manager_create(const ConfigManagerOptions* options);

/**
 * Destroy a configuration manager.
 * 
 * @param manager Manager to destroy (can be NULL)
 */
void config_manager_destroy(ConfigManager* manager);

// Loading

/**
 * Load configuration from all configured sources.
 * 
 * @param manager Config manager (required)
 * @return true on success, false on error
 * @note Loads in order: defaults, system, user, local
 */
bool config_manager_load(ConfigManager* manager);

/**
 * Load configuration from a specific file.
 * 
 * @param manager Config manager (required)
 * @param path File path to load (required)
 * @param source Source priority level
 * @return true on success, false on error
 */
bool config_manager_load_file(ConfigManager* manager, const char* path, ConfigSource source);

/**
 * Apply command-line configuration override.
 * 
 * @param manager Config manager (required)
 * @param key Configuration key path (e.g., "display.width")
 * @param value Value to set (required)
 * @return true on success, false on error
 * @note Uses CONFIG_SOURCE_CLI priority
 */
bool config_manager_apply_override(ConfigManager* manager, const char* key, const char* value);

// Access

/**
 * Get the loaded configuration.
 * 
 * @param manager Config manager (required)
 * @return Immutable configuration or NULL if not loaded
 * @note Returned config is owned by manager - do not free
 */
const Config* config_manager_get_config(const ConfigManager* manager);

// Validation

/**
 * Validate a configuration file without loading it.
 * 
 * @param path File path to validate (required)
 * @return Validation result with error details if invalid
 */
ValidationResult config_validate_file(const char* path);

/**
 * Generate a default configuration file.
 * 
 * @param path Output file path (required)
 * @param include_comments Include descriptive comments
 * @return true on success, false on error
 */
bool config_generate_default(const char* path, bool include_comments);

// Value access

/**
 * Get integer configuration value by path.
 * 
 * @param manager Config manager (required)
 * @param path Dot-separated path (e.g., "display.width")
 * @param value Receives the value (required)
 * @return true if found and valid, false otherwise
 */
bool config_get_int(const ConfigManager* manager, const char* path, int* value);

/**
 * Get boolean configuration value by path.
 * 
 * @param manager Config manager (required)
 * @param path Dot-separated path (e.g., "display.fullscreen")
 * @param value Receives the value (required)
 * @return true if found and valid, false otherwise
 */
bool config_get_bool(const ConfigManager* manager, const char* path, bool* value);

/**
 * Get float configuration value by path.
 * 
 * @param manager Config manager (required)
 * @param path Dot-separated path (e.g., "ui.scale")
 * @param value Receives the value (required)
 * @return true if found and valid, false otherwise
 */
bool config_get_float(const ConfigManager* manager, const char* path, float* value);

/**
 * Get string configuration value by path.
 * 
 * @param manager Config manager (required)
 * @param path Dot-separated path (e.g., "api.base_url")
 * @param buffer Buffer to receive string (required)
 * @param buffer_size Size of buffer
 * @return true if found and copied, false otherwise
 */
bool config_get_string(const ConfigManager* manager, const char* path, char* buffer, size_t buffer_size);

// Monitoring

/**
 * Check if any loaded configuration file has changed on disk.
 * 
 * @param manager Config manager (required)
 * @return true if any file changed, false otherwise
 */
bool config_manager_has_changed(const ConfigManager* manager);

/**
 * Get the source of a configuration value.
 * 
 * @param manager Config manager (required)
 * @param path Dot-separated path to check
 * @return Source that provided the value
 */
ConfigSource config_get_source(const ConfigManager* manager, const char* path);

// Debugging

/**
 * Log a summary of loaded configuration.
 * 
 * @param manager Config manager (required)
 * @note Logs sources, values, and overrides
 */
void config_log_summary(const ConfigManager* manager);

#endif // CONFIG_MANAGER_H