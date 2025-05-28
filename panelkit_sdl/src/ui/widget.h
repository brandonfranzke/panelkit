/**
 * @file widget.h
 * @brief Core widget system interface
 * 
 * Base widget structure and functions for the UI framework.
 * All UI elements inherit from the base Widget type.
 */

#ifndef WIDGET_H
#define WIDGET_H

#include <SDL.h>
#include <stdbool.h>
#include <stddef.h>
#include "../core/error.h"

// Forward declarations
typedef struct EventSystem EventSystem;
typedef struct StateStore StateStore;
typedef struct Widget Widget;
typedef struct Style Style;
typedef struct StyleBase StyleBase;

// Widget types enumeration
typedef enum {
    WIDGET_TYPE_BASE,
    WIDGET_TYPE_BUTTON,
    WIDGET_TYPE_LABEL,
    WIDGET_TYPE_WEATHER,
    WIDGET_TYPE_CONTAINER,
    WIDGET_TYPE_CUSTOM
} WidgetType;

// Widget state flags
typedef enum {
    WIDGET_STATE_NORMAL    = 0,
    WIDGET_STATE_HOVERED   = 1 << 0,
    WIDGET_STATE_PRESSED   = 1 << 1,
    WIDGET_STATE_FOCUSED   = 1 << 2,
    WIDGET_STATE_DISABLED  = 1 << 3,
    WIDGET_STATE_HIDDEN    = 1 << 4,
    WIDGET_STATE_DIRTY     = 1 << 5  // Needs redraw
} WidgetState;

// Widget event handler function types
typedef void (*widget_event_handler)(Widget* widget, const SDL_Event* event);
typedef void (*widget_data_handler)(Widget* widget, const char* event_name,
                                   const void* data, size_t data_size);
typedef PkError (*widget_render_func)(Widget* widget, SDL_Renderer* renderer);
typedef void (*widget_update_func)(Widget* widget, double delta_time);
typedef void (*widget_destroy_func)(Widget* widget);

// Base widget structure - all widgets inherit from this
struct Widget {
    // Identity
    char id[64];
    WidgetType type;
    
    // Hierarchy
    Widget* parent;
    Widget** children;
    size_t child_count;
    size_t child_capacity;
    
    // Layout
    SDL_Rect bounds;           // Absolute position and size
    SDL_Rect relative_bounds;  // Position relative to parent
    
    // State
    uint32_t state_flags;
    bool needs_layout;
    
    // Event subscriptions
    char** subscribed_events;
    size_t event_count;
    size_t event_capacity;
    
    // Style system
    Style* style;                  // Widget's style (owned or referenced)
    bool style_owned;              // true if widget owns the style
    const StyleBase* active_style; // Currently active style (resolved from state)
    
    // References (not owned)
    EventSystem* event_system;
    StateStore* state_store;
    
    // Virtual function table
    widget_render_func render;
    widget_event_handler handle_event;
    widget_data_handler handle_data_event;
    widget_update_func update;
    widget_destroy_func destroy;
    
    // Layout functions
    void (*layout)(Widget* widget);
    void (*measure)(Widget* widget, int* width, int* height);
};

/* Widget lifecycle functions */

/**
 * Create a new widget with the specified ID and type.
 * 
 * @param id Unique identifier for the widget (required, copied internally)
 * @param type Widget type from WidgetType enum
 * @return New widget or NULL on error (caller owns)
 * @note Call pk_get_last_error() on NULL return for error details
 */
Widget* widget_create(const char* id, WidgetType type);

/**
 * Destroy a widget and all its children recursively.
 * 
 * @param widget Widget to destroy (can be NULL)
 * @note Unsubscribes from all events and disconnects from systems
 */
void widget_destroy(Widget* widget);

/* Widget hierarchy management */

/**
 * Add a child widget to a parent widget.
 * 
 * @param parent Parent widget to add child to
 * @param child Child widget to add (removed from previous parent if any)
 * @return true on success, false on error
 * @note Parent takes ownership of child
 */
bool widget_add_child(Widget* parent, Widget* child);

/**
 * Remove a child widget from its parent.
 * 
 * @param parent Parent widget to remove child from
 * @param child Child widget to remove
 * @return true if child was removed, false if not found
 * @note Caller becomes responsible for the removed child
 */
bool widget_remove_child(Widget* parent, Widget* child);

