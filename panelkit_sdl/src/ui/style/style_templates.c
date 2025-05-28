#include "style_templates.h"
#include "color_utils.h"
#include "font_manager.h"
#include "core/logger.h"
#include "core/error.h"
#include <stdlib.h>
#include <string.h>

// Template registry
typedef struct {
    char name[64];
    Style* template;
} TemplateEntry;

static struct {
    TemplateEntry* entries;
    size_t count;
    size_t capacity;
} g_template_registry = {0};

// ============================================================================
// Button Templates
// ============================================================================

Style* style_template_button(PkColor base_color) {
    Style* style = style_create();
    if (!style) return NULL;
    
    // Base state
    style->base = BUTTON_BASE_STYLE;
    style->base.background = base_color;
    style->base.foreground = pk_color_contrast_text(base_color);
    
    // Allocate state variants
    style->hover = style_base_create();
    style->pressed = style_base_create();
    style->disabled = style_base_create();
    style->focused = style_base_create();
    
    if (!style->hover || !style->pressed || !style->disabled || !style->focused) {
        style_destroy(style);
        return NULL;
    }
    
    // Copy base style to states
    style_base_copy(style->hover, &BUTTON_HOVER_STYLE);
    style_base_copy(style->pressed, &BUTTON_PRESSED_STYLE);
    style_base_copy(style->disabled, &BUTTON_DISABLED_STYLE);
    style_base_copy(style->focused, &style->base);
    
    // Adjust colors for states
    style->hover->background = pk_color_lighten(base_color, 0.1f);
    style->hover->foreground = style->base.foreground;
    
    style->pressed->background = pk_color_darken(base_color, 0.2f);
    style->pressed->foreground = style->base.foreground;
    
    style->focused->border = BORDER_FOCUSED;
    
    style->states_owned = true;
    
    log_debug("Created button template with color (%d,%d,%d)", 
              base_color.r, base_color.g, base_color.b);
    
    return style;
}

// Pre-defined button colors
Style* style_template_button_red(void) {
    return style_template_button(COLOR_BUTTON_RED);
}

Style* style_template_button_green(void) {
    return style_template_button(COLOR_BUTTON_GREEN);
}

Style* style_template_button_blue(void) {
    return style_template_button(COLOR_BUTTON_BLUE);
}

Style* style_template_button_yellow(void) {
    return style_template_button(COLOR_BUTTON_YELLOW);
}

Style* style_template_button_purple(void) {
    return style_template_button(COLOR_BUTTON_PURPLE);
}

Style* style_template_button_orange(void) {
    return style_template_button(COLOR_BUTTON_ORANGE);
}

Style* style_template_button_teal(void) {
    return style_template_button(COLOR_BUTTON_TEAL);
}

Style* style_template_button_pink(void) {
    return style_template_button(COLOR_BUTTON_PINK);
}

// ============================================================================
// Text Templates
// ============================================================================

Style* style_template_text_label(void) {
    Style* style = style_create();
    if (!style) return NULL;
    
    style->base = TEXT_BASE_STYLE;
    return style;
}

Style* style_template_text_heading(void) {
    Style* style = style_create();
    if (!style) return NULL;
    
    style->base = TEXT_BASE_STYLE;
    style->base.font_size = FONT_SIZE_XLARGE;
    style->base.font_weight = 700;
    style->base.margin = SPACING_MD;
    
    return style;
}

Style* style_template_text_caption(void) {
    Style* style = style_create();
    if (!style) return NULL;
    
    style->base = TEXT_BASE_STYLE;
    style->base.font_size = FONT_SIZE_SMALL;
    style->base.foreground = COLOR_TEXT_SECONDARY;
    
    return style;
}

// ============================================================================
// Panel Templates
// ============================================================================

Style* style_template_panel(void) {
    Style* style = style_create();
    if (!style) return NULL;
    
    style->base = PANEL_BASE_STYLE;
    return style;
}

Style* style_template_panel_transparent(void) {
    Style* style = style_create();
    if (!style) return NULL;
    
    style->base = PANEL_BASE_STYLE;
    style->base.background = (PkColor){0, 0, 0, 0};
    style->base.border.width = 0;
    style->base.shadow = (Shadow){0};
    
    return style;
}

Style* style_template_input_field(void) {
    Style* style = style_create();
    if (!style) return NULL;
    
    // Base style for input fields
    style->base.background = COLOR_SURFACE;
    style->base.foreground = COLOR_TEXT_PRIMARY;
    style->base.border = BORDER_DEFAULT;
    style->base.border_radius = 4;
    style->base.padding = SPACING_SM;
    style->base.font_family[0] = '\0';  // Use default
    style->base.font_size = FONT_SIZE_NORMAL;
    
    // Focused state
    style->focused = style_base_create();
    if (style->focused) {
        style_base_copy(style->focused, &style->base);
        style->focused->border = BORDER_FOCUSED;
    }
    
    // Disabled state
    style->disabled = style_base_create();
    if (style->disabled) {
        style_base_copy(style->disabled, &style->base);
        style->disabled->background = COLOR_DIVIDER;
        style->disabled->foreground = COLOR_TEXT_DISABLED;
    }
    
    style->states_owned = true;
    
    return style;
}

// ============================================================================
// Smart Home Templates
// ============================================================================

