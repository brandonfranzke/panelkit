#include "pages.h"
#include "../core/logger.h"
#include <string.h>
#include <math.h>

// Global page state - preserving exact behavior from app.c
static Page* pages = NULL;
static int current_page = 0;  // Start on page 1 (0-indexed, so 0 is the first page)
static int total_pages = 0;
static float page_transition = 0.0f;  // For smooth page transitions (0.0 = current page, 1.0 = target page)
static int target_page = -1;          // Page we're transitioning to, or -1 if not transitioning
static TransitionState transition_state = TRANSITION_NONE; // Current transition state
static float drag_offset = 0.0f;      // Current drag offset for page transitions

// Page indicator visibility - exact behavior from app.c
static bool show_indicators = false;   // Only show indicators during swipe and briefly after
static Uint32 indicator_hide_time = 0; // When to hide the indicators
static int indicator_alpha = 153;      // Indicator alpha value (60% opacity)
static bool indicator_fading = false;  // Whether indicator is currently fading out
static Uint32 fade_start_time = 0;     // When the fade animation started
#define FADE_DURATION 400              // Fade duration in milliseconds

// Render callback
static page_render_callback g_render_callback = NULL;

// Screen dimensions (will be set externally)
static int actual_width = 640;
static int actual_height = 480;

// Initialize page management system
void pages_init(int num_pages, page_render_callback render_cb) {
    total_pages = num_pages;
    g_render_callback = render_cb;
    
    // Allocate pages
    pages = calloc(num_pages, sizeof(Page));
    
    // Initialize pages with default values
    for (int i = 0; i < num_pages; i++) {
        pages[i].scroll_position = 0;
        pages[i].max_scroll = 0;
        pages[i].button_count = 0;
        pages[i].title = NULL;
    }
}

// Set screen dimensions
void pages_set_dimensions(int width, int height) {
    actual_width = width;
    actual_height = height;
}

// Get page data
Page* pages_get(int index) {
    if (index >= 0 && index < total_pages) {
        return &pages[index];
    }
    return NULL;
}

int pages_get_current(void) {
    return current_page;
}

int pages_get_total(void) {
    return total_pages;
}

// Start a page transition - exact implementation from app.c
void pages_transition_to(int page_index) {
    if (page_index >= 0 && page_index < total_pages && page_index != current_page) {
        target_page = page_index;
        transition_state = TRANSITION_ANIMATING;
        
        // Set initial transition value
        if (target_page > current_page) {
            // Moving forward (current slides left, new comes from right)
            page_transition = 0.0f;
        } else {
            // Moving backward (current slides right, new comes from left)
            page_transition = 0.0f;
        }
        
        log_event("PAGE_TRANSITION", "from=%d to=%d", current_page, page_index);
    }
}

// Handle page swipe - preserving exact behavior from update_gesture
void pages_handle_swipe(float offset, bool is_complete) {
    if (!is_complete) {
        // Still dragging
        float normalized_delta = offset / actual_width; // Convert to 0.0-1.0 scale
        
        // Show indicators whenever we detect horizontal dragging
        show_indicators = true;
        indicator_hide_time = SDL_GetTicks() + 2000; // Keep visible for 2 seconds after last drag
        
        // Start dragging transition if not already in one
        if (transition_state == TRANSITION_NONE) {
            transition_state = TRANSITION_DRAGGING;
            
            // Determine potential target page based on drag direction
            if (normalized_delta > 0 && current_page > 0) {
                // Dragging right - previous page is potential target 
                target_page = current_page - 1;
            } 
            else if (normalized_delta < 0 && current_page < total_pages - 1) {
                // Dragging left - next page is potential target
                target_page = current_page + 1;
            }
            else {
                // Can't go beyond first/last page
                target_page = current_page;
            }
        }
        
        // Update drag offset based on finger position (with limits)
        if (target_page != current_page) {
            // Calculate drag offset based on direction
            if (target_page > current_page) {
                // Dragging to next page (negative offset)
                drag_offset = fmax(normalized_delta, -1.0f); // Limit to -1.0
            } else {
                // Dragging to previous page (positive offset) 
                drag_offset = fmin(normalized_delta, 1.0f);  // Limit to 1.0
            }
        } else {
            // Elastic resistance when trying to go beyond first/last page
            drag_offset = normalized_delta * 0.3f; // Reduced movement to indicate boundary
        }
    } else {
        // Drag complete - handle end of horizontal drag (page swipe)
        if (transition_state == TRANSITION_DRAGGING) {
            float half_threshold = 0.3f; // Snap to new page if dragged more than 30% of screen width
            
            // Determine which page to snap to based on drag distance
            if (target_page != current_page) {
                bool snap_to_target = false;
                
                if (target_page > current_page) {
                    // Next page: check if dragged far enough left
                    snap_to_target = (drag_offset <= -half_threshold);
                } else {
                    // Previous page: check if dragged far enough right
                    snap_to_target = (drag_offset >= half_threshold);
                }
                
                if (snap_to_target) {
                    // Continue to target page
                    page_transition = drag_offset;
                    transition_state = TRANSITION_ANIMATING;
                    log_event("PAGE_SWIPE", "to=%d", target_page);
                } else {
                    // Snap back to current page
                    transition_state = TRANSITION_ANIMATING;
                    // Keep same target and current page, but animate back to 0 offset
                    log_debug("Swipe cancelled, returning to page %d", current_page);
                }
            } else {
                // Reset if dragging at page boundary
                transition_state = TRANSITION_NONE;
                target_page = -1;
                drag_offset = 0.0f;
            }
            
            // Show indicators and set timer to hide them after 2 seconds
            show_indicators = true;
            indicator_hide_time = SDL_GetTicks() + 2000; // 2 seconds from now
        }
    }
}

