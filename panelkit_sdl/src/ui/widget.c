#include "widget.h"
#include "../events/event_system.h"
#include "../state/state_store.h"
#include "style/style_core.h"
#include "style/style_constants.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "core/logger.h"
#include "core/error.h"

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
    PK_CHECK_NULL(id != NULL, PK_ERROR_NULL_PARAM);
    
    Widget* widget = calloc(1, sizeof(Widget));
    if (!widget) {
        log_error("Failed to allocate widget for '%s'", id);
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate Widget struct for '%s'", id);
        return NULL;
    }
    
    // Set identity
    strncpy(widget->id, id, sizeof(widget->id) - 1);
    widget->type = type;
    
    // Initialize arrays with proper cleanup on failure
    widget->child_capacity = INITIAL_CHILD_CAPACITY;
    widget->children = calloc(widget->child_capacity, sizeof(Widget*));
    if (!widget->children) {
        log_error("Failed to allocate children array for widget '%s'", id);
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate %zu bytes for children array",
                                       widget->child_capacity * sizeof(Widget*));
        // Cleanup partially created widget
        free(widget);
        return NULL;
    }
    
    widget->event_capacity = INITIAL_EVENT_CAPACITY;
    widget->subscribed_events = calloc(widget->event_capacity, sizeof(char*));
    if (!widget->subscribed_events) {
        log_error("Failed to allocate event array for widget '%s'", id);
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate %zu bytes for event array",
                                       widget->event_capacity * sizeof(char*));
        // Cleanup partially created widget
        free(widget->children);
        free(widget);
        return NULL;
    }
    
    // Style system initialization
    widget->style = NULL;
    widget->style_owned = false;
    widget->active_style = NULL;
    
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
    
    // Clean up style if owned
    if (widget->style_owned && widget->style) {
        style_destroy(widget->style);
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
    PK_CHECK_FALSE(parent != NULL && child != NULL, PK_ERROR_NULL_PARAM);
    
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
            pk_set_last_error(PK_ERROR_OUT_OF_MEMORY);
            return false;
        }
        parent->children = new_children;
        parent->child_capacity = new_capacity;
    }
    
    parent->children[parent->child_count++] = child;
    child->parent = parent;
    
    // Inherit event system and state store
    if (parent->event_system || parent->state_store) {
        PkError err = widget_connect_systems(child, parent->event_system, parent->state_store);
        if (err != PK_OK) {
            // Rollback: Remove child from parent on connection failure
            parent->child_count--;
            child->parent = NULL;
            
            log_error("Failed to connect child '%s' to systems: %s", 
                     child->id, pk_error_string(err));
            pk_set_last_error_with_context(err,
                                          "Failed to connect child '%s' to parent '%s' systems",
                                          child->id, parent->id);
            return false;
        }
    }
    
    widget_invalidate_layout(parent);
    log_debug("Added child '%s' to parent '%s'", child->id, parent->id);
    return true;
}

bool widget_remove_child(Widget* parent, Widget* child) {
    PK_CHECK_FALSE_WITH_CONTEXT(parent != NULL, PK_ERROR_NULL_PARAM, 
                                "parent is NULL");
    PK_CHECK_FALSE_WITH_CONTEXT(child != NULL, PK_ERROR_NULL_PARAM,
                                "child is NULL");
    
    if (child->parent != parent) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "child '%s' is not a child of parent '%s'",
                                       child->id, parent->id);
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
    
    pk_set_last_error_with_context(PK_ERROR_NOT_FOUND,
                                   "child '%s' not found in parent '%s'",
                                   child->id, parent->id);
    return false;
}

Widget* widget_find_child(Widget* parent, const char* id) {
    PK_CHECK_NULL_WITH_CONTEXT(parent != NULL, PK_ERROR_NULL_PARAM,
                               "parent is NULL");
    PK_CHECK_NULL_WITH_CONTEXT(id != NULL, PK_ERROR_NULL_PARAM,
                               "id is NULL");
    
    for (size_t i = 0; i < parent->child_count; i++) {
        if (strcmp(parent->children[i]->id, id) == 0) {
            return parent->children[i];
        }
    }
    
    // Not an error - widget simply not found
    return NULL;
}

