#include "widget.h"
#include "../events/event_system.h"
#include "../state/state_store.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "core/logger.h"

#define INITIAL_CHILD_CAPACITY 4
#define INITIAL_EVENT_CAPACITY 4

// Internal event handler that dispatches to widget
static void widget_event_handler_callback(const char* event_name,
                                        const void* data,
                                        size_t data_size,
                                        void* context) {
    Widget* widget = (Widget*)context;
    if (!widget || !widget->handle_data_event) {
        return;
    }
    
    widget->handle_data_event(widget, event_name, data, data_size);
    widget_invalidate(widget);  // Mark for redraw
}

Widget* widget_create(const char* id, WidgetType type) {
    Widget* widget = calloc(1, sizeof(Widget));
    if (!widget) {
        log_error("Failed to allocate widget");
        return NULL;
    }
    
    // Set identity
    strncpy(widget->id, id, sizeof(widget->id) - 1);
    widget->type = type;
    
    // Initialize arrays
    widget->child_capacity = INITIAL_CHILD_CAPACITY;
    widget->children = calloc(widget->child_capacity, sizeof(Widget*));
    if (!widget->children) {
        free(widget);
        return NULL;
    }
    
    widget->event_capacity = INITIAL_EVENT_CAPACITY;
    widget->subscribed_events = calloc(widget->event_capacity, sizeof(char*));
    if (!widget->subscribed_events) {
        free(widget->children);
        free(widget);
        return NULL;
    }
    
    // Default colors
    widget->background_color = (SDL_Color){33, 33, 33, 255};  // Dark like app
    widget->foreground_color = (SDL_Color){255, 255, 255, 255};  // White text
    widget->border_color = (SDL_Color){100, 100, 100, 255};  // Gray border
    widget->border_width = 1;
    widget->padding = 5;
    
    // Set default virtual functions
    widget->render = widget_default_render;
    widget->handle_event = widget_default_handle_event;
    widget->layout = widget_default_layout;
    widget->destroy = widget_default_destroy;
    
    log_debug("Created widget '%s' of type %d", id, type);
    return widget;
}

void widget_destroy(Widget* widget) {
    if (!widget) {
        return;
    }
    
    // Unsubscribe from all events
    if (widget->event_system) {
        for (size_t i = 0; i < widget->event_count; i++) {
            event_unsubscribe(widget->event_system, widget->subscribed_events[i],
                            widget_event_handler_callback);
            free(widget->subscribed_events[i]);
        }
    }
    free(widget->subscribed_events);
    
    // Remove from parent
    if (widget->parent) {
        widget_remove_child(widget->parent, widget);
    }
    
    // Destroy children
    for (size_t i = 0; i < widget->child_count; i++) {
        if (widget->children[i]) {
            widget->children[i]->parent = NULL;  // Prevent removal attempt
            widget_destroy(widget->children[i]);
        }
    }
    free(widget->children);
    
    // Call type-specific destructor
    if (widget->destroy) {
        widget->destroy(widget);
    }
    
    log_debug("Destroyed widget '%s'", widget->id);
    free(widget);
}

bool widget_add_child(Widget* parent, Widget* child) {
    if (!parent || !child) {
        return false;
    }
    
    // Remove from current parent if any
    if (child->parent) {
        widget_remove_child(child->parent, child);
    }
    
    // Grow array if needed
    if (parent->child_count >= parent->child_capacity) {
        size_t new_capacity = parent->child_capacity * 2;
        Widget** new_children = realloc(parent->children, 
                                       new_capacity * sizeof(Widget*));
        if (!new_children) {
            log_error("Failed to grow children array");
            return false;
        }
        parent->children = new_children;
        parent->child_capacity = new_capacity;
    }
    
    parent->children[parent->child_count++] = child;
    child->parent = parent;
    
    // Inherit event system and state store
    if (parent->event_system || parent->state_store) {
        widget_connect_systems(child, parent->event_system, parent->state_store);
    }
    
    widget_invalidate_layout(parent);
    log_debug("Added child '%s' to parent '%s'", child->id, parent->id);
    return true;
}

bool widget_remove_child(Widget* parent, Widget* child) {
    if (!parent || !child || child->parent != parent) {
        return false;
    }
    
    for (size_t i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) {
            // Shift remaining children
            for (size_t j = i + 1; j < parent->child_count; j++) {
                parent->children[j - 1] = parent->children[j];
            }
            parent->child_count--;
            child->parent = NULL;
            
            widget_invalidate_layout(parent);
            log_debug("Removed child '%s' from parent '%s'", child->id, parent->id);
            return true;
        }
    }
    
    return false;
}

Widget* widget_find_child(Widget* parent, const char* id) {
    if (!parent || !id) {
        return NULL;
    }
    
    for (size_t i = 0; i < parent->child_count; i++) {
        if (strcmp(parent->children[i]->id, id) == 0) {
            return parent->children[i];
        }
    }
    
    return NULL;
}

