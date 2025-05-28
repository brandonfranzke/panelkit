#ifndef PK_STYLE_VALIDATION_H
#define PK_STYLE_VALIDATION_H

#include "style_core.h"
#include "../widget.h"

// Style validation results
typedef enum {
    STYLE_VALID = 0,
    STYLE_ERROR_NULL_STYLE,
    STYLE_ERROR_MISSING_FONT,
    STYLE_ERROR_INVALID_FONT_SIZE,
    STYLE_ERROR_LOW_CONTRAST,
    STYLE_WARNING_LOW_CONTRAST
} StyleValidation;

// Validate a style for use with a widget
StyleValidation style_validate(const Style* style, const Widget* widget);

// Validate just the base style properties
StyleValidation style_base_validate(const StyleBase* base, const char* widget_id);

// Check if font exists (requires font manager)
bool style_font_exists(const char* font_family);

// Get validation error message
const char* style_validation_message(StyleValidation result);

#endif // PK_STYLE_VALIDATION_H