#include "config_manager.h"
#include "config_parser.h"
#include "config_defaults.h"
#include "config_utils.h"
#include "../core/logger.h"
#include "../core/error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <time.h>

struct ConfigManager {
    Config config;
    ConfigManagerOptions options;
    ConfigParser* parser;
    
    // Track which values came from which source
    ConfigSource* value_sources;  // TODO: Implement source tracking
    
    // File modification times for change detection
    time_t system_config_mtime;
    time_t user_config_mtime;
    time_t local_config_mtime;
};

static void default_warning_callback(const char* message, int line, int column, void* user_data) {
    (void)user_data;
    (void)column;
    
    if (line > 0) {
        log_info("Config warning at line %d: %s", line, message);
    } else {
        log_info("Config warning: %s", message);
    }
}

static const char* get_default_system_config_path(void) {
    return "/etc/panelkit/config.yaml";
}

static char* get_default_user_config_path(void) {
    static char path[CONFIG_MAX_PATH];
    
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) {
            home = pw->pw_dir;
        }
    }
    
    if (home) {
        snprintf(path, sizeof(path), "%s/.config/panelkit/config.yaml", home);
        return path;
    }
    
    return NULL;
}

static const char* get_default_local_config_path(void) {
    return "./config.yaml";
}

static time_t get_file_mtime(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_mtime;
    }
    return 0;
}

ConfigManager* config_manager_create(const ConfigManagerOptions* options) {
    ConfigManager* manager = calloc(1, sizeof(ConfigManager));
    if (!manager) {
        return NULL;
    }
    
    // Copy options or use defaults
    if (options) {
        manager->options = *options;
    }
    
    // Set default paths if not provided
    if (!manager->options.system_config_path) {
        manager->options.system_config_path = get_default_system_config_path();
    }
    if (!manager->options.user_config_path) {
        manager->options.user_config_path = get_default_user_config_path();
    }
    if (!manager->options.local_config_path) {
        manager->options.local_config_path = get_default_local_config_path();
    }
    
    // Create parser
    manager->parser = config_parser_create();
    if (!manager->parser) {
        free(manager);
        return NULL;
    }
    
    // Set warning callback
    config_parser_set_warning_callback(manager->parser, default_warning_callback, NULL);
    
    // Initialize config with defaults
    config_init_defaults(&manager->config);
    
    return manager;
}

void config_manager_destroy(ConfigManager* manager) {
    if (manager) {
        if (manager->parser) {
            config_parser_destroy(manager->parser);
        }
        free(manager);
    }
}

bool config_manager_load_file(ConfigManager* manager, const char* path, ConfigSource source) {
    if (!manager || !path) {
        return false;
    }
    
    // Check if file exists
    if (access(path, R_OK) != 0) {
        log_debug("Configuration file not found or not readable: %s", path);
        return false;
    }
    
    log_info("Loading configuration from: %s", path);
    
    // Parse the file
    if (!config_parser_parse_file(manager->parser, path, &manager->config)) {
        const ConfigParseError* error = config_parser_get_error(manager->parser);
        log_error("Failed to parse configuration file %s: %s", path, error->message);
        return false;
    }
    
    // Track modification time
    switch (source) {
        case CONFIG_SOURCE_SYSTEM:
            manager->system_config_mtime = get_file_mtime(path);
            break;
        case CONFIG_SOURCE_USER:
            manager->user_config_mtime = get_file_mtime(path);
            break;
        case CONFIG_SOURCE_LOCAL:
            manager->local_config_mtime = get_file_mtime(path);
            break;
        default:
            break;
    }
    
    return true;
}

