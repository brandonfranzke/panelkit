#ifndef PAGES_H
#define PAGES_H

#include "../core/sdl_includes.h"
#include <stdbool.h>

// Page transition states - exact enum from app.c
typedef enum {
    TRANSITION_NONE,      // No transition in progress
    TRANSITION_DRAGGING,  // User is actively dragging between pages
    TRANSITION_ANIMATING  // Automatic animation after drag release
} TransitionState;

// Page definition - exact struct from app.c
typedef struct {
    int scroll_position;         // Vertical scroll position for this page
    int max_scroll;              // Maximum scroll value for this page
    const char* title;           // Page title
    int button_count;            // Number of buttons on this page
    char button_texts[9][64];    // Text for each button
    SDL_Color button_colors[9];  // Color for each button
} Page;

// Page dimensions callback
typedef void (*page_render_callback)(int page_index, float offset_x);

// Initialize page management system
void pages_init(int total_pages, page_render_callback render_cb);

// Set screen dimensions
void pages_set_dimensions(int width, int height);

// Get page data
Page* pages_get(int index);
int pages_get_current(void);
int pages_get_total(void);

// Page transitions
void pages_transition_to(int page_index);
void pages_handle_swipe(float offset, bool is_complete);
void pages_update_transition(void);

// Page indicators
void pages_set_indicators_visible(bool visible);
bool pages_should_show_indicators(void);
int pages_get_indicator_alpha(void);

// Scroll management
void pages_update_scroll(int page_index, int delta_y);

// Transition state
TransitionState pages_get_transition_state(void);
float pages_get_transition_offset(void);
int pages_get_target_page(void);

#endif // PAGES_H