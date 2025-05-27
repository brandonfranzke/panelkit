#ifndef WIDGET_H
#define WIDGET_H

#include <SDL.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct EventSystem EventSystem;
typedef struct StateStore StateStore;
typedef struct Widget Widget;

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
typedef void (*widget_render_func)(Widget* widget, SDL_Renderer* renderer);
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
    
    // Style properties
    SDL_Color background_color;
    SDL_Color foreground_color;
    SDL_Color border_color;
    int border_width;
    int padding;
    
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
    
    // Private implementation data
    void* impl_data;
};

// Widget lifecycle functions
Widget* widget_create(const char* id, WidgetType type);
void widget_destroy(Widget* widget);

// Widget hierarchy management
bool widget_add_child(Widget* parent, Widget* child);
bool widget_remove_child(Widget* parent, Widget* child);
Widget* widget_find_child(Widget* parent, const char* id);
Widget* widget_find_descendant(Widget* root, const char* id);

// Event subscription management
bool widget_subscribe_event(Widget* widget, const char* event_name);
bool widget_unsubscribe_event(Widget* widget, const char* event_name);
void widget_connect_systems(Widget* widget, EventSystem* events, StateStore* store);

// Widget state management
void widget_set_state(Widget* widget, WidgetState state, bool enabled);
bool widget_has_state(Widget* widget, WidgetState state);
void widget_set_visible(Widget* widget, bool visible);
bool widget_is_visible(Widget* widget);
void widget_set_enabled(Widget* widget, bool enabled);
bool widget_is_enabled(Widget* widget);

// Layout and positioning
void widget_set_bounds(Widget* widget, int x, int y, int width, int height);
void widget_set_relative_bounds(Widget* widget, int x, int y, int width, int height);
void widget_invalidate_layout(Widget* widget);
void widget_perform_layout(Widget* widget);
void widget_update_child_bounds(Widget* parent);

// Rendering
void widget_render(Widget* widget, SDL_Renderer* renderer);
void widget_invalidate(Widget* widget);

// Event handling
void widget_handle_event(Widget* widget, const SDL_Event* event);
bool widget_contains_point(Widget* widget, int x, int y);
Widget* widget_hit_test(Widget* root, int x, int y);

// Update
void widget_update(Widget* widget, double delta_time);

// Default implementations for virtual functions
void widget_default_render(Widget* widget, SDL_Renderer* renderer);
void widget_default_handle_event(Widget* widget, const SDL_Event* event);
void widget_default_layout(Widget* widget);
void widget_default_destroy(Widget* widget);

#endif // WIDGET_H