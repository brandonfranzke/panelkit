#include "config_utils.h"
#include "../core/error.h"
#include <string.h>
#include <strings.h>  // for strcasecmp

bool parse_bool(const char* value, bool* result) {
    if (!value || !result) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
            "parse_bool: value=%p, result=%p", (void*)value, (void*)result);
        return false;
    }
    
    if (strcasecmp(value, "true") == 0 || 
        strcasecmp(value, "yes") == 0 || 
        strcasecmp(value, "on") == 0 || 
        strcmp(value, "1") == 0) {
        *result = true;
        return true;
    }
    
    if (strcasecmp(value, "false") == 0 || 
        strcasecmp(value, "no") == 0 || 
        strcasecmp(value, "off") == 0 || 
        strcmp(value, "0") == 0) {
        *result = false;
        return true;
    }
    
    return false;
}