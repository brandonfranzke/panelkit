#include "time_widget.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Forward declarations
static void time_widget_update(Widget* widget, double delta_time);
static void time_widget_render(Widget* widget, SDL_Renderer* renderer);
static void time_widget_destroy(Widget* widget);

Widget* time_widget_create(const char* id, const char* format, TTF_Font* font) {
    TimeWidget* time_widget = calloc(1, sizeof(TimeWidget));
    if (!time_widget) return NULL;
    
    // Initialize base widget
    Widget* base = &time_widget->base;
    strncpy(base->id, id, sizeof(base->id) - 1);
    base->type = WIDGET_TYPE_CUSTOM;
    base->state_flags = WIDGET_STATE_NORMAL;
    
    // Set widget methods
    base->update = time_widget_update;
    base->render = time_widget_render;
    base->destroy = time_widget_destroy;
    
    // Initialize arrays
    base->child_capacity = 1;
    base->children = calloc(1, sizeof(Widget*));
    base->event_capacity = 0;
    base->subscribed_events = NULL;
    
    // Set time properties
    strncpy(time_widget->time_format, format ? format : "%H:%M:%S", 
            sizeof(time_widget->time_format) - 1);
    time_widget->show_seconds = true;
    time_widget->last_update = 0;
    
    // Create internal text widget
    char text_id[128];
    snprintf(text_id, sizeof(text_id), "%s_text", id);
    time_widget->text_widget = (TextWidget*)text_widget_create(text_id, "00:00:00", font);
    if (time_widget->text_widget) {
        widget_add_child(base, (Widget*)time_widget->text_widget);
        
        // Copy size from text widget
        base->bounds = time_widget->text_widget->base.bounds;
    }
    
    return (Widget*)time_widget;
}

void time_widget_set_format(Widget* widget, const char* format) {
    if (!widget || !format) return;
    TimeWidget* time_widget = (TimeWidget*)widget;
    
    strncpy(time_widget->time_format, format, sizeof(time_widget->time_format) - 1);
    time_widget->last_update = 0; // Force update
}

void time_widget_set_show_seconds(Widget* widget, bool show_seconds) {
    if (!widget) return;
    TimeWidget* time_widget = (TimeWidget*)widget;
    
    time_widget->show_seconds = show_seconds;
    if (!show_seconds) {
        time_widget_set_format(widget, "%H:%M");
    } else {
        time_widget_set_format(widget, "%H:%M:%S");
    }
}

static void time_widget_update(Widget* widget, double delta_time) {
    if (!widget) return;
    TimeWidget* time_widget = (TimeWidget*)widget;
    
    // Update time every second (or minute if not showing seconds)
    time_t current_time = time(NULL);
    int update_interval = time_widget->show_seconds ? 1 : 60;
    
    if (current_time - time_widget->last_update >= update_interval) {
        // Format current time
        struct tm* tm_info = localtime(&current_time);
        char time_str[64];
        strftime(time_str, sizeof(time_str), time_widget->time_format, tm_info);
        
        // Update text widget
        if (time_widget->text_widget) {
            text_widget_set_text((Widget*)time_widget->text_widget, time_str);
        }
        
        time_widget->last_update = current_time;
    }
}

static void time_widget_render(Widget* widget, SDL_Renderer* renderer) {
    if (!widget || !renderer) return;
    TimeWidget* time_widget = (TimeWidget*)widget;
    
    // Position text widget
    if (time_widget->text_widget) {
        time_widget->text_widget->base.bounds = widget->bounds;
    }
    
    // Render children (text widget)
    for (size_t i = 0; i < widget->child_count; i++) {
        if (widget->children[i] && widget->children[i]->render) {
            widget->children[i]->render(widget->children[i], renderer);
        }
    }
}

static void time_widget_destroy(Widget* widget) {
    // Children are destroyed by widget_destroy
}