bool config_manager_load(ConfigManager* manager) {
    if (!manager) {
        return false;
    }
    
    bool any_loaded = false;
    
    // Load in order of precedence (lowest to highest)
    
    // 1. System configuration
    if (!manager->options.skip_system_config && manager->options.system_config_path) {
        if (config_manager_load_file(manager, manager->options.system_config_path, CONFIG_SOURCE_SYSTEM)) {
            any_loaded = true;
        }
    }
    
    // 2. User configuration
    if (!manager->options.skip_user_config && manager->options.user_config_path) {
        if (config_manager_load_file(manager, manager->options.user_config_path, CONFIG_SOURCE_USER)) {
            any_loaded = true;
        }
    }
    
    // 3. Local configuration
    if (!manager->options.skip_local_config && manager->options.local_config_path) {
        if (config_manager_load_file(manager, manager->options.local_config_path, CONFIG_SOURCE_LOCAL)) {
            any_loaded = true;
        }
    }
    
    // If no configuration files were loaded, we still have defaults
    if (!any_loaded) {
        log_warn("No configuration files found, using defaults only");
    }
    
    return true;
}

bool config_manager_apply_override(ConfigManager* manager, const char* key, const char* value) {
    if (!manager || !key || !value) {
        return false;
    }
    
    // Parse the key path and apply the value
    // This is a simplified implementation - could be expanded
    
    if (strcmp(key, "display.width") == 0) {
        manager->config.display.width = atoi(value);
    }
    else if (strcmp(key, "display.height") == 0) {
        manager->config.display.height = atoi(value);
    }
    else if (strcmp(key, "display.fullscreen") == 0) {
        parse_bool(value, &manager->config.display.fullscreen);
    }
    else if (strcmp(key, "system.debug_overlay") == 0) {
        parse_bool(value, &manager->config.system.debug_overlay);
    }
    else if (strcmp(key, "logging.level") == 0) {
        strncpy(manager->config.logging.level, value, CONFIG_MAX_STRING - 1);
    }
    else {
        log_warn("Unknown configuration override key: %s", key);
        return false;
    }
    
    log_info("Applied configuration override: %s = %s", key, value);
    return true;
}

const Config* config_manager_get_config(const ConfigManager* manager) {
    return manager ? &manager->config : NULL;
}

bool config_manager_has_changed(const ConfigManager* manager) {
    if (!manager) {
        return false;
    }
    
    // Check each configuration file for changes
    if (!manager->options.skip_system_config && manager->options.system_config_path) {
        time_t current_mtime = get_file_mtime(manager->options.system_config_path);
        if (current_mtime != manager->system_config_mtime) {
            return true;
        }
    }
    
    if (!manager->options.skip_user_config && manager->options.user_config_path) {
        time_t current_mtime = get_file_mtime(manager->options.user_config_path);
        if (current_mtime != manager->user_config_mtime) {
            return true;
        }
    }
    
    if (!manager->options.skip_local_config && manager->options.local_config_path) {
        time_t current_mtime = get_file_mtime(manager->options.local_config_path);
        if (current_mtime != manager->local_config_mtime) {
            return true;
        }
    }
    
    return false;
}

void config_log_summary(const ConfigManager* manager) {
    if (!manager) {
        return;
    }
    
    const Config* cfg = &manager->config;
    
    log_info("=== Configuration Summary ===");
    log_info("Display: %dx%d, fullscreen=%s, vsync=%s, backend=%s",
             cfg->display.width, cfg->display.height,
             cfg->display.fullscreen ? "yes" : "no",
             cfg->display.vsync ? "yes" : "no",
             cfg->display.backend);
    
    log_info("Input: source=%s, device=%s, mouse_emulation=%s",
             cfg->input.source,
             cfg->input.device_path,
             cfg->input.mouse_emulation ? "yes" : "no");
    
    log_info("API: default_timeout=%dms, verify_ssl=%s, num_services=%zu",
             cfg->api.default_timeout_ms,
             cfg->api.default_verify_ssl ? "yes" : "no",
             cfg->api.num_services);
    
    log_info("UI: fonts=%d/%d/%d, animations=%s, colors: bg=%s primary=%s",
             cfg->ui.fonts.regular_size,
             cfg->ui.fonts.large_size,
             cfg->ui.fonts.small_size,
             cfg->ui.animations.enabled ? "yes" : "no",
             cfg->ui.colors.background,
             cfg->ui.colors.primary);
    
    log_info("Logging: level=%s, file=%s, console=%s",
             cfg->logging.level,
             cfg->logging.file,
             cfg->logging.console ? "yes" : "no");
    
    log_info("System: debug_overlay=%s, allow_exit=%s, startup_page=%d",
             cfg->system.debug_overlay ? "yes" : "no",
             cfg->system.allow_exit ? "yes" : "no",
             cfg->system.startup_page);
    
    if (cfg->loaded_from[0]) {
        log_info("Loaded from: %s", cfg->loaded_from);
    }
    
    log_info("===========================");
}

