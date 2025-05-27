#include "widget_manager.h"
#include "../events/event_system.h"
#include "../state/state_store.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "core/logger.h"

#define INITIAL_ROOT_CAPACITY 8

// Internal structure to track named roots
typedef struct {
    char name[64];
    Widget* root;
} NamedRoot;

WidgetManager* widget_manager_create(SDL_Renderer* renderer,
                                   EventSystem* event_system,
                                   StateStore* state_store) {
    WidgetManager* manager = calloc(1, sizeof(WidgetManager));
    if (!manager) {
        log_error("Failed to allocate widget manager");
        return NULL;
    }
    
    manager->renderer = renderer;
    manager->event_system = event_system;
    manager->state_store = state_store;
    
    manager->root_capacity = INITIAL_ROOT_CAPACITY;
    manager->roots = calloc(manager->root_capacity, sizeof(Widget*));
    if (!manager->roots) {
        free(manager);
        return NULL;
    }
    
    manager->last_update_time = SDL_GetTicks();
    
    log_info("Created widget manager");
    return manager;
}

void widget_manager_destroy(WidgetManager* manager) {
    if (!manager) {
        return;
    }
    
    // Clear focus and hover states
    widget_manager_clear_focus(manager);
    manager->hovered_widget = NULL;
    manager->pressed_widget = NULL;
    
    // Destroy all roots
    for (size_t i = 0; i < manager->root_count; i++) {
        widget_destroy(manager->roots[i]);
    }
    free(manager->roots);
    
    log_info("Destroyed widget manager");
    free(manager);
}

bool widget_manager_add_root(WidgetManager* manager, Widget* root, const char* name) {
    if (!manager || !root || !name) {
        return false;
    }
    
    // Check if name already exists
    for (size_t i = 0; i < manager->root_count; i++) {
        if (strcmp(manager->roots[i]->id, name) == 0) {
            log_error("Root with name '%s' already exists", name);
            return false;
        }
    }
    
    // Grow array if needed
    if (manager->root_count >= manager->root_capacity) {
        size_t new_capacity = manager->root_capacity * 2;
        Widget** new_roots = realloc(manager->roots, new_capacity * sizeof(Widget*));
        if (!new_roots) {
            log_error("Failed to grow roots array");
            return false;
        }
        manager->roots = new_roots;
        manager->root_capacity = new_capacity;
    }
    
    // Set root name to match
    strncpy(root->id, name, sizeof(root->id) - 1);
    
    // Connect systems
    widget_connect_systems(root, manager->event_system, manager->state_store);
    
    manager->roots[manager->root_count++] = root;
    
    // Set as active if first root
    if (manager->root_count == 1) {
        manager->active_root = root;
    }
    
    log_info("Added root widget '%s'", name);
    return true;
}

bool widget_manager_remove_root(WidgetManager* manager, const char* name) {
    if (!manager || !name) {
        return false;
    }
    
    for (size_t i = 0; i < manager->root_count; i++) {
        if (strcmp(manager->roots[i]->id, name) == 0) {
            Widget* root = manager->roots[i];
            
            // Clear active if removing active root
            if (manager->active_root == root) {
                manager->active_root = NULL;
            }
            
            // Clear focus if in this root
            if (manager->focused_widget && 
                widget_find_descendant(root, manager->focused_widget->id)) {
                widget_manager_clear_focus(manager);
            }
            
            // Destroy the root
            widget_destroy(root);
            
            // Shift remaining roots
            for (size_t j = i + 1; j < manager->root_count; j++) {
                manager->roots[j - 1] = manager->roots[j];
            }
            manager->root_count--;
            
            // Set new active if needed
            if (!manager->active_root && manager->root_count > 0) {
                manager->active_root = manager->roots[0];
            }
            
            log_info("Removed root widget '%s'", name);
            return true;
        }
    }
    
    return false;
}

Widget* widget_manager_get_root(WidgetManager* manager, const char* name) {
    if (!manager || !name) {
        return NULL;
    }
    
    for (size_t i = 0; i < manager->root_count; i++) {
        if (strcmp(manager->roots[i]->id, name) == 0) {
            return manager->roots[i];
        }
    }
    
    return NULL;
}

bool widget_manager_set_active_root(WidgetManager* manager, const char* name) {
    if (!manager || !name) {
        return false;
    }
    
    Widget* root = widget_manager_get_root(manager, name);
    if (!root) {
        log_error("Root '%s' not found", name);
        return false;
    }
    
    if (manager->active_root != root) {
        // Clear focus if changing roots
        widget_manager_clear_focus(manager);
        
        manager->active_root = root;
        widget_invalidate(root);
        
        log_info("Set active root to '%s'", name);
    }
    
    return true;
}

