#include "style_validation.h"
#include "font_manager.h"
#include "color_utils.h"
#include "core/logger.h"
#include <string.h>
#include <stdlib.h>

// Global font manager reference (set by application)
extern FontManager* g_font_manager;

StyleValidation style_validate(const Style* style, const Widget* widget) {
    if (!style) {
        // NULL style is valid - widget will use defaults
        return STYLE_VALID;
    }
    
    const char* widget_id = widget ? widget->id : "unknown";
    
    // Validate base style
    StyleValidation result = style_base_validate(&style->base, widget_id);
    if (result != STYLE_VALID) {
        return result;
    }
    
    // Validate state styles if present
    if (style->hover) {
        result = style_base_validate(style->hover, widget_id);
        if (result != STYLE_VALID) {
            log_warn("Hover style validation failed for widget '%s'", widget_id);
        }
    }
    
    if (style->pressed) {
        result = style_base_validate(style->pressed, widget_id);
        if (result != STYLE_VALID) {
            log_warn("Pressed style validation failed for widget '%s'", widget_id);
        }
    }
    
    if (style->disabled) {
        result = style_base_validate(style->disabled, widget_id);
        if (result != STYLE_VALID) {
            log_warn("Disabled style validation failed for widget '%s'", widget_id);
        }
    }
    
    return STYLE_VALID;
}

StyleValidation style_base_validate(const StyleBase* base, const char* widget_id) {
    if (!base) {
        return STYLE_VALID;  // NULL is valid
    }
    
    // CRITICAL: Font must exist if specified
    if (strlen(base->font_family) > 0 && strcmp(base->font_family, "default") != 0) {
        if (!style_font_exists(base->font_family)) {
            log_error("Font '%s' not found for widget '%s'", 
                      base->font_family, widget_id);
            return STYLE_ERROR_MISSING_FONT;
        }
    }
    
    // CRITICAL: Font size must be valid
    if (base->font_size == 0) {
        log_error("Invalid font size 0 for widget '%s'", widget_id);
        return STYLE_ERROR_INVALID_FONT_SIZE;
    }
    
    // WARNING: Check contrast (don't fail, just warn)
    // Calculate simple contrast ratio
    int fg_brightness = (base->foreground.r + base->foreground.g + base->foreground.b) / 3;
    int bg_brightness = (base->background.r + base->background.g + base->background.b) / 3;
    int diff = abs(fg_brightness - bg_brightness);
    if (diff < 100) {  // Simple threshold
        log_warn("Low contrast for widget '%s' (fg=%d, bg=%d, diff=%d)", 
                 widget_id, fg_brightness, bg_brightness, diff);
        // Don't fail - this is a design choice
    }
    
    return STYLE_VALID;
}

bool style_font_exists(const char* font_family) {
    if (!font_family || strlen(font_family) == 0) {
        return true;  // Empty font family is valid (uses default)
    }
    
    if (strcmp(font_family, "default") == 0) {
        return true;  // Default font always exists
    }
    
    if (!g_font_manager) {
        log_warn("Font manager not initialized, cannot validate font '%s'", font_family);
        return true;  // Assume valid if we can't check
    }
    
    // Check if font exists in font manager
    // For now, we'll just check if we can get any size
    FontHandle handle = {0};
    PkError err = font_manager_get_font(g_font_manager, font_family, 16, 0, &handle);
    return err == PK_OK;
}

const char* style_validation_message(StyleValidation result) {
    switch (result) {
        case STYLE_VALID:
            return "Style is valid";
        case STYLE_ERROR_NULL_STYLE:
            return "Style is NULL";
        case STYLE_ERROR_MISSING_FONT:
            return "Required font not found";
        case STYLE_ERROR_INVALID_FONT_SIZE:
            return "Invalid font size (must be > 0)";
        case STYLE_ERROR_LOW_CONTRAST:
            return "Text contrast too low for readability";
        case STYLE_WARNING_LOW_CONTRAST:
            return "Text contrast below recommended level";
        default:
            return "Unknown validation error";
    }
}