Widget* widget_find_descendant(Widget* root, const char* id) {
    PK_CHECK_NULL_WITH_CONTEXT(root != NULL, PK_ERROR_NULL_PARAM,
                               "root is NULL");
    PK_CHECK_NULL_WITH_CONTEXT(id != NULL, PK_ERROR_NULL_PARAM,
                               "id is NULL");
    
    if (strcmp(root->id, id) == 0) {
        return root;
    }
    
    for (size_t i = 0; i < root->child_count; i++) {
        Widget* found = widget_find_descendant(root->children[i], id);
        if (found) {
            return found;
        }
    }
    
    // Not an error - widget simply not found
    return NULL;
}

bool widget_subscribe_event(Widget* widget, const char* event_name) {
    PK_CHECK_FALSE_WITH_CONTEXT(widget != NULL, PK_ERROR_NULL_PARAM,
                                "widget is NULL");
    PK_CHECK_FALSE_WITH_CONTEXT(event_name != NULL, PK_ERROR_NULL_PARAM,
                                "event_name is NULL");
    
    // Check if already subscribed
    for (size_t i = 0; i < widget->event_count; i++) {
        if (strcmp(widget->subscribed_events[i], event_name) == 0) {
            return true;  // Already subscribed - not an error
        }
    }
    
    // Grow array if needed
    if (widget->event_count >= widget->event_capacity) {
        size_t new_capacity = widget->event_capacity * 2;
        char** new_events = realloc(widget->subscribed_events,
                                   new_capacity * sizeof(char*));
        if (!new_events) {
            log_error("Failed to grow events array for widget '%s'", widget->id);
            pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                           "Failed to resize events array from %zu to %zu",
                                           widget->event_capacity, new_capacity);
            return false;
        }
        widget->subscribed_events = new_events;
        widget->event_capacity = new_capacity;
    }
    
    // Store event name
    widget->subscribed_events[widget->event_count] = strdup(event_name);
    if (!widget->subscribed_events[widget->event_count]) {
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to duplicate event name '%s'",
                                       event_name);
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
            pk_set_last_error_with_context(PK_ERROR_EVENT_NOT_FOUND,
                                           "Failed to subscribe widget '%s' to event '%s'",
                                           widget->id, event_name);
            return false;
        }
    }
    
    log_debug("Widget '%s' subscribed to event '%s'", widget->id, event_name);
    return true;
}

bool widget_unsubscribe_event(Widget* widget, const char* event_name) {
    PK_CHECK_FALSE_WITH_CONTEXT(widget != NULL, PK_ERROR_NULL_PARAM,
                                "widget is NULL");
    PK_CHECK_FALSE_WITH_CONTEXT(event_name != NULL, PK_ERROR_NULL_PARAM,
                                "event_name is NULL");
    
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
    
    pk_set_last_error_with_context(PK_ERROR_NOT_FOUND,
                                   "Widget '%s' not subscribed to event '%s'",
                                   widget->id, event_name);
    return false;
}

PkError widget_connect_systems(Widget* widget, EventSystem* events, StateStore* store) {
    PK_CHECK_RETURN(widget != NULL, PK_ERROR_NULL_PARAM);
    
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
            if (!event_subscribe(events, widget->subscribed_events[i],
                               widget_event_handler_callback, widget)) {
                // Try to recover by rolling back subscriptions
                log_error("Failed to subscribe widget '%s' to event '%s'",
                         widget->id, widget->subscribed_events[i]);
                
                // Unsubscribe what we've done so far
                for (size_t j = 0; j < i; j++) {
                    event_unsubscribe(events, widget->subscribed_events[j],
                                    widget_event_handler_callback);
                }
                
                // Clear the system references
                widget->event_system = NULL;
                widget->state_store = NULL;
                
                return pk_get_last_error();  // Propagate error from event_subscribe
            }
        }
    }
    
    // Propagate to children
    for (size_t i = 0; i < widget->child_count; i++) {
        PkError err = widget_connect_systems(widget->children[i], events, store);
        if (err != PK_OK) {
            pk_set_last_error_with_context(err,
                                          "Failed to connect child '%s' of parent '%s'",
                                          widget->children[i]->id, widget->id);
            return err;
        }
    }
    
    return PK_OK;
}

void widget_set_state(Widget* widget, WidgetState state, bool enabled) {
    if (!widget) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "widget is NULL in widget_set_state");
        return;
    }
    
    uint32_t old_flags = widget->state_flags;
    
    if (enabled) {
        widget->state_flags |= state;
    } else {
        widget->state_flags &= ~state;
    }
    
    if (old_flags != widget->state_flags) {
        widget_update_active_style(widget);
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
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "widget is NULL in widget_set_bounds");
        return;
    }
    
    if (width < 0 || height < 0) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Invalid bounds: width=%d, height=%d",
                                       width, height);
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
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "widget is NULL in widget_set_relative_bounds");
        return;
    }
    
    if (width < 0 || height < 0) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Invalid relative bounds: width=%d, height=%d",
                                       width, height);
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
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "widget is NULL in widget_invalidate_layout");
        return;
    }
    
    widget->needs_layout = true;
    widget_invalidate(widget);
}

