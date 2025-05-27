/**
 * @file config_parser.h
 * @brief YAML configuration file parser
 * 
 * Parses YAML configuration files into the application's config structure.
 */

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include "config_schema.h"
#include <stdbool.h>

// Forward declaration
typedef struct ConfigParser ConfigParser;

// Parser error information
typedef struct {
    bool has_error;
    char message[256];
    int line;
    int column;
    char context[128];  // Line content around error
} ConfigParseError;

// Create a new configuration parser
ConfigParser* config_parser_create(void);

// Destroy the parser
void config_parser_destroy(ConfigParser* parser);

// Parse configuration from a YAML string
bool config_parser_parse(ConfigParser* parser, const char* yaml_content, size_t length, Config* config);

// Parse configuration from a file
bool config_parser_parse_file(ConfigParser* parser, const char* filename, Config* config);

// Get the last parse error (if any)
const ConfigParseError* config_parser_get_error(const ConfigParser* parser);

// Clear any existing parse error
void config_parser_clear_error(ConfigParser* parser);

// Get warnings from parsing (unknown keys, etc.)
typedef void (*ConfigParserWarningCallback)(const char* message, int line, int column, void* user_data);
void config_parser_set_warning_callback(ConfigParser* parser, ConfigParserWarningCallback callback, void* user_data);

// Validate without fully parsing (syntax check)
bool config_parser_validate(ConfigParser* parser, const char* yaml_content, size_t length);

#endif // CONFIG_PARSER_H