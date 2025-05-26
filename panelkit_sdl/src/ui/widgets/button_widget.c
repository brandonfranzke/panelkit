#include "button_widget.h"
#include "../../events/event_system.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Simple logging macros
#ifndef log_info
#define log_info(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define log_error(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#define log_debug(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#endif

// Forward declarations for virtual functions
static void button_widget_render(Widget* widget, SDL_Renderer* renderer);
static void button_widget_handle_event(Widget* widget, const SDL_Event* event);
static void button_widget_destroy(Widget* widget);

ButtonWidget* button_widget_create(const char* id, const char* label) {
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
    
    // Initialize widget arrays (required by base widget)
    base->child_capacity = 0;  // Button has no children
    base->children = NULL;
    
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
    button->text_color = (SDL_Color){255, 255, 255, 255};
    
    base->background_color = button->normal_color;
    base->border_color = (SDL_Color){50, 50, 50, 255};
    base->border_width = 2;
    base->padding = 10;
    
    // Set default size
    base->bounds.w = 120;
    base->bounds.h = 40;
    
    // Set label
    strncpy(button->label, label, sizeof(button->label) - 1);
    
    log_info("Created button widget '%s' with label '%s'", id, label);
    return button;
}

void button_widget_set_label(ButtonWidget* button, const char* label) {
    if (!button || !label) {
        return;
    }
    
    strncpy(button->label, label, sizeof(button->label) - 1);
    widget_invalidate(&button->base);
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

void button_widget_set_text_color(ButtonWidget* button, SDL_Color color) {
    if (!button) {
        return;
    }
    
    button->text_color = color;
    widget_invalidate(&button->base);
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
        button->publish_event = strdup(event_name);
        
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
    
    // Draw label indicator (centered rectangle)
    if (strlen(button->label) > 0) {
        SDL_Rect label_rect = {
            widget->bounds.x + widget->padding,
            widget->bounds.y + widget->bounds.h / 2 - 10,
            widget->bounds.w - widget->padding * 2,
            20
        };
        
        // Use text color for label indicator
        SDL_SetRenderDrawColor(renderer,
                             button->text_color.r,
                             button->text_color.g,
                             button->text_color.b,
                             button->text_color.a);
        SDL_RenderDrawRect(renderer, &label_rect);
        
        // Draw simple text length indicator
        int text_width = (int)(strlen(button->label) * 6);  // Rough estimate
        if (text_width > label_rect.w - 4) {
            text_width = label_rect.w - 4;
        }
        
        SDL_Rect text_indicator = {
            label_rect.x + (label_rect.w - text_width) / 2,
            label_rect.y + 2,
            text_width,
            label_rect.h - 4
        };
        SDL_RenderDrawRect(renderer, &text_indicator);
    }
    
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
    
    // Call base handler for hover/press states
    widget_default_handle_event(widget, event);
    
    // Handle click
    if (event->type == SDL_MOUSEBUTTONUP && 
        event->button.button == SDL_BUTTON_LEFT &&
        widget_has_state(widget, WIDGET_STATE_PRESSED) &&
        widget_contains_point(widget, event->button.x, event->button.y)) {
        
        button_widget_click(button);
    }
    
    // Handle keyboard activation
    if (event->type == SDL_KEYDOWN &&
        widget_has_state(widget, WIDGET_STATE_FOCUSED)) {
        
        if (event->key.keysym.sym == SDLK_SPACE ||
            event->key.keysym.sym == SDLK_RETURN) {
            button_widget_click(button);
        }
    }
}

static void button_widget_destroy(Widget* widget) {
    ButtonWidget* button = (ButtonWidget*)widget;
    if (!button) {
        return;
    }
    
    // Free event data
    if (button->publish_event) {
        free(button->publish_event);
    }
    if (button->publish_data) {
        free(button->publish_data);
    }
    
    // Base cleanup handles the rest
}