void widget_manager_handle_event(WidgetManager* manager, const SDL_Event* event) {
    if (!manager || !event || !manager->active_root) {
        return;
    }
    
    // Handle mouse/touch tracking
    switch (event->type) {
        case SDL_MOUSEMOTION:
        case SDL_FINGERMOTION: {
            int x, y;
            if (event->type == SDL_MOUSEMOTION) {
                x = event->motion.x;
                y = event->motion.y;
            } else {
                // Convert normalized touch coords
                int w, h;
                SDL_GetRendererOutputSize(manager->renderer, &w, &h);
                x = (int)(event->tfinger.x * w);
                y = (int)(event->tfinger.y * h);
            }
            
            Widget* hit = widget_hit_test(manager->active_root, x, y);
            if (hit != manager->hovered_widget) {
                // Clear old hover
                if (manager->hovered_widget) {
                    widget_set_state(manager->hovered_widget, WIDGET_STATE_HOVERED, false);
                }
                
                // Set new hover
                manager->hovered_widget = hit;
                if (hit) {
                    widget_set_state(hit, WIDGET_STATE_HOVERED, true);
                }
            }
            break;
        }
        
        case SDL_MOUSEBUTTONDOWN:
        case SDL_FINGERDOWN: {
            int x, y;
            if (event->type == SDL_MOUSEBUTTONDOWN) {
                x = event->button.x;
                y = event->button.y;
            } else {
                int w, h;
                SDL_GetRendererOutputSize(manager->renderer, &w, &h);
                x = (int)(event->tfinger.x * w);
                y = (int)(event->tfinger.y * h);
            }
            
            Widget* hit = widget_hit_test(manager->active_root, x, y);
            if (hit) {
                log_debug("WIDGET HIT: id='%s' type=%d at click pos (%d,%d), widget bounds=(%d,%d,%dx%d)", 
                         hit->id, hit->type, x, y,
                         hit->bounds.x, hit->bounds.y, hit->bounds.w, hit->bounds.h);
                
                // If it's a button, log additional info
                if (hit->type == WIDGET_TYPE_BUTTON) {
                    log_debug("HIT is a button widget: %s", hit->id);
                }
                
                manager->pressed_widget = hit;
                widget_set_state(hit, WIDGET_STATE_PRESSED, true);
                
                // Set focus
                if (hit != manager->focused_widget) {
                    widget_manager_set_focus(manager, hit);
                }
            }
            break;
        }
        
        case SDL_MOUSEBUTTONUP:
        case SDL_FINGERUP: {
            // Don't clear pressed state yet - let widgets handle the event first
            break;
        }
        
        // Mouse leave handling would go here if SDL supported it
        // For now, hover clearing happens when mouse moves to a different widget
    }
    
    // Let active root handle event
    widget_handle_event(manager->active_root, event);
    
    // Now clear pressed state after widgets have handled the event
    if ((event->type == SDL_MOUSEBUTTONUP || event->type == SDL_FINGERUP) && 
        manager->pressed_widget) {
        widget_set_state(manager->pressed_widget, WIDGET_STATE_PRESSED, false);
        manager->pressed_widget = NULL;
    }
}

void widget_manager_update(WidgetManager* manager) {
    if (!manager || !manager->active_root) {
        return;
    }
    
    uint32_t current_time = SDL_GetTicks();
    double delta_time = (current_time - manager->last_update_time) / 1000.0;
    manager->last_update_time = current_time;
    
    widget_update(manager->active_root, delta_time);
}

void widget_manager_render(WidgetManager* manager) {
    if (!manager || !manager->active_root || !manager->renderer) {
        return;
    }
    
    widget_render(manager->active_root, manager->renderer);
}

Widget* widget_manager_find_widget(WidgetManager* manager, const char* id) {
    if (!manager || !id) {
        return NULL;
    }
    
    // Search in all roots
    for (size_t i = 0; i < manager->root_count; i++) {
        Widget* found = widget_find_descendant(manager->roots[i], id);
        if (found) {
            return found;
        }
    }
    
    return NULL;
}

Widget* widget_manager_hit_test(WidgetManager* manager, int x, int y) {
    if (!manager || !manager->active_root) {
        return NULL;
    }
    
    return widget_hit_test(manager->active_root, x, y);
}

void widget_manager_set_focus(WidgetManager* manager, Widget* widget) {
    if (!manager) {
        return;
    }
    
    if (manager->focused_widget == widget) {
        return;
    }
    
    // Clear old focus
    if (manager->focused_widget) {
        widget_set_state(manager->focused_widget, WIDGET_STATE_FOCUSED, false);
    }
    
    // Set new focus
    manager->focused_widget = widget;
    if (widget) {
        widget_set_state(widget, WIDGET_STATE_FOCUSED, true);
        log_debug("Set focus to widget '%s'", widget->id);
    }
}

Widget* widget_manager_get_focus(WidgetManager* manager) {
    return manager ? manager->focused_widget : NULL;
}

void widget_manager_clear_focus(WidgetManager* manager) {
    widget_manager_set_focus(manager, NULL);
}