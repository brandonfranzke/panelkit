#ifndef EVENT_TYPES_H
#define EVENT_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// Common event data structures - replacing anonymous structs

// Button event data
typedef struct {
    int button_index;
    int page;
    uint32_t timestamp;
    char button_text[32];
} ButtonEventData;

// Page change event data
typedef struct {
    int from_page;
    int to_page;
    uint32_t timestamp;
} PageChangeEventData;

// Touch event data
typedef struct {
    int x;
    int y;
    bool is_down;
    uint32_t timestamp;
} TouchEventData;

// API state change event data
typedef struct {
    char state_name[64];
    char value[256];
} ApiStateChangeData;

// API refresh event data
typedef struct {
    uint32_t timestamp;
    char source[32];
} ApiRefreshData;

#endif // EVENT_TYPES_H