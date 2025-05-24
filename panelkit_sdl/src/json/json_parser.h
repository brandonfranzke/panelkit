#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct JsonParser JsonParser;
typedef struct JsonValue JsonValue;

// JSON value types
typedef enum {
    JSON_TYPE_UNDEFINED = 0,
    JSON_TYPE_OBJECT = 1,
    JSON_TYPE_ARRAY = 2,
    JSON_TYPE_STRING = 3,
    JSON_TYPE_PRIMITIVE = 4
} JsonType;

// Error codes
typedef enum {
    JSON_SUCCESS = 0,
    JSON_ERROR_NOMEM = -1,      // Not enough tokens
    JSON_ERROR_INVAL = -2,      // Invalid character inside JSON string
    JSON_ERROR_PART = -3,       // JSON string is not complete
    JSON_ERROR_NOT_FOUND = -4,  // Key not found
    JSON_ERROR_TYPE = -5        // Wrong type
} JsonError;

// Create/destroy parser
JsonParser* json_parser_create(void);
void json_parser_destroy(JsonParser* parser);

// Parse JSON string
JsonError json_parser_parse(JsonParser* parser, const char* json_string, size_t length);

// Get root object
JsonValue* json_parser_get_root(JsonParser* parser);

// Object/Array access
JsonValue* json_object_get(JsonValue* object, const char* key);
JsonValue* json_array_get(JsonValue* array, int index);
int json_array_size(JsonValue* array);

// Value extraction
JsonError json_value_get_string(JsonValue* value, char* buffer, size_t buffer_size);
JsonError json_value_get_int(JsonValue* value, int* result);
JsonError json_value_get_bool(JsonValue* value, bool* result);

// Type checking
JsonType json_value_get_type(JsonValue* value);
bool json_value_is_null(JsonValue* value);

// Utility functions
const char* json_error_string(JsonError error);

#endif // JSON_PARSER_H