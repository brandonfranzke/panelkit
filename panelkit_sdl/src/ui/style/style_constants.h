#ifndef PK_STYLE_CONSTANTS_H
#define PK_STYLE_CONSTANTS_H

#include "style_core.h"
#include "color_utils.h"

// ============================================================================
// Semantic Colors
// ============================================================================

// Primary brand colors
static const PkColor COLOR_PRIMARY = {33, 150, 243, 255};      // Blue
static const PkColor COLOR_PRIMARY_DARK = {25, 118, 210, 255}; // Darker blue
static const PkColor COLOR_PRIMARY_LIGHT = {66, 165, 245, 255}; // Lighter blue

// Status colors
static const PkColor COLOR_SUCCESS = {76, 175, 80, 255};       // Green
static const PkColor COLOR_WARNING = {255, 152, 0, 255};       // Orange
static const PkColor COLOR_ERROR = {244, 67, 54, 255};         // Red
static const PkColor COLOR_INFO = {0, 188, 212, 255};          // Cyan

// Neutral colors
static const PkColor COLOR_TEXT_PRIMARY = {33, 33, 33, 255};   // Near black
static const PkColor COLOR_TEXT_SECONDARY = {117, 117, 117, 255}; // Gray
static const PkColor COLOR_TEXT_DISABLED = {189, 189, 189, 255};  // Light gray
static const PkColor COLOR_BACKGROUND = {250, 250, 250, 255};  // Off-white
static const PkColor COLOR_SURFACE = {255, 255, 255, 255};     // White
static const PkColor COLOR_DIVIDER = {224, 224, 224, 255};     // Light gray

// ============================================================================
// 8 Distinct Button Colors (from requirements)
// ============================================================================

static const PkColor COLOR_BUTTON_RED = {244, 67, 54, 255};
static const PkColor COLOR_BUTTON_GREEN = {76, 175, 80, 255};
static const PkColor COLOR_BUTTON_BLUE = {33, 150, 243, 255};
static const PkColor COLOR_BUTTON_YELLOW = {255, 235, 59, 255};
static const PkColor COLOR_BUTTON_PURPLE = {156, 39, 176, 255};
static const PkColor COLOR_BUTTON_ORANGE = {255, 152, 0, 255};
static const PkColor COLOR_BUTTON_TEAL = {0, 150, 136, 255};
static const PkColor COLOR_BUTTON_PINK = {233, 30, 99, 255};

// ============================================================================
// Common Spacing Values
// ============================================================================

static const Spacing SPACING_NONE = {0, 0, 0, 0};
static const Spacing SPACING_XS = {4, 4, 4, 4};
static const Spacing SPACING_SM = {8, 8, 8, 8};
static const Spacing SPACING_MD = {16, 16, 16, 16};
static const Spacing SPACING_LG = {24, 24, 24, 24};
static const Spacing SPACING_XL = {32, 32, 32, 32};

// Button-specific spacing
static const Spacing BUTTON_PADDING = {12, 24, 12, 24};
static const Spacing BUTTON_PADDING_SM = {8, 16, 8, 16};
static const Spacing BUTTON_PADDING_LG = {16, 32, 16, 32};

// ============================================================================
// Common Borders
// ============================================================================

static const Border BORDER_DEFAULT = {
    .color = {224, 224, 224, 255},
    .width = 1,
    .style = BORDER_STYLE_SOLID
};

static const Border BORDER_FOCUSED = {
    .color = {33, 150, 243, 255},
    .width = 2,
    .style = BORDER_STYLE_SOLID
};

static const Border BORDER_ERROR = {
    .color = {244, 67, 54, 255},
    .width = 1,
    .style = BORDER_STYLE_SOLID
};

// ============================================================================
// Common Shadows
// ============================================================================

static const Shadow SHADOW_SM = {
    .color = {0, 0, 0, 51},  // 20% black
    .offset_x = 0,
    .offset_y = 1,
    .blur_radius = 3
};

static const Shadow SHADOW_MD = {
    .color = {0, 0, 0, 38},  // 15% black
    .offset_x = 0,
    .offset_y = 2,
    .blur_radius = 6
};

static const Shadow SHADOW_LG = {
    .color = {0, 0, 0, 26},  // 10% black
    .offset_x = 0,
    .offset_y = 4,
    .blur_radius = 12
};

// ============================================================================
// Pre-defined Style Templates
// ============================================================================

