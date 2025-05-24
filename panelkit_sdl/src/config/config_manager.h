#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "config_schema.h"
#include <stdbool.h>

// Forward declarations
typedef struct ConfigManager ConfigManager;

// Configuration source types
typedef enum {
    CONFIG_SOURCE_DEFAULT,
    CONFIG_SOURCE_SYSTEM,
    CONFIG_SOURCE_USER,
    CONFIG_SOURCE_LOCAL,
    CONFIG_SOURCE_CLI
} ConfigSource;

// Configuration manager options
typedef struct {
    const char* system_config_path;  // Default: /etc/panelkit/config.yaml
    const char* user_config_path;    // Default: ~/.config/panelkit/config.yaml
    const char* local_config_path;   // Default: ./config.yaml
    bool skip_system_config;
    bool skip_user_config;
    bool skip_local_config;
} ConfigManagerOptions;

// Validation result
typedef struct {
    bool valid;
    char error_message[256];
    int error_line;
    int error_column;
} ValidationResult;

// Create configuration manager with options
ConfigManager* config_manager_create(const ConfigManagerOptions* options);

// Destroy configuration manager
void config_manager_destroy(ConfigManager* manager);

// Load configuration from all sources
bool config_manager_load(ConfigManager* manager);

// Load configuration from specific file
bool config_manager_load_file(ConfigManager* manager, const char* path, ConfigSource source);

// Apply command-line overrides (key=value format)
bool config_manager_apply_override(ConfigManager* manager, const char* key, const char* value);

// Get the loaded configuration (immutable)
const Config* config_manager_get_config(const ConfigManager* manager);

// Validate a configuration file without loading
ValidationResult config_validate_file(const char* path);

// Generate default configuration to file
bool config_generate_default(const char* path, bool include_comments);

// Get configuration value by path (e.g., "display.width")
bool config_get_int(const ConfigManager* manager, const char* path, int* value);
bool config_get_bool(const ConfigManager* manager, const char* path, bool* value);
bool config_get_float(const ConfigManager* manager, const char* path, float* value);
bool config_get_string(const ConfigManager* manager, const char* path, char* buffer, size_t buffer_size);

// Check if configuration has changed on disk
bool config_manager_has_changed(const ConfigManager* manager);

// Get source of a configuration value
ConfigSource config_get_source(const ConfigManager* manager, const char* path);

// Logging helper for configuration
void config_log_summary(const ConfigManager* manager);

#endif // CONFIG_MANAGER_H