// Example of using the style observer system
#include "style_observer.h"
#include "style_templates.h"
#include "../widget.h"
#include "../widgets/button_widget.h"
#include "core/logger.h"

// Example 1: Log style changes
static void log_style_change(Widget* widget, const Style* old_style, 
                           const Style* new_style, void* user_data) {
    const char* context = (const char*)user_data;
    
    log_info("%s: Widget '%s' style changed", context, widget->id);
    
    if (old_style && old_style != new_style) {
        log_info("  Old background: (%d,%d,%d)",
                 old_style->base.background.r,
                 old_style->base.background.g,
                 old_style->base.background.b);
    }
    
    if (new_style) {
        log_info("  New background: (%d,%d,%d)",
                 new_style->base.background.r,
                 new_style->base.background.g,
                 new_style->base.background.b);
    }
}

// Example 2: Synchronize styles between widgets
typedef struct {
    Widget* target;
    bool syncing;  // Prevent infinite recursion
} SyncData;

static void sync_style_change(Widget* widget, const Style* old_style,
                            const Style* new_style, void* user_data) {
    SyncData* sync = (SyncData*)user_data;
    
    if (sync->syncing) return;  // Prevent recursion
    
    sync->syncing = true;
    
    // Copy style to target widget
    if (new_style) {
        Style* copy = style_create_from(new_style);
        if (copy) {
            widget_set_style_owned(sync->target, copy);
        }
    } else {
        widget_set_style_ref(sync->target, NULL);
    }
    
    sync->syncing = false;
}

// Example 3: Template change notification
static void on_template_changed(const char* template_name, 
                              const Style* new_template, void* user_data) {
    log_info("Template '%s' has been updated", template_name);
    
    // In a real app, you might want to update all widgets using this template
    // style_observer_update_template_users(template_name, new_template);
}

// Example 4: Batch style updates
void update_theme(Widget* root, bool dark_mode) {
    // Begin batch to avoid multiple redraws
    style_observer_begin_batch();
    
    // Update multiple templates
    if (dark_mode) {
        // Dark theme
        Style* dark_bg = style_create();
        dark_bg->base.background = (PkColor){33, 33, 33, 255};
        dark_bg->base.foreground = (PkColor){255, 255, 255, 255};
        style_template_register("app.background", dark_bg);
        
        Style* dark_button = style_template_button((PkColor){66, 66, 66, 255});
        style_template_register("button.primary", dark_button);
    } else {
        // Light theme
        Style* light_bg = style_create();
        light_bg->base.background = (PkColor){250, 250, 250, 255};
        light_bg->base.foreground = (PkColor){33, 33, 33, 255};
        style_template_register("app.background", light_bg);
        
        Style* light_button = style_template_button_blue();
        style_template_register("button.primary", light_button);
    }
    
    // End batch - all notifications sent at once
    style_observer_end_batch();
}

// Example 5: Monitor specific widget
void monitor_button_styles(ButtonWidget* button) {
    // Register observer
    ObserverHandle handle = style_observer_register_widget(
        (Widget*)button,
        log_style_change,
        "ButtonMonitor"
    );
    
    // Later, unregister when done
    // style_observer_unregister_widget((Widget*)button, handle);
}

// Example 6: Create linked widgets
void create_linked_buttons(Widget* container) {
    // Create two buttons that share styles
    ButtonWidget* btn1 = button_widget_create("button1");
    ButtonWidget* btn2 = button_widget_create("button2");
    
    // Set up sync data
    SyncData* sync1 = malloc(sizeof(SyncData));
    sync1->target = (Widget*)btn2;
    sync1->syncing = false;
    
    SyncData* sync2 = malloc(sizeof(SyncData));
    sync2->target = (Widget*)btn1;
    sync2->syncing = false;
    
    // Register bidirectional sync
    style_observer_register_widget((Widget*)btn1, sync_style_change, sync1);
    style_observer_register_widget((Widget*)btn2, sync_style_change, sync2);
    
    // Now any style change to btn1 will be reflected in btn2 and vice versa
    
    widget_add_child(container, (Widget*)btn1);
    widget_add_child(container, (Widget*)btn2);
}

// Example 7: Dynamic color scheme based on data
static void update_temperature_colors(Widget* widget, float temperature) {
    Style* style = widget->style_owned ? widget->style 
                                       : style_create_from(widget->style);
    
    if (!style) return;
    
    // Update colors based on temperature
    if (temperature < 18.0f) {
        style->base.background = COLOR_BUTTON_BLUE;  // Cold
    } else if (temperature < 24.0f) {
        style->base.background = COLOR_BUTTON_GREEN; // Comfortable
    } else {
        style->base.background = COLOR_BUTTON_RED;   // Hot
    }
    
    if (!widget->style_owned) {
        widget_set_style_owned(widget, style);
    } else {
        // Trigger observer notification manually
        style_observer_notify_widget(widget, widget->style, style);
        widget_update_active_style(widget);
        widget_invalidate(widget);
    }
}