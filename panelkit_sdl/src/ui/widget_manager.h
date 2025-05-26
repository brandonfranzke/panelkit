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

// Lifecycle
WidgetManager* widget_manager_create(SDL_Renderer* renderer,
                                   EventSystem* event_system,
                                   StateStore* state_store);
void widget_manager_destroy(WidgetManager* manager);

// Root management
bool widget_manager_add_root(WidgetManager* manager, Widget* root, const char* name);
bool widget_manager_remove_root(WidgetManager* manager, const char* name);
Widget* widget_manager_get_root(WidgetManager* manager, const char* name);
bool widget_manager_set_active_root(WidgetManager* manager, const char* name);

// Event handling
void widget_manager_handle_event(WidgetManager* manager, const SDL_Event* event);

// Update and render
void widget_manager_update(WidgetManager* manager);
void widget_manager_render(WidgetManager* manager);

// Widget finding
Widget* widget_manager_find_widget(WidgetManager* manager, const char* id);
Widget* widget_manager_hit_test(WidgetManager* manager, int x, int y);

// Focus management
void widget_manager_set_focus(WidgetManager* manager, Widget* widget);
Widget* widget_manager_get_focus(WidgetManager* manager);
void widget_manager_clear_focus(WidgetManager* manager);

#endif // WIDGET_MANAGER_H