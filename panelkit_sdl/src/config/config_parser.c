#include "config_parser.h"
#include "config_defaults.h"
#include "config_utils.h"
#include "../yaml/yaml.h"
#include "../core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

struct ConfigParser {
    ConfigParseError error;
    ConfigParserWarningCallback warning_callback;
    void* warning_user_data;
    RequiredFields required_fields;
};

// Helper structure for tracking parsing context
typedef struct {
    ConfigParser* parser;
    Config* config;
    yaml_parser_t* yaml_parser;
    char current_section[128];
    char current_key[128];
    int depth;
} ParseContext;

ConfigParser* config_parser_create(void) {
    ConfigParser* parser = calloc(1, sizeof(ConfigParser));
    if (!parser) {
        return NULL;
    }
    
    return parser;
}

void config_parser_destroy(ConfigParser* parser) {
    if (parser) {
        free(parser);
    }
}

void config_parser_clear_error(ConfigParser* parser) {
    if (parser) {
        memset(&parser->error, 0, sizeof(ConfigParseError));
    }
}

const ConfigParseError* config_parser_get_error(const ConfigParser* parser) {
    return parser ? &parser->error : NULL;
}

void config_parser_set_warning_callback(ConfigParser* parser, 
                                        ConfigParserWarningCallback callback, 
                                        void* user_data) {
    if (parser) {
        parser->warning_callback = callback;
        parser->warning_user_data = user_data;
    }
}

static void set_parse_error(ConfigParser* parser, const char* format, ...) {
    parser->error.has_error = true;
    
    va_list args;
    va_start(args, format);
    vsnprintf(parser->error.message, sizeof(parser->error.message), format, args);
    va_end(args);
}

static void emit_warning(ParseContext* ctx, const char* format, ...) {
    if (!ctx->parser->warning_callback) {
        return;
    }
    
    char message[256];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    ctx->parser->warning_callback(message, 0, 0, ctx->parser->warning_user_data);
}

// Color validation helper
static bool is_valid_color(const char* color) {
    if (!color || strlen(color) != 7) return false;
    if (color[0] != '#') return false;
    
    for (int i = 1; i < 7; i++) {
        if (!isxdigit(color[i])) return false;
    }
    
    return true;
}


