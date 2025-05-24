#include "config_utils.h"
#include <string.h>
#include <strings.h>  // for strcasecmp

bool parse_bool(const char* value, bool* result) {
    if (!value || !result) {
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