// Base button style
static const StyleBase BUTTON_BASE_STYLE = {
    .background = {33, 150, 243, 255},      // Primary blue
    .foreground = {255, 255, 255, 255},     // White text
    .border = {.color = {0, 0, 0, 0}, .width = 0, .style = BORDER_STYLE_NONE},
    .border_radius = 4,
    .padding = {12, 24, 12, 24},
    .margin = {0, 0, 0, 0},
    .font_family = "default",
    .font_size = 16,
    .font_weight = 500,
    .text_align = STYLE_TEXT_ALIGN_CENTER,
    .text_decoration = TEXT_DECORATION_NONE,
    .line_height = 1.2f,
    .opacity = 255,
    .shadow = {.color = {0, 0, 0, 51}, .offset_x = 0, .offset_y = 1, .blur_radius = 3},
    .background_image = ""
};

// Hover state for buttons
static const StyleBase BUTTON_HOVER_STYLE = {
    .background = {25, 118, 210, 255},      // Darker blue
    .foreground = {255, 255, 255, 255},     // White text
    .border = {.color = {0, 0, 0, 0}, .width = 0, .style = BORDER_STYLE_NONE},
    .border_radius = 4,
    .padding = {12, 24, 12, 24},
    .margin = {0, 0, 0, 0},
    .font_family = "default",
    .font_size = 16,
    .font_weight = 500,
    .text_align = STYLE_TEXT_ALIGN_CENTER,
    .text_decoration = TEXT_DECORATION_NONE,
    .line_height = 1.2f,
    .opacity = 255,
    .shadow = {.color = {0, 0, 0, 38}, .offset_x = 0, .offset_y = 2, .blur_radius = 6},
    .background_image = ""
};

// Pressed state for buttons
static const StyleBase BUTTON_PRESSED_STYLE = {
    .background = {21, 101, 192, 255},      // Even darker blue
    .foreground = {255, 255, 255, 255},     // White text
    .border = {.color = {0, 0, 0, 0}, .width = 0, .style = BORDER_STYLE_NONE},
    .border_radius = 4,
    .padding = {12, 24, 12, 24},
    .margin = {0, 0, 0, 0},
    .font_family = "default",
    .font_size = 16,
    .font_weight = 500,
    .text_align = STYLE_TEXT_ALIGN_CENTER,
    .text_decoration = TEXT_DECORATION_NONE,
    .line_height = 1.2f,
    .opacity = 255,
    .shadow = {.color = {0, 0, 0, 0}, .offset_x = 0, .offset_y = 0, .blur_radius = 0},
    .background_image = ""
};

// Disabled state for buttons
static const StyleBase BUTTON_DISABLED_STYLE = {
    .background = {224, 224, 224, 255},     // Gray
    .foreground = {158, 158, 158, 255},     // Gray text
    .border = {.color = {0, 0, 0, 0}, .width = 0, .style = BORDER_STYLE_NONE},
    .border_radius = 4,
    .padding = {12, 24, 12, 24},
    .margin = {0, 0, 0, 0},
    .font_family = "default",
    .font_size = 16,
    .font_weight = 500,
    .text_align = STYLE_TEXT_ALIGN_CENTER,
    .text_decoration = TEXT_DECORATION_NONE,
    .line_height = 1.2f,
    .opacity = 255,
    .shadow = {.color = {0, 0, 0, 0}, .offset_x = 0, .offset_y = 0, .blur_radius = 0},
    .background_image = ""
};

// Text widget base style
static const StyleBase TEXT_BASE_STYLE = {
    .background = {0, 0, 0, 0},             // Transparent
    .foreground = {33, 33, 33, 255},        // Dark text
    .border = {.color = {0, 0, 0, 0}, .width = 0, .style = BORDER_STYLE_NONE},
    .border_radius = 0,
    .padding = {0, 0, 0, 0},
    .margin = {0, 0, 0, 0},
    .font_family = "default",
    .font_size = 16,
    .font_weight = 400,
    .text_align = STYLE_TEXT_ALIGN_LEFT,
    .text_decoration = TEXT_DECORATION_NONE,
    .line_height = 1.5f,
    .opacity = 255,
    .shadow = {.color = {0, 0, 0, 0}, .offset_x = 0, .offset_y = 0, .blur_radius = 0},
    .background_image = ""
};