// Process a key-value pair based on current section
static void process_key_value(ParseContext* ctx, const char* key, const char* value) {
    char path[256];
    snprintf(path, sizeof(path), "%s%s%s", 
             ctx->current_section, 
             ctx->current_section[0] ? "." : "", 
             key);
    
    // Display section
    if (strncmp(path, "display.", 8) == 0) {
        const char* subkey = path + 8;
        
        if (strcmp(subkey, "width") == 0) {
            ctx->config->display.width = atoi(value);
            ctx->parser->required_fields.display_width = true;
        }
        else if (strcmp(subkey, "height") == 0) {
            ctx->config->display.height = atoi(value);
            ctx->parser->required_fields.display_height = true;
        }
        else if (strcmp(subkey, "fullscreen") == 0) {
            parse_bool(value, &ctx->config->display.fullscreen);
        }
        else if (strcmp(subkey, "vsync") == 0) {
            parse_bool(value, &ctx->config->display.vsync);
        }
        else if (strcmp(subkey, "backend") == 0) {
            strncpy(ctx->config->display.backend, value, CONFIG_MAX_STRING - 1);
        }
        else {
            emit_warning(ctx, "Unknown display configuration key: %s", subkey);
        }
    }
    // Input section
    else if (strncmp(path, "input.", 6) == 0) {
        const char* subkey = path + 6;
        
        if (strcmp(subkey, "source") == 0) {
            strncpy(ctx->config->input.source, value, CONFIG_MAX_STRING - 1);
        }
        else if (strcmp(subkey, "device_path") == 0) {
            strncpy(ctx->config->input.device_path, value, CONFIG_MAX_PATH - 1);
        }
        else if (strcmp(subkey, "mouse_emulation") == 0) {
            parse_bool(value, &ctx->config->input.mouse_emulation);
        }
        else if (strcmp(subkey, "auto_detect_devices") == 0) {
            parse_bool(value, &ctx->config->input.auto_detect_devices);
        }
        else {
            emit_warning(ctx, "Unknown input configuration key: %s", subkey);
        }
    }
    // API section
    else if (strncmp(path, "api.", 4) == 0) {
        const char* subkey = path + 4;
        
        if (strcmp(subkey, "default_timeout_ms") == 0) {
            ctx->config->api.default_timeout_ms = atoi(value);
        }
        else if (strcmp(subkey, "default_retry_count") == 0) {
            ctx->config->api.default_retry_count = atoi(value);
        }
        else if (strcmp(subkey, "default_retry_delay_ms") == 0) {
            ctx->config->api.default_retry_delay_ms = atoi(value);
        }
        else if (strcmp(subkey, "default_verify_ssl") == 0) {
            parse_bool(value, &ctx->config->api.default_verify_ssl);
        }
        else if (strcmp(subkey, "default_user_agent") == 0) {
            strncpy(ctx->config->api.default_user_agent, value, CONFIG_MAX_STRING - 1);
        }
        else if (strncmp(subkey, "services", 8) == 0) {
            // TODO: Parse nested services structure including headers and meta
            emit_warning(ctx, "Nested API services parsing not yet implemented (includes headers/meta): %s", subkey);
        }
        else {
            emit_warning(ctx, "Unknown API configuration key: %s", subkey);
        }
    }
    // UI Colors section
    else if (strncmp(path, "ui.colors.", 10) == 0) {
        const char* subkey = path + 10;
        
        if (strcmp(subkey, "background") == 0) {
            if (is_valid_color(value)) {
                strncpy(ctx->config->ui.colors.background, value, CONFIG_MAX_COLOR - 1);
            } else {
                emit_warning(ctx, "Invalid color format for ui.colors.background: %s", value);
            }
        }
        else if (strcmp(subkey, "primary") == 0) {
            if (is_valid_color(value)) {
                strncpy(ctx->config->ui.colors.primary, value, CONFIG_MAX_COLOR - 1);
            }
        }
        else if (strcmp(subkey, "secondary") == 0) {
            if (is_valid_color(value)) {
                strncpy(ctx->config->ui.colors.secondary, value, CONFIG_MAX_COLOR - 1);
            }
        }
        else if (strcmp(subkey, "accent") == 0) {
            if (is_valid_color(value)) {
                strncpy(ctx->config->ui.colors.accent, value, CONFIG_MAX_COLOR - 1);
            }
        }
        else if (strcmp(subkey, "error") == 0) {
            if (is_valid_color(value)) {
                strncpy(ctx->config->ui.colors.error, value, CONFIG_MAX_COLOR - 1);
            }
        }
        else if (strcmp(subkey, "warning") == 0) {
            if (is_valid_color(value)) {
                strncpy(ctx->config->ui.colors.warning, value, CONFIG_MAX_COLOR - 1);
            }
        }
        else if (strcmp(subkey, "success") == 0) {
            if (is_valid_color(value)) {
                strncpy(ctx->config->ui.colors.success, value, CONFIG_MAX_COLOR - 1);
            }
        }
        else {
            emit_warning(ctx, "Unknown UI color configuration key: %s", subkey);
        }
    }
    // UI Fonts section
    else if (strncmp(path, "ui.fonts.", 9) == 0) {
        const char* subkey = path + 9;
        
        if (strcmp(subkey, "regular_size") == 0) {
            ctx->config->ui.fonts.regular_size = atoi(value);
        }
        else if (strcmp(subkey, "large_size") == 0) {
            ctx->config->ui.fonts.large_size = atoi(value);
        }
        else if (strcmp(subkey, "small_size") == 0) {
            ctx->config->ui.fonts.small_size = atoi(value);
        }
        else if (strcmp(subkey, "family") == 0) {
            strncpy(ctx->config->ui.fonts.family, value, CONFIG_MAX_STRING - 1);
        }
        else {
            emit_warning(ctx, "Unknown UI font configuration key: %s", subkey);
        }
    }
    // UI Animations section
    else if (strncmp(path, "ui.animations.", 14) == 0) {
        const char* subkey = path + 14;
        
        if (strcmp(subkey, "enabled") == 0) {
            parse_bool(value, &ctx->config->ui.animations.enabled);
        }
        else if (strcmp(subkey, "page_transition_ms") == 0) {
            ctx->config->ui.animations.page_transition_ms = atoi(value);
        }
        else if (strcmp(subkey, "scroll_friction") == 0) {
            ctx->config->ui.animations.scroll_friction = atof(value);
        }
        else if (strcmp(subkey, "button_press_scale") == 0) {
            ctx->config->ui.animations.button_press_scale = atof(value);
        }
        else {
            emit_warning(ctx, "Unknown UI animation configuration key: %s", subkey);
        }
    }
    // UI Layout section
    else if (strncmp(path, "ui.layout.", 10) == 0) {
        const char* subkey = path + 10;
        
        if (strcmp(subkey, "button_padding") == 0) {
            ctx->config->ui.layout.button_padding = atoi(value);
        }
        else if (strcmp(subkey, "header_height") == 0) {
            ctx->config->ui.layout.header_height = atoi(value);
        }
        else if (strcmp(subkey, "margin") == 0) {
            ctx->config->ui.layout.margin = atoi(value);
        }
        else if (strcmp(subkey, "scroll_threshold") == 0) {
            ctx->config->ui.layout.scroll_threshold = atoi(value);
        }
        else if (strcmp(subkey, "swipe_threshold") == 0) {
            ctx->config->ui.layout.swipe_threshold = atoi(value);
        }
        else {
            emit_warning(ctx, "Unknown UI layout configuration key: %s", subkey);
        }
    }
    // Logging section
    else if (strncmp(path, "logging.", 8) == 0) {
        const char* subkey = path + 8;
        
        if (strcmp(subkey, "level") == 0) {
            strncpy(ctx->config->logging.level, value, CONFIG_MAX_STRING - 1);
        }
        else if (strcmp(subkey, "file") == 0) {
            strncpy(ctx->config->logging.file, value, CONFIG_MAX_PATH - 1);
            ctx->parser->required_fields.logging_file = true;
        }
        else if (strcmp(subkey, "max_size") == 0) {
            ctx->config->logging.max_size = strtoull(value, NULL, 10);
        }
        else if (strcmp(subkey, "max_files") == 0) {
            ctx->config->logging.max_files = atoi(value);
        }
        else if (strcmp(subkey, "console") == 0) {
            parse_bool(value, &ctx->config->logging.console);
        }
        else {
            emit_warning(ctx, "Unknown logging configuration key: %s", subkey);
        }
    }
    // System section
    else if (strncmp(path, "system.", 7) == 0) {
        const char* subkey = path + 7;
        
        if (strcmp(subkey, "startup_page") == 0) {
            ctx->config->system.startup_page = atoi(value);
        }
        else if (strcmp(subkey, "debug_overlay") == 0) {
            parse_bool(value, &ctx->config->system.debug_overlay);
        }
        else if (strcmp(subkey, "allow_exit") == 0) {
            parse_bool(value, &ctx->config->system.allow_exit);
        }
        else if (strcmp(subkey, "idle_timeout") == 0) {
            ctx->config->system.idle_timeout = atoi(value);
        }
        else if (strcmp(subkey, "config_check_interval") == 0) {
            ctx->config->system.config_check_interval = atoi(value);
        }
        else {
            emit_warning(ctx, "Unknown system configuration key: %s", subkey);
        }
    }
    // Root level or unknown section
    else {
        emit_warning(ctx, "Unknown configuration key: %s", path);
    }
}