Style* style_template_device_button(bool is_on) {
    PkColor base_color = is_on ? COLOR_SUCCESS : COLOR_DIVIDER;
    Style* style = style_template_button(base_color);
    
    if (style && is_on) {
        // Add glow effect for "on" state
        style->base.shadow = SHADOW_MD;
        style->base.shadow.color = pk_color_with_alpha(COLOR_SUCCESS, 128);
    }
    
    return style;
}

Style* style_template_temperature_display(float temperature) {
    Style* style = style_create();
    if (!style) return NULL;
    
    style->base = TEXT_BASE_STYLE;
    style->base.font_size = FONT_SIZE_XXLARGE;
    style->base.font_weight = 300;
    
    // Color based on temperature
    if (temperature < 10.0f) {
        style->base.foreground = COLOR_BUTTON_BLUE;  // Cold
    } else if (temperature < 20.0f) {
        style->base.foreground = COLOR_BUTTON_TEAL;  // Cool
    } else if (temperature < 25.0f) {
        style->base.foreground = COLOR_BUTTON_GREEN; // Comfortable
    } else if (temperature < 30.0f) {
        style->base.foreground = COLOR_BUTTON_ORANGE; // Warm
    } else {
        style->base.foreground = COLOR_BUTTON_RED;   // Hot
    }
    
    return style;
}

Style* style_template_weather_widget(void) {
    Style* style = style_create();
    if (!style) return NULL;
    
    style->base = PANEL_BASE_STYLE;
    style->base.background = pk_color_with_alpha(COLOR_SURFACE, 240);
    style->base.padding = SPACING_LG;
    
    return style;
}

Style* style_template_notification(NotificationType type) {
    Style* style = style_create();
    if (!style) return NULL;
    
    style->base = PANEL_BASE_STYLE;
    style->base.border_radius = 8;
    style->base.padding = SPACING_MD;
    style->base.margin = SPACING_SM;
    
    switch (type) {
        case NOTIFICATION_INFO:
            style->base.background = COLOR_INFO;
            style->base.foreground = COLOR_SURFACE;
            break;
        case NOTIFICATION_SUCCESS:
            style->base.background = COLOR_SUCCESS;
            style->base.foreground = COLOR_SURFACE;
            break;
        case NOTIFICATION_WARNING:
            style->base.background = COLOR_WARNING;
            style->base.foreground = COLOR_TEXT_PRIMARY;
            break;
        case NOTIFICATION_ERROR:
            style->base.background = COLOR_ERROR;
            style->base.foreground = COLOR_SURFACE;
            break;
    }
    
    return style;
}

// ============================================================================
// Template Registry
// ============================================================================

void style_template_register(const char* name, const Style* template) {
    if (!name || !template) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "NULL name or template in style_template_register");
        return;
    }
    
    // Check if already registered
    for (size_t i = 0; i < g_template_registry.count; i++) {
        if (strcmp(g_template_registry.entries[i].name, name) == 0) {
            // Replace existing
            style_destroy(g_template_registry.entries[i].template);
            g_template_registry.entries[i].template = style_create_from(template);
            log_debug("Updated template '%s'", name);
            return;
        }
    }
    
    // Grow array if needed
    if (g_template_registry.count >= g_template_registry.capacity) {
        size_t new_capacity = g_template_registry.capacity == 0 ? 16 : g_template_registry.capacity * 2;
        TemplateEntry* new_entries = realloc(g_template_registry.entries, 
                                           new_capacity * sizeof(TemplateEntry));
        if (!new_entries) {
            pk_set_last_error(PK_ERROR_OUT_OF_MEMORY);
            return;
        }
        g_template_registry.entries = new_entries;
        g_template_registry.capacity = new_capacity;
    }
    
    // Add new entry
    TemplateEntry* entry = &g_template_registry.entries[g_template_registry.count++];
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    entry->template = style_create_from(template);
    
    log_debug("Registered template '%s'", name);
}

const Style* style_template_get(const char* name) {
    if (!name) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM,
                                       "NULL name in style_template_get");
        return NULL;
    }
    
    for (size_t i = 0; i < g_template_registry.count; i++) {
        if (strcmp(g_template_registry.entries[i].name, name) == 0) {
            return g_template_registry.entries[i].template;
        }
    }
    
    return NULL;
}

Style* style_template_create_from(const char* template_name) {
    const Style* template = style_template_get(template_name);
    if (!template) {
        log_warn("Template '%s' not found", template_name);
        return NULL;
    }
    
    return style_create_from(template);
}

size_t style_template_list(const char*** names) {
    if (!names) {
        return g_template_registry.count;
    }
    
    static const char** name_list = NULL;
    static size_t name_list_capacity = 0;
    
    if (name_list_capacity < g_template_registry.count) {
        const char** new_list = realloc(name_list, 
                                      g_template_registry.count * sizeof(char*));
        if (!new_list) {
            return 0;
        }
        name_list = new_list;
        name_list_capacity = g_template_registry.count;
    }
    
    for (size_t i = 0; i < g_template_registry.count; i++) {
        name_list[i] = g_template_registry.entries[i].name;
    }
    
    *names = name_list;
    return g_template_registry.count;
}

void style_template_clear_registry(void) {
    for (size_t i = 0; i < g_template_registry.count; i++) {
        style_destroy(g_template_registry.entries[i].template);
    }
    
    free(g_template_registry.entries);
    g_template_registry.entries = NULL;
    g_template_registry.count = 0;
    g_template_registry.capacity = 0;
    
    log_debug("Cleared template registry");
}