Widget* widget_find_descendant(Widget* root, const char* id) {
    if (!root || !id) {
        return NULL;
    }
    
    if (strcmp(root->id, id) == 0) {
        return root;
    }
    
    for (size_t i = 0; i < root->child_count; i++) {
        Widget* found = widget_find_descendant(root->children[i], id);
        if (found) {
            return found;
        }
    }
    
    return NULL;
}

bool widget_subscribe_event(Widget* widget, const char* event_name) {
    if (!widget || !event_name) {
        return false;
    }
    
    // Check if already subscribed
    for (size_t i = 0; i < widget->event_count; i++) {
        if (strcmp(widget->subscribed_events[i], event_name) == 0) {
            return true;  // Already subscribed
        }
    }
    
    // Grow array if needed
    if (widget->event_count >= widget->event_capacity) {
        size_t new_capacity = widget->event_capacity * 2;
        char** new_events = realloc(widget->subscribed_events,
                                   new_capacity * sizeof(char*));
        if (!new_events) {
            log_error("Failed to grow events array");
            return false;
        }
        widget->subscribed_events = new_events;
        widget->event_capacity = new_capacity;
    }
    
    // Store event name
    widget->subscribed_events[widget->event_count] = strdup(event_name);
    if (!widget->subscribed_events[widget->event_count]) {
        return false;
    }
    widget->event_count++;
    
    // Subscribe if connected to event system
    if (widget->event_system) {
        bool success = event_subscribe(widget->event_system, event_name,
                                     widget_event_handler_callback, widget);
        if (!success) {
            // Rollback
            free(widget->subscribed_events[--widget->event_count]);
            return false;
        }
    }
    
    log_debug("Widget '%s' subscribed to event '%s'", widget->id, event_name);
    return true;
}

bool widget_unsubscribe_event(Widget* widget, const char* event_name) {
    if (!widget || !event_name) {
        return false;
    }
    
    for (size_t i = 0; i < widget->event_count; i++) {
        if (strcmp(widget->subscribed_events[i], event_name) == 0) {
            // Unsubscribe from event system
            if (widget->event_system) {
                event_unsubscribe(widget->event_system, event_name,
                                widget_event_handler_callback);
            }
            
            // Remove from array
            free(widget->subscribed_events[i]);
            for (size_t j = i + 1; j < widget->event_count; j++) {
                widget->subscribed_events[j - 1] = widget->subscribed_events[j];
            }
            widget->event_count--;
            
            log_debug("Widget '%s' unsubscribed from event '%s'", widget->id, event_name);
            return true;
        }
    }
    
    return false;
}

void widget_connect_systems(Widget* widget, EventSystem* events, StateStore* store) {
    if (!widget) {
        return;
    }
    
    // Unsubscribe from old system if changing
    if (widget->event_system && widget->event_system != events) {
        for (size_t i = 0; i < widget->event_count; i++) {
            event_unsubscribe(widget->event_system, widget->subscribed_events[i],
                            widget_event_handler_callback);
        }
    }
    
    widget->event_system = events;
    widget->state_store = store;
    
    // Resubscribe to new system
    if (events) {
        for (size_t i = 0; i < widget->event_count; i++) {
            event_subscribe(events, widget->subscribed_events[i],
                          widget_event_handler_callback, widget);
        }
    }
    
    // Propagate to children
    for (size_t i = 0; i < widget->child_count; i++) {
        widget_connect_systems(widget->children[i], events, store);
    }
}

void widget_set_state(Widget* widget, WidgetState state, bool enabled) {
    if (!widget) {
        return;
    }
    
    uint32_t old_flags = widget->state_flags;
    
    if (enabled) {
        widget->state_flags |= state;
    } else {
        widget->state_flags &= ~state;
    }
    
    if (old_flags != widget->state_flags) {
        widget_invalidate(widget);
    }
}

bool widget_has_state(Widget* widget, WidgetState state) {
    return widget && (widget->state_flags & state) != 0;
}

void widget_set_visible(Widget* widget, bool visible) {
    widget_set_state(widget, WIDGET_STATE_HIDDEN, !visible);
}

bool widget_is_visible(Widget* widget) {
    return widget && !widget_has_state(widget, WIDGET_STATE_HIDDEN);
}

void widget_set_enabled(Widget* widget, bool enabled) {
    widget_set_state(widget, WIDGET_STATE_DISABLED, !enabled);
}

bool widget_is_enabled(Widget* widget) {
    return widget && !widget_has_state(widget, WIDGET_STATE_DISABLED);
}