// Process YAML events
static bool process_yaml_stream(ParseContext* ctx) {
    yaml_event_t event;
    bool done = false;
    bool in_mapping = false;
    char current_key[128] = {0};
    
    while (!done) {
        if (!yaml_parser_parse(ctx->yaml_parser, &event)) {
            set_parse_error(ctx->parser, "YAML parse error: %s", 
                           ctx->yaml_parser->problem ? ctx->yaml_parser->problem : "unknown");
            yaml_event_delete(&event);
            return false;
        }
        
        switch (event.type) {
            case YAML_STREAM_END_EVENT:
                done = true;
                break;
                
            case YAML_MAPPING_START_EVENT:
                in_mapping = true;
                if (current_key[0] && ctx->depth == 0) {
                    // Entering a section
                    strncpy(ctx->current_section, current_key, sizeof(ctx->current_section) - 1);
                } else if (current_key[0] && ctx->depth == 1) {
                    // Entering a subsection
                    char temp[128];
                    snprintf(temp, sizeof(temp), "%s.%s", ctx->current_section, current_key);
                    strncpy(ctx->current_section, temp, sizeof(ctx->current_section) - 1);
                }
                ctx->depth++;
                break;
                
            case YAML_MAPPING_END_EVENT:
                ctx->depth--;
                if (ctx->depth == 0) {
                    ctx->current_section[0] = '\0';
                } else if (ctx->depth == 1) {
                    // Back to parent section
                    char* dot = strrchr(ctx->current_section, '.');
                    if (dot) *dot = '\0';
                }
                break;
                
            case YAML_SCALAR_EVENT:
                if (in_mapping && current_key[0] == '\0') {
                    // This is a key
                    strncpy(current_key, (char*)event.data.scalar.value, sizeof(current_key) - 1);
                } else if (in_mapping && current_key[0] != '\0') {
                    // This is a value
                    process_key_value(ctx, current_key, (char*)event.data.scalar.value);
                    current_key[0] = '\0';
                }
                break;
                
            default:
                break;
        }
        
        yaml_event_delete(&event);
    }
    
    return true;
}