/**
 * Find a direct child widget by ID.
 * 
 * @param parent Parent widget to search in
 * @param id ID of child to find
 * @return Child widget or NULL if not found (borrowed reference)
 */
Widget* widget_find_child(Widget* parent, const char* id);

/**
 * Find a descendant widget by ID (recursive search).
 * 
 * @param root Root widget to start search from
 * @param id ID of widget to find
 * @return Found widget or NULL if not found (borrowed reference)
 */
Widget* widget_find_descendant(Widget* root, const char* id);

/* Event subscription management */

/**
 * Subscribe a widget to receive a specific event type.
 * 
 * @param widget Widget to subscribe
 * @param event_name Name of event to subscribe to
 * @return true on success, false on error
 * @note Widget must be connected to an event system first
 */
bool widget_subscribe_event(Widget* widget, const char* event_name);

/**
 * Unsubscribe a widget from a specific event type.
 * 
 * @param widget Widget to unsubscribe
 * @param event_name Name of event to unsubscribe from
 * @return true if unsubscribed, false if not found
 */
bool widget_unsubscribe_event(Widget* widget, const char* event_name);

/**
 * Connect a widget to event system and state store.
 * 
 * @param widget Widget to connect
 * @param events Event system (can be NULL)
 * @param store State store (can be NULL)
 * @note Recursively connects all children
 */
/**
 * Connect widget to event and state systems.
 * 
 * @param widget Widget to connect (required)
 * @param events Event system (can be NULL)
 * @param store State store (can be NULL)
 * @return PK_OK on success, error code on failure
 * @note Propagates to all children recursively
 */
PkError widget_connect_systems(Widget* widget, EventSystem* events, StateStore* store);

/* Widget state management */

/**
 * Set or clear a widget state flag.
 * 
 * @param widget Widget to modify
 * @param state State flag to set/clear
 * @param enabled true to set, false to clear
 */
void widget_set_state(Widget* widget, WidgetState state, bool enabled);

/**
 * Check if a widget has a specific state flag set.
 * 
 * @param widget Widget to check
 * @param state State flag to check
 * @return true if state is set, false otherwise
 */
bool widget_has_state(Widget* widget, WidgetState state);

/**
 * Set widget visibility.
 * 
 * @param widget Widget to show/hide
 * @param visible true to show, false to hide
 * @note Hidden widgets don't receive events or render
 */
void widget_set_visible(Widget* widget, bool visible);

/**
 * Check if widget is visible.
 * 
 * @param widget Widget to check
 * @return true if visible, false if hidden or NULL
 */
bool widget_is_visible(Widget* widget);

/**
 * Enable or disable a widget.
 * 
 * @param widget Widget to enable/disable
 * @param enabled true to enable, false to disable
 * @note Disabled widgets don't receive input events
 */
void widget_set_enabled(Widget* widget, bool enabled);

/**
 * Check if widget is enabled.
 * 
 * @param widget Widget to check
 * @return true if enabled, false if disabled or NULL
 */
bool widget_is_enabled(Widget* widget);

/* Style management */

/**
 * Set widget style (referenced, not owned).
 * 
 * @param widget Widget to style
 * @param style Style to apply (widget references, doesn't own)
 * @note Previous owned style is freed if style_owned was true
 */
void widget_set_style_ref(Widget* widget, Style* style);

/**
 * Set widget style (owned).
 * 
 * @param widget Widget to style
 * @param style Style to apply (widget takes ownership)
 * @note Previous owned style is freed if style_owned was true
 */
void widget_set_style_owned(Widget* widget, Style* style);

/**
 * Update active style based on current widget state.
 * 
 * @param widget Widget to update
 * @note Sets active_style to appropriate state variant
 */
void widget_update_active_style(Widget* widget);

/**
 * Get the current active style.
 * 
 * @param widget Widget to query
 * @return Active style or NULL (borrowed reference)
 */
const StyleBase* widget_get_active_style(Widget* widget);

/* Layout and positioning */

/**
 * Set absolute widget bounds.
 * 
 * @param widget Widget to position
 * @param x X coordinate in pixels
 * @param y Y coordinate in pixels
 * @param width Widget width in pixels
 * @param height Widget height in pixels
 */
void widget_set_bounds(Widget* widget, int x, int y, int width, int height);

