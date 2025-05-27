/**
 * @file api_parsers.h
 * @brief JSON parsing utilities for API responses
 * 
 * Provides type-safe parsing of API responses into domain structures.
 */

#ifndef API_PARSERS_H
#define API_PARSERS_H

#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct ApiServiceConfig ApiServiceConfig;
typedef struct ApiEndpointConfig ApiEndpointConfig;
typedef struct UserData UserData;

// API parser function signature
// Returns true on successful parse, false on error
typedef bool (*api_parser_func)(const char* response_data, size_t data_len, 
                                const ApiServiceConfig* service, 
                                const ApiEndpointConfig* endpoint, 
                                UserData* output);

// Parser registry entry
typedef struct {
    const char* service_id;          // Must match ApiServiceConfig.id
    const char* endpoint_id;         // Must match ApiEndpointConfig.id
    const char* parser_name;         // Human-readable parser name
    api_parser_func parse_func;      // Parser function
} ApiParserEntry;

// Parser registry functions
bool api_parsers_init(void);
void api_parsers_cleanup(void);
bool api_parsers_register(const char* service_id, const char* endpoint_id, 
                          const char* parser_name, api_parser_func parse_func);
api_parser_func api_parsers_get(const char* service_id, const char* endpoint_id);
const char* api_parsers_get_name(const char* service_id, const char* endpoint_id);
bool api_parsers_is_supported(const char* service_id, const char* endpoint_id);

// Helper functions for accessing service configuration
const char* api_service_get_header(const ApiServiceConfig* service, const char* header_name);
const char* api_service_get_meta(const ApiServiceConfig* service, const char* meta_key);
bool api_service_get_meta_bool(const ApiServiceConfig* service, const char* meta_key, bool default_value);
int api_service_get_meta_int(const ApiServiceConfig* service, const char* meta_key, int default_value);

// Built-in parsers
bool parse_randomuser_get_user(const char* response_data, size_t data_len, 
                              const ApiServiceConfig* service, 
                              const ApiEndpointConfig* endpoint, 
                              UserData* output);

#endif // API_PARSERS_H