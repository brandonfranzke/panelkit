#include "api_parsers.h"
#include "../json/json_parser.h"
#include "../core/logger.h"
#include <stdlib.h>
#include <string.h>

// Static registry for parsers
static ApiParserEntry* parser_registry = NULL;
static size_t registry_size = 0;
static size_t registry_capacity = 0;

bool api_parsers_init(void) {
    // Initialize with some default capacity
    registry_capacity = 10;
    parser_registry = calloc(registry_capacity, sizeof(ApiParserEntry));
    if (!parser_registry) {
        log_error("Failed to allocate parser registry");
        return false;
    }
    
    // Register built-in parsers
    if (!api_parsers_register("randomuser", "get_user", "RandomUser API Parser", parse_randomuser_get_user)) {
        log_error("Failed to register randomuser parser");
        return false;
    }
    
    log_info("API parsers initialized with %zu entries", registry_size);
    return true;
}

void api_parsers_cleanup(void) {
    if (parser_registry) {
        free(parser_registry);
        parser_registry = NULL;
        registry_size = 0;
        registry_capacity = 0;
    }
}

bool api_parsers_register(const char* service_id, const char* endpoint_id, 
                          const char* parser_name, api_parser_func parse_func) {
    if (!service_id || !endpoint_id || !parser_name || !parse_func) {
        return false;
    }
    
    // Check if we need to expand the registry
    if (registry_size >= registry_capacity) {
        size_t new_capacity = registry_capacity * 2;
        ApiParserEntry* new_registry = realloc(parser_registry, new_capacity * sizeof(ApiParserEntry));
        if (!new_registry) {
            log_error("Failed to expand parser registry");
            return false;
        }
        parser_registry = new_registry;
        registry_capacity = new_capacity;
    }
    
    // Add the new parser
    ApiParserEntry* entry = &parser_registry[registry_size];
    entry->service_id = service_id;  // Assuming these are string literals or managed elsewhere
    entry->endpoint_id = endpoint_id;
    entry->parser_name = parser_name;
    entry->parse_func = parse_func;
    
    registry_size++;
    log_debug("Registered parser for %s:%s - %s", service_id, endpoint_id, parser_name);
    return true;
}

api_parser_func api_parsers_get(const char* service_id, const char* endpoint_id) {
    if (!service_id || !endpoint_id) {
        return NULL;
    }
    
    for (size_t i = 0; i < registry_size; i++) {
        if (strcmp(parser_registry[i].service_id, service_id) == 0 &&
            strcmp(parser_registry[i].endpoint_id, endpoint_id) == 0) {
            return parser_registry[i].parse_func;
        }
    }
    
    return NULL;
}

const char* api_parsers_get_name(const char* service_id, const char* endpoint_id) {
    if (!service_id || !endpoint_id) {
        return NULL;
    }
    
    for (size_t i = 0; i < registry_size; i++) {
        if (strcmp(parser_registry[i].service_id, service_id) == 0 &&
            strcmp(parser_registry[i].endpoint_id, endpoint_id) == 0) {
            return parser_registry[i].parser_name;
        }
    }
    
    return NULL;
}

bool api_parsers_is_supported(const char* service_id, const char* endpoint_id) {
    return api_parsers_get(service_id, endpoint_id) != NULL;
}

// Built-in parser for RandomUser API
bool parse_randomuser_get_user(const char* response_data, size_t data_len, 
                              const ApiServiceConfig* service, 
                              const ApiEndpointConfig* endpoint, 
                              UserData* output) {
    if (!response_data || data_len == 0 || !output) {
        log_error("Invalid parameters for randomuser parser");
        return false;
    }
    
    log_debug("Parsing RandomUser API response (%zu bytes) for service:%s endpoint:%s", 
              data_len, service ? service->id : "unknown", endpoint ? endpoint->id : "unknown");
    
    // For now, just use the existing JSON parser
    // TODO: This should be updated to handle the new API structure
    JsonParser* parser = json_parser_create();
    if (!parser) {
        log_error("Failed to create JSON parser for randomuser");
        return false;
    }
    
    bool success = json_parser_parse_user_data(parser, response_data, data_len, output);
    if (!success) {
        const JsonParseError* error = json_parser_get_error(parser);
        log_error("Failed to parse randomuser response: %s", error ? error->message : "unknown error");
    } else {
        log_info("Successfully parsed randomuser data: %s %s", output->first_name, output->last_name);
    }
    
    json_parser_destroy(parser);
    return success;
}

// Helper functions for accessing service configuration
const char* api_service_get_header(const ApiServiceConfig* service, const char* header_name) {
    if (!service || !header_name || !service->headers[0]) {
        return NULL;
    }
    
    // TODO: Parse JSON headers and extract specific header
    // For now, just log what we're looking for
    log_debug("Looking for header '%s' in service '%s' headers: %s", 
              header_name, service->id, service->headers);
    return NULL;
}

const char* api_service_get_meta(const ApiServiceConfig* service, const char* meta_key) {
    if (!service || !meta_key || !service->meta[0]) {
        return NULL;
    }
    
    // TODO: Parse JSON meta and extract specific key
    // For now, just log what we're looking for
    log_debug("Looking for meta key '%s' in service '%s' meta: %s", 
              meta_key, service->id, service->meta);
    return NULL;
}

bool api_service_get_meta_bool(const ApiServiceConfig* service, const char* meta_key, bool default_value) {
    const char* value = api_service_get_meta(service, meta_key);
    if (!value) {
        return default_value;
    }
    
    // TODO: Parse boolean from JSON value
    return default_value;
}

int api_service_get_meta_int(const ApiServiceConfig* service, const char* meta_key, int default_value) {
    const char* value = api_service_get_meta(service, meta_key);
    if (!value) {
        return default_value;
    }
    
    // TODO: Parse integer from JSON value
    return default_value;
}