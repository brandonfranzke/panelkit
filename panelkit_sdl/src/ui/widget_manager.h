#ifndef WIDGET_MANAGER_H
#define WIDGET_MANAGER_H

#include "widget.h"
#include <SDL.h>

// Forward declarations
typedef struct EventSystem EventSystem;
typedef struct StateStore StateStore;

// Widget manager - handles widget lifecycle and rendering
typedef struct WidgetManager {
    // Root widgets for different screens/pages
    Widget** roots;
    size_t root_count;
    size_t root_capacity;
    
    // Currently active root
    Widget* active_root;
    
    // Widget under mouse/touch
    Widget* hovered_widget;
    Widget* pressed_widget;
    Widget* focused_widget;
    
    // Systems
    EventSystem* event_system;
    StateStore* state_store;
    
    // Rendering context
    SDL_Renderer* renderer;
    
    // Timing
    uint32_t last_update_time;
} WidgetManager;

/* Lifecycle */

/**
 * Create a new widget manager.
 * 
 * @param renderer SDL renderer for drawing widgets
 * @param event_system Event system for widget events (can be NULL)
 * @param state_store State store for widget state (can be NULL)
 * @return New widget manager or NULL on error (caller owns)
 */
WidgetManager* widget_manager_create(SDL_Renderer* renderer,
                                   EventSystem* event_system,
                                   StateStore* state_store);

/**
 * Destroy widget manager and all managed widgets.
 * 
 * @param manager Widget manager to destroy
 * @note Destroys all root widgets and their children
 */
void widget_manager_destroy(WidgetManager* manager);

/* Root management */

/**
 * Add a root widget to the manager.
 * 
 * @param manager Widget manager
 * @param root Root widget to add (manager takes ownership)
 * @param name Unique name for this root
 * @return true on success, false on error
 */
bool widget_manager_add_root(WidgetManager* manager, Widget* root, const char* name);

/**
 * Remove a root widget from the manager.
 * 
 * @param manager Widget manager
 * @param name Name of root to remove
 * @return true if removed, false if not found
 * @note Destroys the root widget and all its children
 */
bool widget_manager_remove_root(WidgetManager* manager, const char* name);

/**
 * Get a root widget by name.
 * 
 * @param manager Widget manager
 * @param name Name of root to find
 * @return Root widget or NULL if not found (borrowed reference)
 */
Widget* widget_manager_get_root(WidgetManager* manager, const char* name);

/**
 * Set the active root widget for rendering and events.
 * 
 * @param manager Widget manager
 * @param name Name of root to activate
 * @return true if activated, false if not found
 */
bool widget_manager_set_active_root(WidgetManager* manager, const char* name);

/* Event handling */

/**
 * Handle SDL event for active root and its children.
 * 
 * @param manager Widget manager
 * @param event SDL event to process
 * @note Updates hover, pressed, and focus states
 */
void widget_manager_handle_event(WidgetManager* manager, const SDL_Event* event);

/* Update and render */

/**
 * Update all widgets based on elapsed time.
 * 
 * @param manager Widget manager
 * @note Calls widget_update on active root recursively
 */
void widget_manager_update(WidgetManager* manager);

/**
 * Render the active root widget and its children.
 * 
 * @param manager Widget manager
 * @note Only renders if active root is set
 */
void widget_manager_render(WidgetManager* manager);

/* Widget finding */

/**
 * Find a widget by ID in any root.
 * 
 * @param manager Widget manager
 * @param id Widget ID to search for
 * @return Found widget or NULL (borrowed reference)
 */
Widget* widget_manager_find_widget(WidgetManager* manager, const char* id);

/**
 * Find widget at screen coordinates in active root.
 * 
 * @param manager Widget manager
 * @param x X coordinate in pixels
 * @param y Y coordinate in pixels
 * @return Widget at position or NULL (borrowed reference)
 */
Widget* widget_manager_hit_test(WidgetManager* manager, int x, int y);

/* Focus management */

/**
 * Set keyboard focus to a specific widget.
 * 
 * @param manager Widget manager
 * @param widget Widget to focus (can be NULL to clear focus)
 */
void widget_manager_set_focus(WidgetManager* manager, Widget* widget);

/**
 * Get the currently focused widget.
 * 
 * @param manager Widget manager
 * @return Focused widget or NULL (borrowed reference)
 */
Widget* widget_manager_get_focus(WidgetManager* manager);

/**
 * Clear keyboard focus from all widgets.
 * 
 * @param manager Widget manager
 */
void widget_manager_clear_focus(WidgetManager* manager);

#endif // WIDGET_MANAGER_H