// Validation function
ValidationResult config_validate_file(const char* path) {
    ValidationResult result = {0};
    
    if (!path) {
        result.valid = false;
        strncpy(result.error_message, "No path provided", sizeof(result.error_message) - 1);
        return result;
    }
    
    // Create temporary parser
    ConfigParser* parser = config_parser_create();
    if (!parser) {
        result.valid = false;
        strncpy(result.error_message, "Failed to create parser", sizeof(result.error_message) - 1);
        return result;
    }
    
    // Read file
    FILE* file = fopen(path, "rb");
    if (!file) {
        result.valid = false;
        snprintf(result.error_message, sizeof(result.error_message), 
                 "Failed to open file: %s", path);
        config_parser_destroy(parser);
        return result;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0 || file_size > 1024 * 1024) {
        result.valid = false;
        strncpy(result.error_message, "Invalid file size", sizeof(result.error_message) - 1);
        fclose(file);
        config_parser_destroy(parser);
        return result;
    }
    
    char* content = malloc(file_size + 1);
    if (!content) {
        result.valid = false;
        strncpy(result.error_message, "Memory allocation failed", sizeof(result.error_message) - 1);
        fclose(file);
        config_parser_destroy(parser);
        return result;
    }
    
    fread(content, 1, file_size, file);
    fclose(file);
    content[file_size] = '\0';
    
    // Validate
    result.valid = config_parser_validate(parser, content, file_size);
    if (!result.valid) {
        const ConfigParseError* error = config_parser_get_error(parser);
        strncpy(result.error_message, error->message, sizeof(result.error_message) - 1);
        result.error_line = error->line;
        result.error_column = error->column;
    }
    
    free(content);
    config_parser_destroy(parser);
    
    return result;
}

