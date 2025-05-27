#include "button_widget.h"
#include "../../events/event_system.h"
#include "../../events/event_types.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "core/logger.h"
#include "core/memory_patterns.h"  // Memory ownership patterns

// Forward declarations for virtual functions
static void button_widget_render(Widget* widget, SDL_Renderer* renderer);
static void button_widget_handle_event(Widget* widget, const SDL_Event* event);
static void button_widget_destroy(Widget* widget);

ButtonWidget* button_widget_create(const char* id) {
    ButtonWidget* button = calloc(1, sizeof(ButtonWidget));
    if (!button) {
        log_error("Failed to allocate button widget");
        return NULL;
    }
    
    // Initialize base widget
    Widget* base = &button->base;
    strncpy(base->id, id, sizeof(base->id) - 1);
    base->type = WIDGET_TYPE_BUTTON;
    
    // Set virtual functions
    base->render = button_widget_render;
    base->handle_event = button_widget_handle_event;
    base->destroy = button_widget_destroy;
    
    // Initialize widget arrays - buttons can now have children
    base->child_capacity = 2;  // Typically text, but could be icon + text
    base->children = calloc(base->child_capacity, sizeof(Widget*));
    if (!base->children) {
        free(button);
        return NULL;
    }
    
    base->event_capacity = 2;
    base->subscribed_events = calloc(base->event_capacity, sizeof(char*));
    if (!base->subscribed_events) {
        free(button);
        return NULL;
    }
    
    // Set default colors
    button->normal_color = (SDL_Color){100, 100, 100, 255};
    button->hover_color = (SDL_Color){120, 120, 120, 255};
    button->pressed_color = (SDL_Color){80, 80, 80, 255};
    button->disabled_color = (SDL_Color){180, 180, 180, 255};
    
    base->background_color = button->normal_color;
    base->border_color = (SDL_Color){50, 50, 50, 255};
    base->border_width = 2;
    base->padding = 10;
    
    // Set default size
    base->bounds.w = 120;
    base->bounds.h = 40;
    
    log_info("Created button widget '%s'", id);
    return button;
}

void button_widget_set_colors(ButtonWidget* button,
                            SDL_Color normal,
                            SDL_Color hover,
                            SDL_Color pressed,
                            SDL_Color disabled) {
    if (!button) {
        return;
    }
    
    button->normal_color = normal;
    button->hover_color = hover;
    button->pressed_color = pressed;
    button->disabled_color = disabled;
    
    // Update current background color based on state
    Widget* base = &button->base;
    if (widget_has_state(base, WIDGET_STATE_DISABLED)) {
        base->background_color = disabled;
    } else if (widget_has_state(base, WIDGET_STATE_PRESSED)) {
        base->background_color = pressed;
    } else if (widget_has_state(base, WIDGET_STATE_HOVERED)) {
        base->background_color = hover;
    } else {
        base->background_color = normal;
    }
    
    widget_invalidate(base);
}

void button_widget_set_click_callback(ButtonWidget* button,
                                    button_click_callback callback,
                                    void* user_data) {
    if (!button) {
        return;
    }
    
    button->on_click = callback;
    button->user_data = user_data;
}

/**
 * Set event to publish when button is clicked
 * @param button Button widget (borrows reference)
 * @param event_name Event name to publish (copies string)
 * @param data Event data to publish (copies data)
 * @param data_size Size of event data
 * 
 * Memory: Pattern 3 - Callee Owns Copy
 * The button makes copies of both event_name and data, which it
 * owns and will free in button_widget_destroy()
 */
void button_widget_set_publish_event(ButtonWidget* button,
                                   const char* event_name,
                                   void* data,
                                   size_t data_size) {
    if (!button) {
        return;
    }
    
    // Free old event name and data
    if (button->publish_event) {
        free(button->publish_event);
        button->publish_event = NULL;
    }
    if (button->publish_data) {
        free(button->publish_data);
        button->publish_data = NULL;
    }
    
    if (event_name) {
        button->publish_event = PK_STRDUP(event_name);
        
        if (data && data_size > 0) {
            button->publish_data = malloc(data_size);
            if (button->publish_data) {
                memcpy(button->publish_data, data, data_size);
                button->publish_data_size = data_size;
            }
        }
    }
}

void button_widget_click(ButtonWidget* button) {
    if (!button || !widget_is_enabled(&button->base)) {
        return;
    }
    
    log_debug("Button '%s' clicked", button->base.id);
    
    // Call callback
    if (button->on_click) {
        button->on_click(button, button->user_data);
    }
    
    // Publish event
    if (button->publish_event && button->base.event_system) {
        // Debug: print the actual data being published
        if (button->publish_data && button->publish_data_size >= sizeof(ButtonEventData)) {
            ButtonEventData* data = (ButtonEventData*)button->publish_data;
            
            // Update timestamp
            data->timestamp = SDL_GetTicks();
            
            log_debug("Button '%s' publishing: page=%d button=%d text='%s'", 
                     button->base.id, data->page, data->button_index, data->button_text);
        }
        
        event_publish(button->base.event_system,
                     button->publish_event,
                     button->publish_data,
                     button->publish_data_size);
        
        log_debug("Button published event '%s'", button->publish_event);
    }
}