/**
 * Set widget bounds relative to parent.
 * 
 * @param widget Widget to position
 * @param x X offset from parent's content area
 * @param y Y offset from parent's content area
 * @param width Widget width in pixels
 * @param height Widget height in pixels
 */
void widget_set_relative_bounds(Widget* widget, int x, int y, int width, int height);

/**
 * Mark widget as needing layout recalculation.
 * 
 * @param widget Widget that needs layout
 * @note Propagates up to root widget
 */
void widget_invalidate_layout(Widget* widget);

/**
 * Perform layout calculation for widget and children.
 * 
 * @param widget Widget to layout
 * @note Calls widget's layout function if set
 */
void widget_perform_layout(Widget* widget);

/**
 * Update bounds of all child widgets.
 * 
 * @param parent Parent widget whose children need positioning
 * @note Called automatically during layout
 */
void widget_update_child_bounds(Widget* parent);

/* Rendering */

/**
 * Render widget and all visible children.
 * 
 * @param widget Widget to render
 * @param renderer SDL renderer to draw to
 * @note Skips hidden widgets and their children
 */
/**
 * Render widget and its children recursively.
 * 
 * @param widget Widget to render (required)
 * @param renderer SDL renderer to use (required)
 * @return PK_OK on success, error code on failure
 * @note Performs layout if needed before rendering
 */
PkError widget_render(Widget* widget, SDL_Renderer* renderer);

/**
 * Mark widget as needing redraw.
 * 
 * @param widget Widget that needs redrawing
 * @note Currently triggers full screen redraw
 */
void widget_invalidate(Widget* widget);

/* Event handling */

/**
 * Handle SDL event for widget and children.
 * 
 * @param widget Widget to handle event
 * @param event SDL event to process
 * @note Events propagate from children to parents
 */
void widget_handle_event(Widget* widget, const SDL_Event* event);

/**
 * Check if point is within widget bounds.
 * 
 * @param widget Widget to test
 * @param x X coordinate to test
 * @param y Y coordinate to test
 * @return true if point is inside widget bounds
 */
bool widget_contains_point(Widget* widget, int x, int y);

/**
 * Find the deepest widget containing a point.
 * 
 * @param root Root widget to start search
 * @param x X coordinate to test
 * @param y Y coordinate to test
 * @return Deepest widget at point or NULL (borrowed reference)
 */
Widget* widget_hit_test(Widget* root, int x, int y);

/* Update */

/**
 * Update widget state based on elapsed time.
 * 
 * @param widget Widget to update
 * @param delta_time Time elapsed since last update in seconds
 * @note Recursively updates all children
 */
void widget_update(Widget* widget, double delta_time);

/* Default implementations for virtual functions */

/**
 * Default rendering implementation.
 * 
 * @param widget Widget to render
 * @param renderer SDL renderer
 * @note Draws background, border, and children
 */
PkError widget_default_render(Widget* widget, SDL_Renderer* renderer);

/**
 * Default event handling implementation.
 * 
 * @param widget Widget handling event
 * @param event SDL event to handle
 * @note Handles basic mouse events and state changes
 */
void widget_default_handle_event(Widget* widget, const SDL_Event* event);

/**
 * Default layout implementation.
 * 
 * @param widget Widget to layout
 * @note Simple vertical stacking of children
 */
void widget_default_layout(Widget* widget);

/**
 * Default destroy implementation.
 * 
 * @param widget Widget being destroyed
 * @note Base cleanup handled by widget_destroy
 */
void widget_default_destroy(Widget* widget);

// Type-safe casting macros
#define WIDGET_CAST(type, widget) \
    ((widget) && (widget)->type == (type) ? (widget) : NULL)

#define CAST_TO_BUTTON(widget) \
    ((widget) && (widget)->type == WIDGET_TYPE_BUTTON ? (ButtonWidget*)(widget) : NULL)

#define CAST_TO_LABEL(widget) \
    ((widget) && (widget)->type == WIDGET_TYPE_LABEL ? (void*)(widget) : NULL)

#define CAST_TO_WEATHER(widget) \
    ((widget) && (widget)->type == WIDGET_TYPE_WEATHER ? (void*)(widget) : NULL)

#define CAST_TO_CONTAINER(widget) \
    ((widget) && (widget)->type == WIDGET_TYPE_CONTAINER ? (widget) : NULL)

#endif // WIDGET_H