#define JSMN_HEADER
#include "jsmn.h"
#include "json_parser.h"
#include "../core/logger.h"
#include <stdlib.h>
#include <string.h>

#define MAX_TOKENS 128

struct JsonValue {
    jsmntok_t* token;
    const char* json_string;
};

struct JsonParser {
    jsmn_parser parser;
    jsmntok_t tokens[MAX_TOKENS];
    int token_count;
    char* json_string;
    JsonValue root_value;
};

JsonParser* json_parser_create(void) {
    JsonParser* parser = calloc(1, sizeof(JsonParser));
    if (!parser) {
        return NULL;
    }
    
    jsmn_init(&parser->parser);
    return parser;
}

void json_parser_destroy(JsonParser* parser) {
    if (parser) {
        free(parser->json_string);
        free(parser);
    }
}

JsonError json_parser_parse(JsonParser* parser, const char* json_string, size_t length) {
    if (!parser || !json_string) {
        return JSON_ERROR_INVAL;
    }
    
    // Copy JSON string (jsmn requires non-const)
    free(parser->json_string);
    parser->json_string = malloc(length + 1);
    if (!parser->json_string) {
        return JSON_ERROR_NOMEM;
    }
    memcpy(parser->json_string, json_string, length);
    parser->json_string[length] = '\0';
    
    // Reset parser
    jsmn_init(&parser->parser);
    
    // Parse JSON
    int result = jsmn_parse(&parser->parser, parser->json_string, length, 
                           parser->tokens, MAX_TOKENS);
    
    if (result < 0) {
        log_error("JSON parsing failed: %d", result);
        return (JsonError)result;
    }
    
    parser->token_count = result;
    
    // Setup root value
    if (parser->token_count > 0) {
        parser->root_value.token = &parser->tokens[0];
        parser->root_value.json_string = parser->json_string;
    }
    
    log_debug("JSON parsed successfully: %d tokens", parser->token_count);
    return JSON_SUCCESS;
}

JsonValue* json_parser_get_root(JsonParser* parser) {
    if (!parser || parser->token_count == 0) {
        return NULL;
    }
    return &parser->root_value;
}

static int json_token_equal(const char* json, jsmntok_t* token, const char* str) {
    if (token->type == JSMN_STRING && 
        (int)strlen(str) == token->end - token->start &&
        strncmp(json + token->start, str, token->end - token->start) == 0) {
        return 1;
    }
    return 0;
}

JsonValue* json_object_get(JsonValue* object, const char* key) {
    if (!object || !key || object->token->type != JSMN_OBJECT) {
        return NULL;
    }
    
    jsmntok_t* tokens = object->token;
    const char* json = object->json_string;
    
    // Find the key in the object
    for (int i = 1; i < object->token->size * 2; i += 2) {
        if (json_token_equal(json, &tokens[i], key)) {
            // Found the key, return the value
            static JsonValue value;
            value.token = &tokens[i + 1];
            value.json_string = json;
            return &value;
        }
    }
    
    return NULL;
}

JsonValue* json_array_get(JsonValue* array, int index) {
    if (!array || array->token->type != JSMN_ARRAY || index < 0 || index >= array->token->size) {
        return NULL;
    }
    
    static JsonValue value;
    value.token = &array->token[index + 1];
    value.json_string = array->json_string;
    return &value;
}

int json_array_size(JsonValue* array) {
    if (!array || array->token->type != JSMN_ARRAY) {
        return 0;
    }
    return array->token->size;
}

JsonError json_value_get_string(JsonValue* value, char* buffer, size_t buffer_size) {
    if (!value || !buffer || value->token->type != JSMN_STRING) {
        return JSON_ERROR_TYPE;
    }
    
    int length = value->token->end - value->token->start;
    if (length >= (int)buffer_size) {
        return JSON_ERROR_NOMEM;
    }
    
    memcpy(buffer, value->json_string + value->token->start, length);
    buffer[length] = '\0';
    
    return JSON_SUCCESS;
}

JsonError json_value_get_int(JsonValue* value, int* result) {
    if (!value || !result || value->token->type != JSMN_PRIMITIVE) {
        return JSON_ERROR_TYPE;
    }
    
    char buffer[32];
    int length = value->token->end - value->token->start;
    if (length >= sizeof(buffer)) {
        return JSON_ERROR_NOMEM;
    }
    
    memcpy(buffer, value->json_string + value->token->start, length);
    buffer[length] = '\0';
    
    *result = atoi(buffer);
    return JSON_SUCCESS;
}

JsonError json_value_get_bool(JsonValue* value, bool* result) {
    if (!value || !result || value->token->type != JSMN_PRIMITIVE) {
        return JSON_ERROR_TYPE;
    }
    
    char buffer[8];
    int length = value->token->end - value->token->start;
    if (length >= sizeof(buffer)) {
        return JSON_ERROR_NOMEM;
    }
    
    memcpy(buffer, value->json_string + value->token->start, length);
    buffer[length] = '\0';
    
    if (strcmp(buffer, "true") == 0) {
        *result = true;
    } else if (strcmp(buffer, "false") == 0) {
        *result = false;
    } else {
        return JSON_ERROR_TYPE;
    }
    
    return JSON_SUCCESS;
}

JsonType json_value_get_type(JsonValue* value) {
    if (!value) {
        return JSON_TYPE_UNDEFINED;
    }
    return (JsonType)value->token->type;
}

bool json_value_is_null(JsonValue* value) {
    if (!value || value->token->type != JSMN_PRIMITIVE) {
        return false;
    }
    
    return strncmp(value->json_string + value->token->start, "null", 4) == 0;
}

const char* json_error_string(JsonError error) {
    switch (error) {
        case JSON_SUCCESS:
            return "Success";
        case JSON_ERROR_NOMEM:
            return "Not enough memory";
        case JSON_ERROR_INVAL:
            return "Invalid character";
        case JSON_ERROR_PART:
            return "Incomplete JSON";
        case JSON_ERROR_NOT_FOUND:
            return "Key not found";
        case JSON_ERROR_TYPE:
            return "Wrong type";
        default:
            return "Unknown error";
    }
}