void widget_set_bounds(Widget* widget, int x, int y, int width, int height) {
    if (!widget) {
        return;
    }
    
    widget->bounds.x = x;
    widget->bounds.y = y;
    widget->bounds.w = width;
    widget->bounds.h = height;
    
    // Update relative bounds if has parent
    if (widget->parent) {
        widget->relative_bounds.x = x - widget->parent->bounds.x;
        widget->relative_bounds.y = y - widget->parent->bounds.y;
        widget->relative_bounds.w = width;
        widget->relative_bounds.h = height;
    } else {
        widget->relative_bounds = widget->bounds;
    }
    
    widget_invalidate_layout(widget);
}

void widget_set_relative_bounds(Widget* widget, int x, int y, int width, int height) {
    if (!widget) {
        return;
    }
    
    widget->relative_bounds.x = x;
    widget->relative_bounds.y = y;
    widget->relative_bounds.w = width;
    widget->relative_bounds.h = height;
    
    // Update absolute bounds
    if (widget->parent) {
        widget->bounds.x = widget->parent->bounds.x + x;
        widget->bounds.y = widget->parent->bounds.y + y;
        widget->bounds.w = width;
        widget->bounds.h = height;
    } else {
        widget->bounds = widget->relative_bounds;
    }
    
    widget_invalidate_layout(widget);
}

void widget_invalidate_layout(Widget* widget) {
    if (!widget) {
        return;
    }
    
    widget->needs_layout = true;
    widget_invalidate(widget);
}

void widget_perform_layout(Widget* widget) {
    if (!widget || !widget->needs_layout) {
        return;
    }
    
    if (widget->layout) {
        widget->layout(widget);
    }
    
    widget->needs_layout = false;
    
    // Layout children
    for (size_t i = 0; i < widget->child_count; i++) {
        widget_perform_layout(widget->children[i]);
    }
}

void widget_update_child_bounds(Widget* parent) {
    if (!parent) {
        return;
    }
    
    // Update absolute bounds of all children based on relative bounds
    for (size_t i = 0; i < parent->child_count; i++) {
        Widget* child = parent->children[i];
        if (child) {
            // Update child's absolute position based on parent's current position
            child->bounds.x = parent->bounds.x + child->relative_bounds.x;
            child->bounds.y = parent->bounds.y + child->relative_bounds.y;
            // Width and height remain the same
            child->bounds.w = child->relative_bounds.w;
            child->bounds.h = child->relative_bounds.h;
            
            // Recursively update grandchildren
            widget_update_child_bounds(child);
        }
    }
}

void widget_render(Widget* widget, SDL_Renderer* renderer) {
    if (!widget || !renderer || !widget_is_visible(widget)) {
        return;
    }
    
    // Perform layout if needed
    if (widget->needs_layout) {
        widget_perform_layout(widget);
    }
    
    // Render this widget
    if (widget->render) {
        widget->render(widget, renderer);
    }
    
    // Render children
    for (size_t i = 0; i < widget->child_count; i++) {
        widget_render(widget->children[i], renderer);
    }
    
    // Clear dirty flag
    widget_set_state(widget, WIDGET_STATE_DIRTY, false);
}

void widget_invalidate(Widget* widget) {
    if (!widget) {
        return;
    }
    
    widget_set_state(widget, WIDGET_STATE_DIRTY, true);
    
    // Propagate up to parent
    if (widget->parent) {
        widget_invalidate(widget->parent);
    }
}

void widget_handle_event(Widget* widget, const SDL_Event* event) {
    if (!widget || !event || !widget_is_enabled(widget)) {
        return;
    }
    
    // Debug logging for button events
    if ((event->type == SDL_MOUSEBUTTONUP || event->type == SDL_MOUSEBUTTONDOWN) && 
        widget->type == WIDGET_TYPE_BUTTON) {
        log_debug("widget_handle_event: %s widget='%s' at (%d,%d,%dx%d) visible=%d",
                 event->type == SDL_MOUSEBUTTONUP ? "MOUSEUP" : "MOUSEDOWN",
                 widget->id, widget->bounds.x, widget->bounds.y, 
                 widget->bounds.w, widget->bounds.h, widget_is_visible(widget));
    }
    
    // Let widget handle event first
    if (widget->handle_event) {
        widget->handle_event(widget, event);
    }
    
    // Then propagate to children
    for (size_t i = 0; i < widget->child_count; i++) {
        widget_handle_event(widget->children[i], event);
    }
}

bool widget_contains_point(Widget* widget, int x, int y) {
    if (!widget || !widget_is_visible(widget)) {
        return false;
    }
    
    return x >= widget->bounds.x && 
           x < widget->bounds.x + widget->bounds.w &&
           y >= widget->bounds.y && 
           y < widget->bounds.y + widget->bounds.h;
}