// Panel/container base style
static const StyleBase PANEL_BASE_STYLE = {
    .background = {255, 255, 255, 255},     // White
    .foreground = {33, 33, 33, 255},        // Dark text
    .border = {.color = {224, 224, 224, 255}, .width = 1, .style = BORDER_STYLE_SOLID},
    .border_radius = 8,
    .padding = {16, 16, 16, 16},
    .margin = {8, 8, 8, 8},
    .font_family = "default",
    .font_size = 16,
    .font_weight = 400,
    .text_align = STYLE_TEXT_ALIGN_LEFT,
    .text_decoration = TEXT_DECORATION_NONE,
    .line_height = 1.5f,
    .opacity = 255,
    .shadow = {.color = {0, 0, 0, 26}, .offset_x = 0, .offset_y = 2, .blur_radius = 8},
    .background_image = ""
};

// ============================================================================
// Helper Functions
// ============================================================================

// Create a button style with a specific color
static inline Style create_colored_button_style(PkColor color) {
    Style style = {0};
    style.base = BUTTON_BASE_STYLE;
    style.base.background = color;
    
    // Generate appropriate text color
    style.base.foreground = pk_color_contrast_text(color);
    
    return style;
}

// Create a complete button style with states
static inline void setup_button_style_states(Style* style, PkColor base_color) {
    if (!style) return;
    
    // Base state
    style->base = BUTTON_BASE_STYLE;
    style->base.background = base_color;
    style->base.foreground = pk_color_contrast_text(base_color);
    
    // Allocate state variants
    style->hover = style_base_create_from(&BUTTON_HOVER_STYLE);
    style->pressed = style_base_create_from(&BUTTON_PRESSED_STYLE);
    style->disabled = style_base_create_from(&BUTTON_DISABLED_STYLE);
    
    if (style->hover) {
        style->hover->background = pk_color_darken(base_color, 0.1f);
        style->hover->foreground = style->base.foreground;
    }
    
    if (style->pressed) {
        style->pressed->background = pk_color_darken(base_color, 0.2f);
        style->pressed->foreground = style->base.foreground;
    }
    
    style->states_owned = true;
}

// ============================================================================
// Style Creation Functions
// ============================================================================

// Create a text style
static inline Style* style_create_text(void) {
    Style* style = style_create();
    if (style) {
        style->base = TEXT_BASE_STYLE;
    }
    return style;
}

// Create button styles with predefined colors
static inline Style* style_create_button_primary(void) {
    Style* style = style_create();
    if (style) {
        setup_button_style_states(style, COLOR_PRIMARY);
    }
    return style;
}

static inline Style* style_create_button_success(void) {
    Style* style = style_create();
    if (style) {
        setup_button_style_states(style, COLOR_BUTTON_GREEN);
    }
    return style;
}

static inline Style* style_create_button_error(void) {
    Style* style = style_create();
    if (style) {
        setup_button_style_states(style, COLOR_BUTTON_RED);
    }
    return style;
}

static inline Style* style_create_button_warning(void) {
    Style* style = style_create();
    if (style) {
        setup_button_style_states(style, COLOR_BUTTON_ORANGE);
    }
    return style;
}

// Create button styles for each of the 8 required colors
static inline Style* style_create_button_red(void) {
    Style* style = style_create();
    if (style) {
        setup_button_style_states(style, COLOR_BUTTON_RED);
    }
    return style;
}

static inline Style* style_create_button_green(void) {
    Style* style = style_create();
    if (style) {
        setup_button_style_states(style, COLOR_BUTTON_GREEN);
    }
    return style;
}

static inline Style* style_create_button_blue(void) {
    Style* style = style_create();
    if (style) {
        setup_button_style_states(style, COLOR_BUTTON_BLUE);
    }
    return style;
}

static inline Style* style_create_button_yellow(void) {
    Style* style = style_create();
    if (style) {
        setup_button_style_states(style, COLOR_BUTTON_YELLOW);
    }
    return style;
}

static inline Style* style_create_button_purple(void) {
    Style* style = style_create();
    if (style) {
        setup_button_style_states(style, COLOR_BUTTON_PURPLE);
    }
    return style;
}

static inline Style* style_create_button_orange(void) {
    Style* style = style_create();
    if (style) {
        setup_button_style_states(style, COLOR_BUTTON_ORANGE);
    }
    return style;
}

static inline Style* style_create_button_teal(void) {
    Style* style = style_create();
    if (style) {
        setup_button_style_states(style, COLOR_BUTTON_TEAL);
    }
    return style;
}

static inline Style* style_create_button_pink(void) {
    Style* style = style_create();
    if (style) {
        setup_button_style_states(style, COLOR_BUTTON_PINK);
    }
    return style;
}

#endif // PK_STYLE_CONSTANTS_H