// Config generation
bool config_generate_default(const char* path, bool include_comments) {
    if (!path) {
        return false;
    }
    
    FILE* file = fopen(path, "w");
    if (!file) {
        log_error("Failed to create configuration file: %s", path);
        return false;
    }
    
    // Write YAML header
    if (include_comments) {
        fprintf(file, "# PanelKit Configuration File\n");
        fprintf(file, "# Generated on: %s\n", __DATE__);
        fprintf(file, "# Version: 1.0\n\n");
    }
    
    // Display section
    if (include_comments) {
        fprintf(file, "# Display configuration\n");
    }
    fprintf(file, "display:\n");
    fprintf(file, "  width: %d\n", DEFAULT_DISPLAY_WIDTH);
    fprintf(file, "  height: %d\n", DEFAULT_DISPLAY_HEIGHT);
    fprintf(file, "  fullscreen: %s\n", DEFAULT_DISPLAY_FULLSCREEN ? "true" : "false");
    fprintf(file, "  vsync: %s\n", DEFAULT_DISPLAY_VSYNC ? "true" : "false");
    fprintf(file, "  backend: \"%s\"  # Options: auto, sdl, sdl_drm\n\n", DEFAULT_DISPLAY_BACKEND);
    
    // Input section
    if (include_comments) {
        fprintf(file, "# Input configuration\n");
    }
    fprintf(file, "input:\n");
    fprintf(file, "  source: \"%s\"  # Options: auto, sdl_native, evdev\n", DEFAULT_INPUT_SOURCE);
    fprintf(file, "  device_path: \"%s\"  # Device path or \"auto\"\n", DEFAULT_INPUT_DEVICE_PATH);
    fprintf(file, "  mouse_emulation: %s\n", DEFAULT_INPUT_MOUSE_EMULATION ? "true" : "false");
    fprintf(file, "  auto_detect_devices: %s\n\n", DEFAULT_INPUT_AUTO_DETECT ? "true" : "false");
    
    // API section
    if (include_comments) {
        fprintf(file, "# API configuration\n");
        fprintf(file, "# Multiple API services can be defined, each with multiple endpoints\n");
    }
    fprintf(file, "api:\n");
    
    // Default settings
    fprintf(file, "  default_timeout_ms: %d\n", DEFAULT_API_TIMEOUT_MS);
    fprintf(file, "  default_retry_count: %d\n", DEFAULT_API_RETRY_COUNT);
    fprintf(file, "  default_retry_delay_ms: %d\n", DEFAULT_API_RETRY_DELAY_MS);
    fprintf(file, "  default_verify_ssl: %s\n", DEFAULT_API_VERIFY_SSL ? "true" : "false");
    fprintf(file, "  default_user_agent: \"%s\"\n\n", DEFAULT_API_USER_AGENT);
    
    // Services
    fprintf(file, "  services:\n");
    
    // RandomUser API service
    fprintf(file, "    - id: \"randomuser\"\n");
    fprintf(file, "      name: \"Random User Generator\"\n");
    fprintf(file, "      host: \"randomuser.me\"\n");
    fprintf(file, "      protocol: \"https\"\n");
    fprintf(file, "      base_path: \"/api\"\n");
    fprintf(file, "      verify_ssl: true\n");
    if (include_comments) {
        fprintf(file, "      # headers: '{\"X-Custom-Header\": \"value\", \"Accept-Language\": \"en-US\"}'\n");
        fprintf(file, "      # meta: '{\"rate_limit_per_hour\": 1000, \"quota_info\": \"free-tier\"}'\n");
    }
    fprintf(file, "      endpoints:\n");
    fprintf(file, "        - id: \"get_user\"\n");
    fprintf(file, "          name: \"Get Random User\"\n");
    fprintf(file, "          path: \"/\"\n");
    fprintf(file, "          method: \"GET\"\n");
    fprintf(file, "          auto_refresh: true\n");
    fprintf(file, "          refresh_interval_ms: 30000\n");
    fprintf(file, "          optional_params: '{\"results\": \"1\", \"gender\": \"female\"}'\n\n");
    
    // Weather API service (example for N+1 testing)
    fprintf(file, "    - id: \"weather\"\n");
    fprintf(file, "      name: \"OpenWeatherMap API\"\n");
    fprintf(file, "      host: \"api.openweathermap.org\"\n");
    fprintf(file, "      protocol: \"https\"\n");
    fprintf(file, "      base_path: \"/data/2.5\"\n");
    fprintf(file, "      api_key: \"your-api-key-here\"  # Replace with actual API key\n");
    if (include_comments) {
        fprintf(file, "      # Custom headers for this API\n");
    }
    fprintf(file, "      headers: '{\"Accept\": \"application/json\", \"X-Requested-With\": \"PanelKit\"}'\n");
    if (include_comments) {
        fprintf(file, "      # Meta section for service-specific configuration\n");
    }
    fprintf(file, "      meta: '{\"subscription_type\": \"free\", \"calls_per_minute\": 60, \"city_default\": \"San Francisco,US\"}'\n");
    fprintf(file, "      endpoints:\n");
    fprintf(file, "        - id: \"current_weather\"\n");
    fprintf(file, "          name: \"Current Weather\"\n");
    fprintf(file, "          path: \"/weather\"\n");
    fprintf(file, "          method: \"GET\"\n");
    fprintf(file, "          auto_refresh: true\n");
    fprintf(file, "          refresh_interval_ms: 300000  # 5 minutes\n");
    fprintf(file, "          required_params: '{\"q\": \"San Francisco,US\", \"units\": \"metric\"}'\n");
    fprintf(file, "        - id: \"forecast\"\n");
    fprintf(file, "          name: \"5-Day Forecast\"\n");
    fprintf(file, "          path: \"/forecast\"\n");
    fprintf(file, "          method: \"GET\"\n");
    fprintf(file, "          required_params: '{\"q\": \"San Francisco,US\", \"units\": \"metric\"}'\n\n");
    
    // UI section
    if (include_comments) {
        fprintf(file, "# User Interface configuration\n");
    }
    fprintf(file, "ui:\n");
    
    // Colors subsection
    fprintf(file, "  colors:\n");
    fprintf(file, "    background: \"%s\"\n", DEFAULT_COLOR_BACKGROUND);
    fprintf(file, "    primary: \"%s\"\n", DEFAULT_COLOR_PRIMARY);
    fprintf(file, "    secondary: \"%s\"\n", DEFAULT_COLOR_SECONDARY);
    fprintf(file, "    accent: \"%s\"\n", DEFAULT_COLOR_ACCENT);
    fprintf(file, "    error: \"%s\"\n", DEFAULT_COLOR_ERROR);
    fprintf(file, "    warning: \"%s\"\n", DEFAULT_COLOR_WARNING);
    fprintf(file, "    success: \"%s\"\n", DEFAULT_COLOR_SUCCESS);
    
    // Fonts subsection
    fprintf(file, "  \n  fonts:\n");
    fprintf(file, "    regular_size: %d\n", DEFAULT_FONT_REGULAR_SIZE);
    fprintf(file, "    large_size: %d\n", DEFAULT_FONT_LARGE_SIZE);
    fprintf(file, "    small_size: %d\n", DEFAULT_FONT_SMALL_SIZE);
    fprintf(file, "    family: \"%s\"  # \"default\" uses embedded font\n", DEFAULT_FONT_FAMILY);
    
    // Animations subsection
    fprintf(file, "  \n  animations:\n");
    fprintf(file, "    enabled: %s\n", DEFAULT_ANIMATION_ENABLED ? "true" : "false");
    fprintf(file, "    page_transition_ms: %d\n", DEFAULT_ANIMATION_PAGE_TRANSITION_MS);
    fprintf(file, "    scroll_friction: %.2f\n", DEFAULT_ANIMATION_SCROLL_FRICTION);
    fprintf(file, "    button_press_scale: %.2f\n", DEFAULT_ANIMATION_BUTTON_PRESS_SCALE);
    
    // Layout subsection
    fprintf(file, "  \n  layout:\n");
    fprintf(file, "    button_padding: %d\n", DEFAULT_LAYOUT_BUTTON_PADDING);
    fprintf(file, "    header_height: %d\n", DEFAULT_LAYOUT_HEADER_HEIGHT);
    fprintf(file, "    margin: %d\n", DEFAULT_LAYOUT_MARGIN);
    fprintf(file, "    scroll_threshold: %d\n", DEFAULT_LAYOUT_SCROLL_THRESHOLD);
    fprintf(file, "    swipe_threshold: %d\n\n", DEFAULT_LAYOUT_SWIPE_THRESHOLD);
    
    // Logging section
    if (include_comments) {
        fprintf(file, "# Logging configuration\n");
    }
    fprintf(file, "logging:\n");
    fprintf(file, "  level: \"%s\"  # Options: debug, info, warn, error\n", DEFAULT_LOG_LEVEL);
    fprintf(file, "  file: \"%s\"\n", DEFAULT_LOG_FILE);
    fprintf(file, "  max_size: %llu  # bytes\n", (unsigned long long)DEFAULT_LOG_MAX_SIZE);
    fprintf(file, "  max_files: %d\n", DEFAULT_LOG_MAX_FILES);
    fprintf(file, "  console: %s\n\n", DEFAULT_LOG_CONSOLE ? "true" : "false");
    
    // System section
    if (include_comments) {
        fprintf(file, "# System configuration\n");
    }
    fprintf(file, "system:\n");
    fprintf(file, "  startup_page: %d\n", DEFAULT_SYSTEM_STARTUP_PAGE);
    fprintf(file, "  debug_overlay: %s\n", DEFAULT_SYSTEM_DEBUG_OVERLAY ? "true" : "false");
    fprintf(file, "  allow_exit: %s\n", DEFAULT_SYSTEM_ALLOW_EXIT ? "true" : "false");
    fprintf(file, "  idle_timeout: %d  # seconds, 0 = disabled\n", DEFAULT_SYSTEM_IDLE_TIMEOUT);
    fprintf(file, "  config_check_interval: %d  # seconds, 0 = disabled\n", DEFAULT_SYSTEM_CONFIG_CHECK_INTERVAL);
    
    fclose(file);
    
    log_info("Generated default configuration file: %s", path);
    return true;
}