Widget* widget_hit_test(Widget* root, int x, int y) {
    if (!root || !widget_is_visible(root)) {
        return NULL;
    }
    
    // First check if this widget contains the point
    if (!widget_contains_point(root, x, y)) {
        return NULL;
    }
    
    // Check children (front to back) for a better hit
    Widget* child_hit = NULL;
    for (int i = root->child_count - 1; i >= 0; i--) {
        Widget* hit = widget_hit_test(root->children[i], x, y);
        if (hit) {
            child_hit = hit;
            break;
        }
    }
    
    // Check if this widget is truly interactive (not just a container with scroll handling)
    bool is_interactive = (root->type == WIDGET_TYPE_BUTTON);
    
    // If we're interactive, return this widget unless child is also interactive
    if (is_interactive && root->handle_event) {
        if (!child_hit || child_hit->type != WIDGET_TYPE_BUTTON) {
            return root;
        }
    }
    
    // Return the child hit if we found one, otherwise this widget
    if (child_hit) {
        return child_hit;
    }
    
    // This widget contains the point and has no child hits
    return root;
}

void widget_update(Widget* widget, double delta_time) {
    if (!widget || !widget_is_enabled(widget)) {
        return;
    }
    
    // Update this widget
    if (widget->update) {
        widget->update(widget, delta_time);
    }
    
    // Update children
    for (size_t i = 0; i < widget->child_count; i++) {
        widget_update(widget->children[i], delta_time);
    }
}

// Default implementations

void widget_default_render(Widget* widget, SDL_Renderer* renderer) {
    if (!widget || !renderer) {
        return;
    }
    
    log_debug("DEFAULT RENDER: %s at (%d,%d,%dx%d) bg(%d,%d,%d)", 
              widget->id, widget->bounds.x, widget->bounds.y,
              widget->bounds.w, widget->bounds.h,
              widget->background_color.r, widget->background_color.g, widget->background_color.b);
    
    // Draw background
    SDL_SetRenderDrawColor(renderer, 
                          widget->background_color.r,
                          widget->background_color.g,
                          widget->background_color.b,
                          widget->background_color.a);
    SDL_RenderFillRect(renderer, &widget->bounds);
    
    // Draw border
    if (widget->border_width > 0) {
        SDL_SetRenderDrawColor(renderer,
                              widget->border_color.r,
                              widget->border_color.g,
                              widget->border_color.b,
                              widget->border_color.a);
        
        for (int i = 0; i < widget->border_width; i++) {
            SDL_Rect border_rect = {
                widget->bounds.x + i,
                widget->bounds.y + i,
                widget->bounds.w - i * 2,
                widget->bounds.h - i * 2
            };
            SDL_RenderDrawRect(renderer, &border_rect);
        }
    }
    
    // Render children
    for (size_t i = 0; i < widget->child_count; i++) {
        Widget* child = widget->children[i];
        if (child && child->render && !(child->state_flags & WIDGET_STATE_HIDDEN)) {
            child->render(child, renderer);
        }
    }
}

void widget_default_handle_event(Widget* widget, const SDL_Event* event) {
    if (!widget || !event) {
        return;
    }
    
    switch (event->type) {
        case SDL_MOUSEMOTION: {
            bool was_hovered = widget_has_state(widget, WIDGET_STATE_HOVERED);
            bool is_hovered = widget_contains_point(widget, event->motion.x, event->motion.y);
            
            if (was_hovered != is_hovered) {
                widget_set_state(widget, WIDGET_STATE_HOVERED, is_hovered);
            }
            break;
        }
        
        case SDL_MOUSEBUTTONDOWN: {
            if (event->button.button == SDL_BUTTON_LEFT &&
                widget_contains_point(widget, event->button.x, event->button.y)) {
                widget_set_state(widget, WIDGET_STATE_PRESSED, true);
            }
            break;
        }
        
        case SDL_MOUSEBUTTONUP: {
            if (event->button.button == SDL_BUTTON_LEFT) {
                widget_set_state(widget, WIDGET_STATE_PRESSED, false);
            }
            break;
        }
    }
}

void widget_default_layout(Widget* widget) {
    if (!widget) {
        return;
    }
    
    // Simple vertical stack layout for children
    int y_offset = widget->padding;
    
    for (size_t i = 0; i < widget->child_count; i++) {
        Widget* child = widget->children[i];
        if (!widget_is_visible(child)) {
            continue;
        }
        
        // Position child
        widget_set_relative_bounds(child,
                                 widget->padding,
                                 y_offset,
                                 widget->bounds.w - widget->padding * 2,
                                 child->relative_bounds.h);
        
        y_offset += child->bounds.h + widget->padding;
    }
}

void widget_default_destroy(Widget* widget) {
    // Base cleanup is handled by widget_destroy
    // This is for type-specific cleanup
    // Currently no default cleanup needed
    (void)widget; // Suppress unused parameter warning
}