bool config_parser_parse(ConfigParser* parser, const char* yaml_content, size_t length, Config* config) {
    if (!parser || !yaml_content || !config) {
        return false;
    }
    
    // Clear any previous error
    config_parser_clear_error(parser);
    memset(&parser->required_fields, 0, sizeof(RequiredFields));
    
    // Initialize config with defaults
    config_init_defaults(config);
    
    // Initialize YAML parser
    yaml_parser_t yaml_parser;
    if (!yaml_parser_initialize(&yaml_parser)) {
        set_parse_error(parser, "Failed to initialize YAML parser");
        return false;
    }
    
    // Set input
    yaml_parser_set_input_string(&yaml_parser, (const unsigned char*)yaml_content, length);
    
    // Create parse context
    ParseContext ctx = {
        .parser = parser,
        .config = config,
        .yaml_parser = &yaml_parser,
        .depth = 0
    };
    
    // Process the YAML stream
    bool success = process_yaml_stream(&ctx);
    
    // Clean up
    yaml_parser_delete(&yaml_parser);
    
    // Validate required fields
    if (success) {
        // All fields are optional - defaults will be used if not specified
        // This allows for minimal configuration files
    }
    
    return success;
}

bool config_parser_parse_file(ConfigParser* parser, const char* filename, Config* config) {
    if (!parser || !filename || !config) {
        return false;
    }
    
    // Open file
    FILE* file = fopen(filename, "rb");
    if (!file) {
        set_parse_error(parser, "Failed to open configuration file: %s", filename);
        return false;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0 || file_size > 1024 * 1024) { // Max 1MB config file
        set_parse_error(parser, "Invalid configuration file size");
        fclose(file);
        return false;
    }
    
    // Read file content
    char* content = malloc(file_size + 1);
    if (!content) {
        set_parse_error(parser, "Failed to allocate memory for configuration file");
        fclose(file);
        return false;
    }
    
    size_t read_size = fread(content, 1, file_size, file);
    fclose(file);
    
    if (read_size != (size_t)file_size) {
        set_parse_error(parser, "Failed to read configuration file");
        free(content);
        return false;
    }
    
    content[file_size] = '\0';
    
    // Parse content
    bool success = config_parser_parse(parser, content, file_size, config);
    
    // Store the filename if successful
    if (success) {
        strncpy(config->loaded_from, filename, CONFIG_MAX_PATH - 1);
        config->loaded_from[CONFIG_MAX_PATH - 1] = '\0';
    }
    
    free(content);
    return success;
}

bool config_parser_validate(ConfigParser* parser, const char* yaml_content, size_t length) {
    if (!parser || !yaml_content) {
        return false;
    }
    
    // Clear any previous error
    config_parser_clear_error(parser);
    
    // Initialize YAML parser
    yaml_parser_t yaml_parser;
    if (!yaml_parser_initialize(&yaml_parser)) {
        set_parse_error(parser, "Failed to initialize YAML parser");
        return false;
    }
    
    // Set input
    yaml_parser_set_input_string(&yaml_parser, (const unsigned char*)yaml_content, length);
    
    // Just scan through the document to check syntax
    yaml_event_t event;
    bool valid = true;
    bool done = false;
    
    while (!done && valid) {
        if (!yaml_parser_parse(&yaml_parser, &event)) {
            set_parse_error(parser, "YAML syntax error at line %zu: %s", 
                           yaml_parser.problem_mark.line + 1,
                           yaml_parser.problem ? yaml_parser.problem : "unknown error");
            parser->error.line = yaml_parser.problem_mark.line + 1;
            parser->error.column = yaml_parser.problem_mark.column + 1;
            valid = false;
        }
        
        if (event.type == YAML_STREAM_END_EVENT) {
            done = true;
        }
        
        yaml_event_delete(&event);
    }
    
    yaml_parser_delete(&yaml_parser);
    return valid;
}