static void button_widget_render(Widget* widget, SDL_Renderer* renderer) {
    ButtonWidget* button = (ButtonWidget*)widget;
    if (!button || !renderer) {
        return;
    }
    
    log_debug("BUTTON RENDER: %s at (%d,%d) size %dx%d color (%d,%d,%d) children=%zu", 
              button->base.id, widget->bounds.x, widget->bounds.y, 
              widget->bounds.w, widget->bounds.h,
              widget->background_color.r, widget->background_color.g, widget->background_color.b,
              widget->child_count);
    
    // Debug: print child positions
    for (size_t i = 0; i < widget->child_count; i++) {
        Widget* child = widget->children[i];
        if (child) {
            log_debug("  Child %zu: %s at (%d,%d) size %dx%d", 
                     i, child->id, child->bounds.x, child->bounds.y,
                     child->bounds.w, child->bounds.h);
        }
    }
    
    // Update background color based on state
    if (widget_has_state(widget, WIDGET_STATE_DISABLED)) {
        widget->background_color = button->disabled_color;
    } else if (widget_has_state(widget, WIDGET_STATE_PRESSED)) {
        widget->background_color = button->pressed_color;
    } else if (widget_has_state(widget, WIDGET_STATE_HOVERED)) {
        widget->background_color = button->hover_color;
    } else {
        widget->background_color = button->normal_color;
    }
    
    // Call base render for background and border
    widget_default_render(widget, renderer);
    
    // Base render will handle drawing children (text widgets, etc)
    
    // Draw focus indicator
    if (widget_has_state(widget, WIDGET_STATE_FOCUSED)) {
        SDL_SetRenderDrawColor(renderer, 0, 120, 255, 255);
        SDL_Rect focus_rect = {
            widget->bounds.x - 2,
            widget->bounds.y - 2,
            widget->bounds.w + 4,
            widget->bounds.h + 4
        };
        SDL_RenderDrawRect(renderer, &focus_rect);
    }
}

static void button_widget_handle_event(Widget* widget, const SDL_Event* event) {
    ButtonWidget* button = (ButtonWidget*)widget;
    if (!button) {
        return;
    }
    
    // Debug all button events
    if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) {
        log_debug("button_widget_handle_event: %s '%s' at (%d,%d) state_flags=0x%x",
                 event->type == SDL_MOUSEBUTTONDOWN ? "MOUSEDOWN" : "MOUSEUP",
                 widget->id, event->button.x, event->button.y, widget->state_flags);
    }
    
    // Handle click BEFORE calling default handler (which clears PRESSED state)
    if (event->type == SDL_MOUSEBUTTONUP && 
        event->button.button == SDL_BUTTON_LEFT) {
        
        bool has_pressed = widget_has_state(widget, WIDGET_STATE_PRESSED);
        bool contains_point = widget_contains_point(widget, event->button.x, event->button.y);
        
        if (has_pressed || contains_point) {
            log_debug("Button '%s' MOUSEUP: pressed=%d contains=%d at (%d,%d) bounds=(%d,%d,%dx%d)",
                     widget->id, has_pressed, contains_point, 
                     event->button.x, event->button.y,
                     widget->bounds.x, widget->bounds.y, widget->bounds.w, widget->bounds.h);
        }
        
        if (has_pressed && contains_point) {
            button_widget_click(button);
        }
    }
    
    // Call base handler for hover/press states
    widget_default_handle_event(widget, event);
    
    // Handle keyboard activation
    if (event->type == SDL_KEYDOWN &&
        widget_has_state(widget, WIDGET_STATE_FOCUSED)) {
        
        if (event->key.keysym.sym == SDLK_SPACE ||
            event->key.keysym.sym == SDLK_RETURN) {
            button_widget_click(button);
        }
    }
}

/**
 * Destroy button widget and free owned memory
 * Memory: Pattern 1 - Parent Owns Child
 * Frees the event name and data that were copied in set_publish_event
 */
static void button_widget_destroy(Widget* widget) {
    ButtonWidget* button = (ButtonWidget*)widget;
    if (!button) {
        return;
    }
    
    // Free owned event data (Pattern 3 - we own the copies)
    PK_FREE(button->publish_event);
    PK_FREE(button->publish_data);
    
    // Base cleanup handles the rest (children, subscriptions, etc.)
}