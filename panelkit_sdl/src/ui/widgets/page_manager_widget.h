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

// Create a new page manager widget
Widget* page_manager_widget_create(const char* id, int page_count);

// Page manager specific functions
void page_manager_add_page(Widget* widget, int index, Widget* page);
void page_manager_set_current_page(Widget* widget, int page_index);
int page_manager_get_current_page(Widget* widget);
void page_manager_transition_to(Widget* widget, int page_index);

// Handle swipe gestures
void page_manager_handle_swipe(Widget* widget, float offset, bool is_complete);
void page_manager_update_drag(Widget* widget, float delta_x);

// Set page change callback
void page_manager_set_page_changed_callback(Widget* widget, 
    void (*callback)(int from, int to, void* user_data), void* user_data);

#endif // PAGE_MANAGER_WIDGET_H