/*
 * Example of how to integrate the configuration system into app.c
 * This file demonstrates the integration pattern without making actual changes
 */

#ifndef CONFIG_INTEGRATION_EXAMPLE_H
#define CONFIG_INTEGRATION_EXAMPLE_H

#include "config_manager.h"

/*
 * Example integration in main():
 *
 * int main(int argc, char* argv[]) {
 *     // Initialize logging first
 *     logger_init("panelkit");
 *     
 *     // Create configuration manager
 *     ConfigManagerOptions config_options = {0};
 *     ConfigManager* config_manager = config_manager_create(&config_options);
 *     
 *     // Load configuration from all sources
 *     if (!config_manager_load(config_manager)) {
 *         log_error("Failed to load configuration");
 *         return 1;
 *     }
 *     
 *     // Process command-line arguments for config overrides
 *     for (int i = 1; i < argc; i++) {
 *         if (strcmp(argv[i], "--config-override") == 0 && i + 1 < argc) {
 *             char* key = strtok(argv[i + 1], "=");
 *             char* value = strtok(NULL, "=");
 *             if (key && value) {
 *                 config_manager_apply_override(config_manager, key, value);
 *             }
 *             i++; // Skip the value
 *         }
 *         else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
 *             config_manager_load_file(config_manager, argv[i + 1], CONFIG_SOURCE_CLI);
 *             i++;
 *         }
 *         else if (strcmp(argv[i], "--validate-config") == 0 && i + 1 < argc) {
 *             ValidationResult result = config_validate_file(argv[i + 1]);
 *             if (result.valid) {
 *                 printf("Configuration file is valid\n");
 *                 return 0;
 *             } else {
 *                 printf("Configuration error at line %d: %s\n", 
 *                        result.error_line, result.error_message);
 *                 return 1;
 *             }
 *         }
 *         else if (strcmp(argv[i], "--generate-config") == 0 && i + 1 < argc) {
 *             if (config_generate_default(argv[i + 1], true)) {
 *                 printf("Generated configuration file: %s\n", argv[i + 1]);
 *                 return 0;
 *             } else {
 *                 printf("Failed to generate configuration file\n");
 *                 return 1;
 *             }
 *         }
 *     }
 *     
 *     // Get configuration
 *     const Config* config = config_manager_get_config(config_manager);
 *     config_log_summary(config_manager);
 *     
 *     // Use configuration values instead of hardcoded constants
 *     DisplayConfig display_config = {
 *         .width = config->display.width,
 *         .height = config->display.height,
 *         .title = "PanelKit",
 *         .backend_type = backend_type_from_string(config->display.backend),
 *         .fullscreen = config->display.fullscreen,
 *         .vsync = config->display.vsync
 *     };
 *     
 *     // Initialize input with config
 *     InputConfig input_config = {
 *         .source_type = input_source_from_string(config->input.source),
 *         .device_path = strcmp(config->input.device_path, "auto") == 0 ? NULL : config->input.device_path,
 *         .auto_detect_devices = config->input.auto_detect_devices,
 *         .enable_mouse_emulation = config->input.mouse_emulation
 *     };
 *     
 *     // Use UI configuration
 *     actual_width = config->display.width;
 *     actual_height = config->display.height;
 *     
 *     // Colors from config (would need color parsing)
 *     // bg_color = parse_color(config->ui.colors.background);
 *     
 *     // Font sizes from config
 *     int font_size = config->ui.fonts.regular_size;
 *     int large_font_size = config->ui.fonts.large_size;
 *     int small_font_size = config->ui.fonts.small_size;
 *     
 *     // API configuration
 *     ApiManagerConfig api_config = {
 *         .base_url = config->api.base_url,
 *         .timeout = config->api.timeout,
 *         .auto_refresh = config->api.auto_refresh,
 *         .refresh_interval = config->api.refresh_interval
 *     };
 *     
 *     // System settings
 *     debug_overlay = config->system.debug_overlay;
 *     allow_exit = config->system.allow_exit;
 *     current_page = config->system.startup_page;
 *     
 *     // ... rest of initialization ...
 *     
 *     // In main loop, check for config changes if enabled
 *     if (config->system.config_check_interval > 0) {
 *         static Uint32 last_config_check = 0;
 *         Uint32 now = SDL_GetTicks();
 *         if (now - last_config_check > config->system.config_check_interval * 1000) {
 *             if (config_manager_has_changed(config_manager)) {
 *                 log_info("Configuration file changed, restart required");
 *                 // Could show a notification to user
 *             }
 *             last_config_check = now;
 *         }
 *     }
 *     
 *     // Cleanup
 *     config_manager_destroy(config_manager);
 * }
 */

/*
 * Helper functions that would be needed:
 */

// Convert string to backend type
static inline DisplayBackendType backend_type_from_string(const char* str) {
    if (strcmp(str, "sdl") == 0) return DISPLAY_BACKEND_SDL;
    if (strcmp(str, "sdl_drm") == 0) return DISPLAY_BACKEND_SDL_DRM;
    return DISPLAY_BACKEND_AUTO;
}

// Convert string to input source type  
static inline InputSourceType input_source_from_string(const char* str) {
    if (strcmp(str, "sdl_native") == 0) return INPUT_SOURCE_SDL_NATIVE;
    if (strcmp(str, "evdev") == 0) return INPUT_SOURCE_LINUX_EVDEV;
    return INPUT_SOURCE_SDL_NATIVE;  // Default
}

// Parse color from hex string
static inline SDL_Color parse_color(const char* hex) {
    SDL_Color color = {0, 0, 0, 255};
    if (hex && hex[0] == '#' && strlen(hex) == 7) {
        unsigned int rgb;
        sscanf(hex + 1, "%06x", &rgb);
        color.r = (rgb >> 16) & 0xFF;
        color.g = (rgb >> 8) & 0xFF;
        color.b = rgb & 0xFF;
    }
    return color;
}

#endif // CONFIG_INTEGRATION_EXAMPLE_H