void widget_perform_layout(Widget* widget) {
    if (!widget) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "widget is NULL in widget_perform_layout");
        return;
    }
    
    if (!widget->needs_layout) {
        return;  // Not an error - no layout needed
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
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "parent is NULL in widget_update_child_bounds");
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

PkError widget_render(Widget* widget, SDL_Renderer* renderer) {
    PK_CHECK_ERROR_WITH_CONTEXT(widget != NULL, PK_ERROR_NULL_PARAM,
                               "widget is NULL in widget_render");
    PK_CHECK_ERROR_WITH_CONTEXT(renderer != NULL, PK_ERROR_NULL_PARAM,
                               "renderer is NULL in widget_render");
    
    if (!widget_is_visible(widget)) {
        return PK_OK;  // Not an error - widget is hidden
    }
    
    // Perform layout if needed
    if (widget->needs_layout) {
        widget_perform_layout(widget);
    }
    
    // Render this widget
    if (widget->render) {
        PkError err = widget->render(widget, renderer);
        if (err != PK_OK) {
            pk_set_last_error_with_context(err,
                                           "Failed to render widget '%s' of type %d",
                                           widget->id, widget->type);
            return err;
        }
    }
    
    // Render children
    for (size_t i = 0; i < widget->child_count; i++) {
        PkError err = widget_render(widget->children[i], renderer);
        if (err != PK_OK) {
            // Context already set by recursive call
            return err;
        }
    }
    
    // Clear dirty flag
    widget_set_state(widget, WIDGET_STATE_DIRTY, false);
    
    return PK_OK;
}

void widget_invalidate(Widget* widget) {
    if (!widget) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "widget is NULL in widget_invalidate");
        return;
    }
    
    widget_set_state(widget, WIDGET_STATE_DIRTY, true);
    
    // Propagate up to parent
    if (widget->parent) {
        widget_invalidate(widget->parent);
    }
}

void widget_handle_event(Widget* widget, const SDL_Event* event) {
    if (!widget) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "widget is NULL in widget_handle_event");
        return;
    }
    
    if (!event) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "event is NULL in widget_handle_event");
        return;
    }
    
    if (!widget_is_enabled(widget)) {
        return;  // Not an error - widget is disabled
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
    if (!root) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "root is NULL in widget_hit_test");
        return NULL;
    }
    
    if (!widget_is_visible(root)) {
        return NULL;  // Not an error - widget is hidden
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
    if (!widget) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "widget is NULL in widget_update");
        return;
    }
    
    if (!widget_is_enabled(widget)) {
        return;  // Not an error - widget is disabled
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

// Style management functions

void widget_set_style_ref(Widget* widget, Style* style) {
    if (!widget) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "widget is NULL in widget_set_style_ref");
        return;
    }
    
    // Clean up old style if owned
    if (widget->style_owned && widget->style) {
        style_destroy(widget->style);
    }
    
    widget->style = style;
    widget->style_owned = false;
    widget_update_active_style(widget);
    widget_invalidate(widget);
}

void widget_set_style_owned(Widget* widget, Style* style) {
    if (!widget) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "widget is NULL in widget_set_style_owned");
        return;
    }
    
    // Clean up old style if owned
    if (widget->style_owned && widget->style) {
        style_destroy(widget->style);
    }
    
    widget->style = style;
    widget->style_owned = true;
    widget_update_active_style(widget);
    widget_invalidate(widget);
}

void widget_update_active_style(Widget* widget) {
    if (!widget) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "widget is NULL in widget_update_active_style");
        return;
    }
    
    if (!widget->style) {
        widget->active_style = NULL;
        return;
    }
    
    // Resolve style based on current state
    widget->active_style = style_resolve_state(widget->style, widget->state_flags);
}

const StyleBase* widget_get_active_style(Widget* widget) {
    if (!widget) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "widget is NULL in widget_get_active_style");
        return NULL;
    }
    
    return widget->active_style;
}

// Default implementations