// Update page transition animation - should be called every frame
void pages_update_transition(void) {
    // Handle automatic page transitions
    if (transition_state == TRANSITION_ANIMATING) {
        float transition_speed = 0.12f; // 50% faster than original 0.08f
        
        // Determine transition direction based on target
        if (target_page > current_page) {
            // Moving forward (to higher page number)
            page_transition -= transition_speed;
            
            // Check if we've reached the target
            if (page_transition <= -1.0f) {
                current_page = target_page;
                target_page = -1;
                page_transition = 0.0f;
                transition_state = TRANSITION_NONE;
                log_debug("Page transition complete: now on page %d", current_page);
            }
        } else if (target_page < current_page) {
            // Moving backward (to lower page number)
            page_transition += transition_speed;
            
            // Check if we've reached the target
            if (page_transition >= 1.0f) {
                current_page = target_page;
                target_page = -1;
                page_transition = 0.0f;
                transition_state = TRANSITION_NONE;
                log_debug("Page transition complete: now on page %d", current_page);
            }
        } else {
            // Animating back to current page (swipe cancelled)
            if (page_transition > 0) {
                page_transition -= transition_speed;
                if (page_transition <= 0) {
                    page_transition = 0.0f;
                    transition_state = TRANSITION_NONE;
                    target_page = -1;
                }
            } else if (page_transition < 0) {
                page_transition += transition_speed;
                if (page_transition >= 0) {
                    page_transition = 0.0f;
                    transition_state = TRANSITION_NONE;
                    target_page = -1;
                }
            }
        }
    }
    
    // Update page transition for dragging
    if (transition_state == TRANSITION_DRAGGING) {
        page_transition = drag_offset;
    }
    
    // Handle indicator fading
    if (show_indicators && SDL_GetTicks() >= indicator_hide_time) {
        if (!indicator_fading) {
            // Start fade animation
            indicator_fading = true;
            fade_start_time = SDL_GetTicks();
        } else {
            // Update fade
            Uint32 fade_elapsed = SDL_GetTicks() - fade_start_time;
            if (fade_elapsed >= FADE_DURATION) {
                // Fade complete
                show_indicators = false;
                indicator_fading = false;
                indicator_alpha = 153; // Reset to default
            } else {
                // Calculate fade progress
                float fade_progress = (float)fade_elapsed / FADE_DURATION;
                indicator_alpha = (int)(153 * (1.0f - fade_progress));
            }
        }
    } else if (show_indicators && !indicator_fading) {
        // Reset alpha if indicators are visible but not fading
        indicator_alpha = 153;
    }
}

// Page indicators
void pages_set_indicators_visible(bool visible) {
    show_indicators = visible;
    if (visible) {
        indicator_hide_time = SDL_GetTicks() + 2000;
    }
}

bool pages_should_show_indicators(void) {
    return show_indicators;
}

int pages_get_indicator_alpha(void) {
    return indicator_alpha;
}

// Scroll management - preserving exact behavior from update_gesture
void pages_update_scroll(int page_index, int delta_y) {
    if (page_index >= 0 && page_index < total_pages) {
        // Update scroll position
        int old_scroll = pages[page_index].scroll_position;
        pages[page_index].scroll_position += delta_y;
        
        // Enforce limits
        if (pages[page_index].scroll_position < 0) {
            pages[page_index].scroll_position = 0;
        }
        if (pages[page_index].scroll_position > pages[page_index].max_scroll) {
            pages[page_index].scroll_position = pages[page_index].max_scroll;
        }
        
        // Log significant changes
        if (old_scroll != pages[page_index].scroll_position && abs(delta_y) > 5) {
            log_debug("Scroll position: %d (delta: %d)", 
                   pages[page_index].scroll_position, delta_y);
        }
    }
}

// Transition state getters
TransitionState pages_get_transition_state(void) {
    return transition_state;
}

float pages_get_transition_offset(void) {
    return page_transition;
}

int pages_get_target_page(void) {
    return target_page;
}