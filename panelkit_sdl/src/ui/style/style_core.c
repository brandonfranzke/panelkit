#include "style_core.h"
#include "core/logger.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Widget state flags (should match widget.h when integrated)
#define WIDGET_STATE_DISABLED   (1 << 0)
#define WIDGET_STATE_PRESSED    (1 << 1)
#define WIDGET_STATE_HOVER      (1 << 2)
#define WIDGET_STATE_FOCUSED    (1 << 3)

// Default style values
static const StyleBase DEFAULT_STYLE = {
    .background = {255, 255, 255, 255},  // White
    .foreground = {0, 0, 0, 255},        // Black
    .border = {.color = {0, 0, 0, 0}, .width = 0, .style = BORDER_STYLE_NONE},
    .border_radius = 0,
    .padding = {0, 0, 0, 0},
    .margin = {0, 0, 0, 0},
    .font_family = "default",
    .font_size = 16,
    .font_weight = 400,
    .text_align = STYLE_TEXT_ALIGN_LEFT,
    .text_decoration = TEXT_DECORATION_NONE,
    .line_height = 1.2f,
    .opacity = 255,
    .shadow = {.color = {0, 0, 0, 0}, .offset_x = 0, .offset_y = 0, .blur_radius = 0},
    .background_image = ""
};

// Style lifecycle
Style* style_create(void) {
    Style* style = calloc(1, sizeof(Style));
    if (!style) {
        log_error("Failed to allocate Style");
        return NULL;
    }
    
    // Initialize with defaults
    style_base_copy(&style->base, &DEFAULT_STYLE);
    style->states_owned = false;
    
    return style;
}

Style* style_create_from(const Style* template) {
    if (!template) return style_create();
    
    Style* style = calloc(1, sizeof(Style));
    if (!style) {
        log_error("Failed to allocate Style");
        return NULL;
    }
    
    // Copy base style
    style_base_copy(&style->base, &template->base);
    strncpy(style->name, template->name, sizeof(style->name) - 1);
    
    // Don't copy state variants by default - they should be explicitly set
    style->hover = NULL;
    style->pressed = NULL;
    style->disabled = NULL;
    style->focused = NULL;
    style->states_owned = false;
    
    return style;
}

void style_destroy(Style* style) {
    if (!style) return;
    
    // Free owned state variants
    if (style->states_owned) {
        style_base_destroy(style->hover);
        style_base_destroy(style->pressed);
        style_base_destroy(style->disabled);
        style_base_destroy(style->focused);
    }
    
    free(style);
}

// StyleBase lifecycle
StyleBase* style_base_create(void) {
    StyleBase* base = calloc(1, sizeof(StyleBase));
    if (!base) {
        log_error("Failed to allocate StyleBase");
        return NULL;
    }
    
    // Initialize with defaults
    style_base_copy(base, &DEFAULT_STYLE);
    
    return base;
}

StyleBase* style_base_create_from(const StyleBase* template) {
    if (!template) return style_base_create();
    
    StyleBase* base = calloc(1, sizeof(StyleBase));
    if (!base) {
        log_error("Failed to allocate StyleBase");
        return NULL;
    }
    
    style_base_copy(base, template);
    
    return base;
}

void style_base_destroy(StyleBase* base) {
    // Simple free - no nested allocations in StyleBase
    free(base);
}

// Style operations
void style_copy(Style* dest, const Style* src) {
    if (!dest || !src) return;
    
    // Save state pointers and owned flag
    StyleBase* saved_hover = dest->hover;
    StyleBase* saved_pressed = dest->pressed;
    StyleBase* saved_disabled = dest->disabled;
    StyleBase* saved_focused = dest->focused;
    bool saved_owned = dest->states_owned;
    
    // Copy everything
    *dest = *src;
    
    // Restore pointers (don't share state variants)
    dest->hover = saved_hover;
    dest->pressed = saved_pressed;
    dest->disabled = saved_disabled;
    dest->focused = saved_focused;
    dest->states_owned = saved_owned;
}

void style_base_copy(StyleBase* dest, const StyleBase* src) {
    if (!dest || !src) return;
    *dest = *src;
}

bool style_equals(const Style* a, const Style* b) {
    if (!a || !b) return a == b;
    
    // Compare base styles
    if (!style_base_equals(&a->base, &b->base)) return false;
    
    // Compare name
    if (strcmp(a->name, b->name) != 0) return false;
    
    // For state variants, just compare pointers
    // (deep comparison would be expensive and rarely needed)
    return a->hover == b->hover &&
           a->pressed == b->pressed &&
           a->disabled == b->disabled &&
           a->focused == b->focused;
}

