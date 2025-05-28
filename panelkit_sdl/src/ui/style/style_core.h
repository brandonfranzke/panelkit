#ifndef PK_STYLE_CORE_H
#define PK_STYLE_CORE_H

#include "color_utils.h"
#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct FontManager FontManager;

// Border styles
typedef enum {
    BORDER_STYLE_NONE = 0,
    BORDER_STYLE_SOLID,
    BORDER_STYLE_DASHED,
    BORDER_STYLE_DOTTED,
    BORDER_STYLE_DOUBLE
} BorderStyle;

// Text alignment
typedef enum {
    STYLE_TEXT_ALIGN_LEFT = 0,
    STYLE_TEXT_ALIGN_CENTER,
    STYLE_TEXT_ALIGN_RIGHT,
    STYLE_TEXT_ALIGN_JUSTIFY
} StyleTextAlign;

// Text decoration
typedef enum {
    TEXT_DECORATION_NONE = 0,
    TEXT_DECORATION_UNDERLINE = 1 << 0,
    TEXT_DECORATION_OVERLINE = 1 << 1,
    TEXT_DECORATION_LINE_THROUGH = 1 << 2
} TextDecoration;

// Border definition
typedef struct {
    PkColor color;
    uint8_t width;
    BorderStyle style;
} Border;

// Padding/Margin structure
typedef struct {
    uint16_t top;
    uint16_t right;
    uint16_t bottom;
    uint16_t left;
} Spacing;

// Shadow definition
typedef struct {
    PkColor color;
    int16_t offset_x;
    int16_t offset_y;
    uint8_t blur_radius;
} Shadow;

// Base style structure - contains visual properties only
typedef struct StyleBase {
    // Colors
    PkColor background;
    PkColor foreground;
    
    // Border
    Border border;
    uint8_t border_radius;
    
    // Spacing
    Spacing padding;
    Spacing margin;
    
    // Text properties
    char font_family[64];  // Font name reference
    uint16_t font_size;
    uint16_t font_weight;  // 100-900
    StyleTextAlign text_align;
    TextDecoration text_decoration;
    float line_height;     // Multiplier of font size
    
    // Effects
    uint8_t opacity;       // 0-255
    Shadow shadow;
    
    // Background image (optional)
    char background_image[128];  // Path or resource name
    
} StyleBase;

// Full style structure with state variants
typedef struct Style {
    StyleBase base;
    
    // State variants (optional)
    StyleBase* hover;
    StyleBase* pressed;
    StyleBase* disabled;
    StyleBase* focused;
    
    // Metadata
    char name[64];         // Optional style name for debugging
    bool states_owned;     // If true, free state variants on destroy
    
} Style;

// Style lifecycle
Style* style_create(void);
Style* style_create_from(const Style* template);
void style_destroy(Style* style);

// StyleBase lifecycle
StyleBase* style_base_create(void);
StyleBase* style_base_create_from(const StyleBase* template);
void style_base_destroy(StyleBase* base);

// Style operations
void style_copy(Style* dest, const Style* src);
void style_base_copy(StyleBase* dest, const StyleBase* src);
bool style_equals(const Style* a, const Style* b);
bool style_base_equals(const StyleBase* a, const StyleBase* b);

// State resolution
const StyleBase* style_resolve_state(const Style* style, uint32_t widget_state);

// Utility functions
Spacing spacing_uniform(uint16_t value);
Spacing spacing_symmetric(uint16_t vertical, uint16_t horizontal);
Spacing spacing_create(uint16_t top, uint16_t right, uint16_t bottom, uint16_t left);

Border border_none(void);
Border border_solid(PkColor color, uint8_t width);

Shadow shadow_none(void);
Shadow shadow_create(PkColor color, int16_t x, int16_t y, uint8_t blur);

// Debug helpers
void style_print(const Style* style);
void style_base_print(const StyleBase* base);

#endif // PK_STYLE_CORE_H