#ifndef PAGE_MANAGER_WIDGET_H
#define PAGE_MANAGER_WIDGET_H

#include "../widget.h"
#include <stdbool.h>

// Page transition states
typedef enum {
    PAGE_TRANSITION_NONE,
    PAGE_TRANSITION_DRAGGING,
    PAGE_TRANSITION_ANIMATING
} PageTransitionState;

// Page manager widget - handles multiple pages and transitions
typedef struct PageManagerWidget {
    Widget base;
    
    // Page management
    int current_page;
    int target_page;
    int page_count;
    Widget** pages;  // Array of page widgets
    
    // Transition state
    PageTransitionState transition_state;
    float transition_offset;  // -1.0 to 1.0
    float drag_offset;
    
    // Page indicators
    bool show_indicators;
    int indicator_alpha;
    uint32_t indicator_hide_time;
    uint32_t last_interaction_time;
    
    // Callbacks
    void (*on_page_changed)(int from_page, int to_page, void* user_data);
    void* callback_user_data;
} PageManagerWidget;

/**
 * Create a new page manager widget.
 * 
 * @param id Unique identifier for the widget
 * @param page_count Number of pages to manage
 * @return New page manager widget or NULL on error (caller owns)
 * @note Pages must be added separately with page_manager_add_page
 */
Widget* page_manager_widget_create(const char* id, int page_count);

/**
 * Add a page widget at the specified index.
 * 
 * @param widget Page manager widget
 * @param index Page index (0-based)
 * @param page Page widget to add (manager takes ownership)
 * @note Index must be within page_count range
 */
void page_manager_add_page(Widget* widget, int index, Widget* page);

/**
 * Set the current page immediately without animation.
 * 
 * @param widget Page manager widget
 * @param page_index Page index to display
 */
void page_manager_set_current_page(Widget* widget, int page_index);

/**
 * Get the currently displayed page index.
 * 
 * @param widget Page manager widget
 * @return Current page index or -1 on error
 */
int page_manager_get_current_page(Widget* widget);

/**
 * Transition to a page with animation.
 * 
 * @param widget Page manager widget
 * @param page_index Target page index
 * @note Triggers page change callback when complete
 */
void page_manager_transition_to(Widget* widget, int page_index);

/**
 * Handle swipe gesture for page transitions.
 * 
 * @param widget Page manager widget
 * @param offset Swipe offset in pixels
 * @param is_complete True if gesture is complete
 */
void page_manager_handle_swipe(Widget* widget, float offset, bool is_complete);

/**
 * Update dragging offset during swipe gesture.
 * 
 * @param widget Page manager widget
 * @param delta_x Horizontal movement delta in pixels
 */
void page_manager_update_drag(Widget* widget, float delta_x);

/**
 * Set callback for page change notifications.
 * 
 * @param widget Page manager widget
 * @param callback Function to call on page change (can be NULL)
 * @param user_data User data passed to callback (not owned)
 */
void page_manager_set_page_changed_callback(Widget* widget, 
    void (*callback)(int from, int to, void* user_data), void* user_data);

#endif // PAGE_MANAGER_WIDGET_H