PkError widget_default_render(Widget* widget, SDL_Renderer* renderer) {
    PK_CHECK_ERROR_WITH_CONTEXT(widget != NULL, PK_ERROR_NULL_PARAM,
                               "widget is NULL in widget_default_render");
    PK_CHECK_ERROR_WITH_CONTEXT(renderer != NULL, PK_ERROR_NULL_PARAM,
                               "renderer is NULL in widget_default_render");
    
    // Get active style or use default
    const StyleBase* style = widget->active_style;
    if (!style) {
        // Use a default style if none set
        static StyleBase default_style;
        static bool default_initialized = false;
        if (!default_initialized) {
            Style* panel_style = style_create_text();
            if (panel_style) {
                default_style = panel_style->base;
                style_destroy(panel_style);
            } else {
                // Fallback to hardcoded defaults
                default_style.background = (PkColor){33, 33, 33, 255};
                default_style.foreground = (PkColor){255, 255, 255, 255};
                default_style.border.color = (PkColor){100, 100, 100, 255};
                default_style.border.width = 1;
                default_style.padding = (Spacing){5, 5, 5, 5};
            }
            default_initialized = true;
        }
        style = &default_style;
    }
    
    log_debug("DEFAULT RENDER: %s at (%d,%d,%dx%d) bg(%d,%d,%d)", 
              widget->id, widget->bounds.x, widget->bounds.y,
              widget->bounds.w, widget->bounds.h,
              style->background.r, style->background.g, style->background.b);
    
    // Draw background
    if (SDL_SetRenderDrawColor(renderer, 
                              style->background.r,
                              style->background.g,
                              style->background.b,
                              style->background.a) < 0) {
        pk_set_last_error_with_context(PK_ERROR_RENDER_FAILED,
                                       "SDL_SetRenderDrawColor failed: %s",
                                       SDL_GetError());
        return PK_ERROR_RENDER_FAILED;
    }
    
    if (SDL_RenderFillRect(renderer, &widget->bounds) < 0) {
        pk_set_last_error_with_context(PK_ERROR_RENDER_FAILED,
                                       "SDL_RenderFillRect failed for widget '%s': %s",
                                       widget->id, SDL_GetError());
        return PK_ERROR_RENDER_FAILED;
    }
    
    // Draw border
    if (style->border.width > 0) {
        if (SDL_SetRenderDrawColor(renderer,
                                  style->border.color.r,
                                  style->border.color.g,
                                  style->border.color.b,
                                  style->border.color.a) < 0) {
            pk_set_last_error_with_context(PK_ERROR_RENDER_FAILED,
                                           "SDL_SetRenderDrawColor failed for border: %s",
                                           SDL_GetError());
            return PK_ERROR_RENDER_FAILED;
        }
        
        for (int i = 0; i < style->border.width; i++) {
            SDL_Rect border_rect = {
                widget->bounds.x + i,
                widget->bounds.y + i,
                widget->bounds.w - i * 2,
                widget->bounds.h - i * 2
            };
            if (SDL_RenderDrawRect(renderer, &border_rect) < 0) {
                pk_set_last_error_with_context(PK_ERROR_RENDER_FAILED,
                                               "SDL_RenderDrawRect failed: %s",
                                               SDL_GetError());
                return PK_ERROR_RENDER_FAILED;
            }
        }
    }
    
    // Render children
    for (size_t i = 0; i < widget->child_count; i++) {
        Widget* child = widget->children[i];
        if (child && child->render && !(child->state_flags & WIDGET_STATE_HIDDEN)) {
            PkError err = child->render(child, renderer);
            if (err != PK_OK) {
                // Context already set by child
                return err;
            }
        }
    }
    
    return PK_OK;
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
    
    // Get active style for padding
    const StyleBase* style = widget->active_style;
    int padding = 5; // Default padding
    if (style) {
        padding = style->padding.top; // Use top padding as general padding
    }
    
    // Simple vertical stack layout for children
    int y_offset = padding;
    
    for (size_t i = 0; i < widget->child_count; i++) {
        Widget* child = widget->children[i];
        if (!widget_is_visible(child)) {
            continue;
        }
        
        // Position child
        widget_set_relative_bounds(child,
                                 padding,
                                 y_offset,
                                 widget->bounds.w - padding * 2,
                                 child->relative_bounds.h);
        
        y_offset += child->bounds.h + padding;
    }
}

void widget_default_destroy(Widget* widget) {
    // Base cleanup is handled by widget_destroy
    // This is for type-specific cleanup
    // Currently no default cleanup needed
    (void)widget; // Suppress unused parameter warning
}