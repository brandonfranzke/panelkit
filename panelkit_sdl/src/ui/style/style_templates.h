#ifndef PK_STYLE_TEMPLATES_H
#define PK_STYLE_TEMPLATES_H

#include "style_core.h"
#include "style_constants.h"

// ============================================================================
// Type Definitions
// ============================================================================

// Notification types
typedef enum {
    NOTIFICATION_INFO,
    NOTIFICATION_SUCCESS,
    NOTIFICATION_WARNING,
    NOTIFICATION_ERROR
} NotificationType;

// ============================================================================
// Widget Style Templates
// ============================================================================

// Create a complete button style with all states for the given color
Style* style_template_button(PkColor base_color);

// Pre-defined button styles for the 8 required colors
Style* style_template_button_red(void);
Style* style_template_button_green(void);
Style* style_template_button_blue(void);
Style* style_template_button_yellow(void);
Style* style_template_button_purple(void);
Style* style_template_button_orange(void);
Style* style_template_button_teal(void);
Style* style_template_button_pink(void);

// Common widget styles
Style* style_template_text_label(void);
Style* style_template_text_heading(void);
Style* style_template_text_caption(void);
Style* style_template_panel(void);
Style* style_template_panel_transparent(void);
Style* style_template_input_field(void);

// Smart home specific styles
Style* style_template_device_button(bool is_on);
Style* style_template_temperature_display(float temperature);
Style* style_template_weather_widget(void);
Style* style_template_notification(NotificationType type);

// ============================================================================
// Style Template Registry
// ============================================================================

// Register a named template for reuse
void style_template_register(const char* name, const Style* template);

// Get a registered template by name
const Style* style_template_get(const char* name);

// Create a new style instance from a named template
Style* style_template_create_from(const char* template_name);

// List all registered templates
size_t style_template_list(const char*** names);

// Clear all registered templates
void style_template_clear_registry(void);

#endif // PK_STYLE_TEMPLATES_H