bool style_base_equals(const StyleBase* a, const StyleBase* b) {
    if (!a || !b) return a == b;
    
    // Compare all fields
    return pk_color_equals(a->background, b->background) &&
           pk_color_equals(a->foreground, b->foreground) &&
           pk_color_equals(a->border.color, b->border.color) &&
           a->border.width == b->border.width &&
           a->border.style == b->border.style &&
           a->border_radius == b->border_radius &&
           memcmp(&a->padding, &b->padding, sizeof(Spacing)) == 0 &&
           memcmp(&a->margin, &b->margin, sizeof(Spacing)) == 0 &&
           strcmp(a->font_family, b->font_family) == 0 &&
           a->font_size == b->font_size &&
           a->font_weight == b->font_weight &&
           a->text_align == b->text_align &&
           a->text_decoration == b->text_decoration &&
           a->line_height == b->line_height &&
           a->opacity == b->opacity &&
           memcmp(&a->shadow, &b->shadow, sizeof(Shadow)) == 0 &&
           strcmp(a->background_image, b->background_image) == 0;
}

// State resolution
const StyleBase* style_resolve_state(const Style* style, uint32_t widget_state) {
    if (!style) return NULL;
    
    // Priority order: disabled > pressed > focused > hover > base
    if ((widget_state & WIDGET_STATE_DISABLED) && style->disabled) {
        return style->disabled;
    }
    if ((widget_state & WIDGET_STATE_PRESSED) && style->pressed) {
        return style->pressed;
    }
    if ((widget_state & WIDGET_STATE_FOCUSED) && style->focused) {
        return style->focused;
    }
    if ((widget_state & WIDGET_STATE_HOVER) && style->hover) {
        return style->hover;
    }
    
    return &style->base;
}

// Utility functions
Spacing spacing_uniform(uint16_t value) {
    return (Spacing){value, value, value, value};
}

Spacing spacing_symmetric(uint16_t vertical, uint16_t horizontal) {
    return (Spacing){vertical, horizontal, vertical, horizontal};
}

Spacing spacing_create(uint16_t top, uint16_t right, uint16_t bottom, uint16_t left) {
    return (Spacing){top, right, bottom, left};
}

Border border_none(void) {
    return (Border){
        .color = {0, 0, 0, 0},
        .width = 0,
        .style = BORDER_STYLE_NONE
    };
}

Border border_solid(PkColor color, uint8_t width) {
    return (Border){
        .color = color,
        .width = width,
        .style = BORDER_STYLE_SOLID
    };
}

Shadow shadow_none(void) {
    return (Shadow){
        .color = {0, 0, 0, 0},
        .offset_x = 0,
        .offset_y = 0,
        .blur_radius = 0
    };
}

Shadow shadow_create(PkColor color, int16_t x, int16_t y, uint8_t blur) {
    return (Shadow){
        .color = color,
        .offset_x = x,
        .offset_y = y,
        .blur_radius = blur
    };
}

// Debug helpers
void style_print(const Style* style) {
    if (!style) {
        printf("Style: NULL\n");
        return;
    }
    
    printf("Style '%s':\n", style->name[0] ? style->name : "(unnamed)");
    printf("  Base:\n");
    style_base_print(&style->base);
    
    if (style->hover) {
        printf("  Hover state defined\n");
    }
    if (style->pressed) {
        printf("  Pressed state defined\n");
    }
    if (style->disabled) {
        printf("  Disabled state defined\n");
    }
    if (style->focused) {
        printf("  Focused state defined\n");
    }
    printf("  States owned: %s\n", style->states_owned ? "yes" : "no");
}

void style_base_print(const StyleBase* base) {
    if (!base) {
        printf("    NULL\n");
        return;
    }
    
    printf("    Background: #%08X\n", pk_color_to_hex(base->background));
    printf("    Foreground: #%08X\n", pk_color_to_hex(base->foreground));
    printf("    Border: %dpx %s #%08X\n", 
           base->border.width,
           base->border.style == BORDER_STYLE_NONE ? "none" :
           base->border.style == BORDER_STYLE_SOLID ? "solid" :
           base->border.style == BORDER_STYLE_DASHED ? "dashed" :
           base->border.style == BORDER_STYLE_DOTTED ? "dotted" : "double",
           pk_color_to_hex(base->border.color));
    printf("    Border radius: %dpx\n", base->border_radius);
    printf("    Padding: %d %d %d %d\n", 
           base->padding.top, base->padding.right, 
           base->padding.bottom, base->padding.left);
    printf("    Margin: %d %d %d %d\n", 
           base->margin.top, base->margin.right, 
           base->margin.bottom, base->margin.left);
    printf("    Font: %s %dpx weight:%d\n", 
           base->font_family, base->font_size, base->font_weight);
    printf("    Text align: %s\n",
           base->text_align == STYLE_TEXT_ALIGN_LEFT ? "left" :
           base->text_align == STYLE_TEXT_ALIGN_CENTER ? "center" :
           base->text_align == STYLE_TEXT_ALIGN_RIGHT ? "right" : "justify");
    printf("    Line height: %.2f\n", base->line_height);
    printf("    Opacity: %d\n", base->opacity);
    if (base->shadow.blur_radius > 0 || base->shadow.offset_x != 0 || base->shadow.offset_y != 0) {
        printf("    Shadow: %d,%d blur:%d #%08X\n",
               base->shadow.offset_x, base->shadow.offset_y,
               base->shadow.blur_radius, pk_color_to_hex(base->shadow.color));
    }
    if (base->background_image[0]) {
        printf("    Background image: %s\n", base->background_image);
    }
}