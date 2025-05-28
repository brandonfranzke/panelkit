// Example of using style templates with widgets
#include "style_templates.h"
#include "../widget.h"
#include "../widgets/button_widget.h"
#include "../widget_manager.h"

// Example 1: Create 8 different colored buttons
void create_color_buttons(Widget* container) {
    const char* button_names[] = {"red", "green", "blue", "yellow", 
                                  "purple", "orange", "teal", "pink"};
    
    Style* (*creators[])(void) = {
        style_template_button_red,
        style_template_button_green,
        style_template_button_blue,
        style_template_button_yellow,
        style_template_button_purple,
        style_template_button_orange,
        style_template_button_teal,
        style_template_button_pink
    };
    
    int x = 10, y = 10;
    for (int i = 0; i < 8; i++) {
        // Create button
        ButtonWidget* btn = button_widget_create(button_names[i]);
        if (!btn) continue;
        
        // Apply color style
        Style* style = creators[i]();
        if (style) {
            widget_set_style_owned((Widget*)btn, style);
        }
        
        // Position button
        widget_set_bounds((Widget*)btn, x, y, 100, 40);
        
        // Add to container
        widget_add_child(container, (Widget*)btn);
        
        // Move to next position
        x += 110;
        if (i == 3) { // Start second row
            x = 10;
            y += 50;
        }
    }
}

// Example 2: Create temperature-sensitive display
void update_temperature_display(Widget* temp_widget, float temperature) {
    // Create temperature-based style
    Style* style = style_template_temperature_display(temperature);
    if (style) {
        widget_set_style_owned(temp_widget, style);
    }
}

// Example 3: Smart home device control
void create_device_control(Widget* container, const char* device_name, bool is_on) {
    // Create button with on/off styling
    ButtonWidget* btn = button_widget_create(device_name);
    if (!btn) return;
    
    // Apply device button style
    Style* style = style_template_device_button(is_on);
    if (style) {
        widget_set_style_owned((Widget*)btn, style);
    }
    
    widget_add_child(container, (Widget*)btn);
}

// Example 4: Runtime style updates
void update_all_button_colors(PkColor new_color) {
    // Get all buttons
    Widget** buttons = NULL;
    size_t count = 0;
    
    // Note: This assumes widget_manager_find_by_type exists
    // In real implementation, you'd iterate through widget tree
    
    for (size_t i = 0; i < count; i++) {
        if (buttons[i]->style_owned) {
            // Create new style with the color
            Style* new_style = style_template_button(new_color);
            if (new_style) {
                widget_set_style_owned(buttons[i], new_style);
            }
        }
    }
}

// Example 5: Using template registry
void register_app_styles(void) {
    // Register common styles
    style_template_register("button.primary", style_template_button_blue());
    style_template_register("button.success", style_template_button_green());
    style_template_register("button.danger", style_template_button_red());
    style_template_register("text.heading", style_template_text_heading());
    style_template_register("text.caption", style_template_text_caption());
    style_template_register("panel.main", style_template_panel());
}

// Then use registered styles
void apply_registered_style(Widget* widget, const char* style_name) {
    Style* style = style_template_create_from(style_name);
    if (style) {
        widget_set_style_owned(widget, style);
    }
}