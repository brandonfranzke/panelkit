# PanelKit Style System Implementation

Implement PanelKit's style system based on this specification. The style system controls visual appearance (colors, fonts, borders) while the layout system controls positioning and sizing. These are completely separate systems that never interact.

## Prerequisites

Read these files before implementing:

1. `/docs/personal/AGENT.md` - PanelKit coding patterns (error handling, memory management, logging)
2. `/docs/tracking/UI_FEATURE_TRACKING.md` - Lines 40-61 (style/layout separation), 87-91 (compile-time approach), 236-278 (layout implementation)
3. `/src/ui/layout/layout_core.h` - Understand the boundary: layout owns position/size/font-size, style owns appearance
4. `/src/ui/widget.h` - Widget structure you'll extend with style fields

## System Design Requirements

The style system provides:

1. **Direct style assignment** - Each widget explicitly sets its style. No inheritance from parent widgets.
2. **Template-based reuse** - Define style templates once, create variations at runtime.
3. **State variants** - Automatic style changes for hover/pressed/disabled states.
4. **Runtime updates** - Change colors based on data (temperature, device state).
5. **Manual refresh** - When modifying templates, explicitly update affected widgets.

The system explicitly does NOT provide:
- CSS-like cascading or selectors
- Parent-to-child style inheritance  
- Complex specificity resolution
- Runtime theme loading from files

## 3. Type System

Use a two-tier type system to prevent infinite style nesting at compile time.

```c
typedef enum {
    FONT_WEIGHT_NORMAL = 400,
    FONT_WEIGHT_MEDIUM = 500,
    FONT_WEIGHT_BOLD = 700
} FontWeight;

typedef struct StyleBase {
    SDL_Color background;
    SDL_Color foreground;
    SDL_Color border;
    int border_width;
    const char* font_name;
    FontWeight font_weight;
} StyleBase;

typedef struct Style {
    StyleBase base;
    StyleBase* hover;
    StyleBase* pressed;
    StyleBase* disabled;
} Style;
```

**Critical design constraint**: State variants (hover/pressed/disabled) are StyleBase* pointers, not Style*. This prevents `style->hover->hover` at compile time - the compiler won't allow it because StyleBase has no state variant fields.

**State resolution**: When widget state changes, set `widget->active_style` to point to the appropriate StyleBase:
- If WIDGET_STATE_DISABLED and style->disabled exists → use style->disabled
- Else if WIDGET_STATE_PRESSED and style->pressed exists → use style->pressed
- Else if WIDGET_STATE_HOVER and style->hover exists → use style->hover
- Else → use &style->base

**Memory model**: 
- Static templates: `static const StyleBase BUTTON_HOVER = {...}`
- Dynamic styles: `Style* s = calloc(1, sizeof(Style))`
- Widget owns style if `style_owned = true`, otherwise references shared template

## 4. Font Management

Implement a FontManager to handle font loading and size caching. Styles reference fonts by name (string), not direct pointers.

```c
typedef struct Font {
    char name[64];
    TTF_Font* size_cache[8];  // Sizes: 10, 12, 14, 16, 18, 24, 32, 48
    uint8_t* data;           // For embedded fonts
    size_t data_size;
    bool is_embedded;
} Font;

typedef struct FontManager {
    Font* fonts;
    size_t font_count;
    Font* default_sans;  // Required fallback
    Font* default_mono;  // Required fallback
} FontManager;
```

**Why centralized font management:**
- Prevents loading "arial.ttf" 100 times for 100 buttons
- Allows embedded fonts (required for kiosks without system fonts)
- Provides mandatory fallbacks when fonts are missing
- Separates font identity (style) from font size (layout)

**Layout/Style coordination**: Layout specifies size (affects positioning), style specifies appearance (family/weight). Combine at render time:

```c
TTF_Font* font_manager_get_sized(FontManager* fm, const char* name, int size) {
    Font* font = font_manager_get(fm, name);
    if (!font) font = fm->default_sans;  // Mandatory fallback
    
    // Check cache first
    for (int i = 0; i < 8; i++) {
        if (cached_sizes[i] == size && font->size_cache[i]) {
            return font->size_cache[i];
        }
    }
    
    // Load new size
    return font_load_size(font, size);
}
```

**Embedded font loading**: Generate .h files with `xxd -i font.ttf > font_embedded.h`, then load with `font_manager_load_embedded()`.

## 5. Memory Ownership

Widgets either own their style or reference a shared template. Track with `bool style_owned`.

**Shared reference (style_owned = false)**:
```c
// Global template
static const Style BUTTON_STANDARD = { ... };

// Widget references it
widget_set_style_ref(widget, &BUTTON_STANDARD);
```
Never free shared styles - they're compile-time constants or globals.

**Owned style (style_owned = true)**:
```c
// Create at runtime
Style* style = style_create_from(&BUTTON_STANDARD);
style->base.background = temperature_to_color(25.0f);

// Widget owns it
widget_set_style(widget, style);
```
Widget must free owned styles in its destroy function.

**Why no reference counting**: Styles are ~100 bytes each. With 2GB minimum RAM, tracking references adds complexity without meaningful benefit. Either the style is static (never freed) or dynamic (one owner).

## 6. Color Utilities

Implement these pure functions for color manipulation:

```c
// Lightness adjustment (0.0-1.0, where 0.3 = 30% lighter)
SDL_Color color_lighten(SDL_Color color, float factor);
SDL_Color color_darken(SDL_Color color, float factor);

// Auto-generate readable text color for any background
SDL_Color color_make_readable_on(SDL_Color background);

// Domain-specific conversions
SDL_Color temperature_to_color(float celsius);  // Blue→Green→Yellow→Red
SDL_Color color_from_hex(const char* hex);      // "#FF5733" format

// Validation
float color_contrast_ratio(SDL_Color fg, SDL_Color bg);
```

**Key use cases solved**:
- "I have orange background, need readable orange text" → Use lighten/darken
- "API returns random color for button" → Use color_make_readable_on()
- "Show temperature visually" → Use temperature_to_color()

**Color constants** (define in style_constants.h):
```c
// Semantic colors
#define COLOR_PRIMARY      ((SDL_Color){33, 150, 243, 255})
#define COLOR_SUCCESS      ((SDL_Color){76, 175, 80, 255})
#define COLOR_WARNING      ((SDL_Color){255, 152, 0, 255})
#define COLOR_ERROR        ((SDL_Color){244, 67, 54, 255})

// 8 distinct button colors from requirements
#define COLOR_RED          ((SDL_Color){244, 67, 54, 255})
#define COLOR_GREEN        ((SDL_Color){76, 175, 80, 255})
#define COLOR_BLUE         ((SDL_Color){33, 150, 243, 255})
#define COLOR_YELLOW       ((SDL_Color){255, 235, 59, 255})
#define COLOR_PURPLE       ((SDL_Color){156, 39, 176, 255})
#define COLOR_ORANGE       ((SDL_Color){255, 152, 0, 255})
#define COLOR_TEAL         ((SDL_Color){0, 150, 136, 255})
#define COLOR_PINK         ((SDL_Color){233, 30, 99, 255})
```

## 7. Style Assignment

Three patterns for assigning styles to widgets:

**Pattern 1: Reference shared template** (most common)
```c
widget_set_style_ref(widget, &BUTTON_STANDARD);
// widget->style = &BUTTON_STANDARD
// widget->style_owned = false
```

**Pattern 2: Clone and modify template**
```c
Style* custom = style_create_from(&BUTTON_STANDARD);
custom->base.background = COLOR_WARNING;
widget_set_style(widget, custom);
// widget->style = custom
// widget->style_owned = true
```

**Pattern 3: Create from scratch** (rare)
```c
Style* dynamic = style_create();
dynamic->base = (StyleBase){
    .background = temperature_to_color(temp),
    .foreground = COLOR_WHITE,
    .border_width = 2
};
widget_set_style(widget, dynamic);
```

**Implementation note**: `widget_set_style()` must free previous style if `style_owned` was true.

## 8. State Resolution

Update `widget->active_style` whenever widget state changes. Implement this exact precedence:

```c
void widget_update_active_style(Widget* widget) {
    if (!widget->style) {
        widget->active_style = NULL;
        return;
    }
    
    Style* style = widget->style;
    
    // Precedence: disabled > pressed > hover > base
    if ((widget->state_flags & WIDGET_STATE_DISABLED) && style->disabled) {
        widget->active_style = style->disabled;
    } else if ((widget->state_flags & WIDGET_STATE_PRESSED) && style->pressed) {
        widget->active_style = style->pressed;
    } else if ((widget->state_flags & WIDGET_STATE_HOVERED) && style->hover) {
        widget->active_style = style->hover;
    } else {
        widget->active_style = &style->base;
    }
    
    widget_mark_dirty(widget);
}
```

**When to call**: 
- After `widget_set_state()`
- After `widget_set_style()` or `widget_set_style_ref()`
- During widget initialization

**Renderer uses `active_style` directly** - no resolution at draw time.

## 9. Dynamic Style Updates

Optional observer pattern for widgets that change style based on external data:

```c
typedef struct StyleObserver {
    void (*on_style_changed)(Widget* widget, const Style* old_style, const Style* new_style);
    void* user_data;
} StyleObserver;
```

**Use cases**:
- Smart device buttons that change color when device state changes
- Temperature displays with gradient backgrounds
- Alerts that pulse or change appearance

**Why both old and new style parameters**:
- Calculate minimal redraw region (only if background changed)
- Enable smooth transitions (interpolate old→new)
- Debug what changed (log style transitions)

**Example implementation**:
```c
// Smart light button observer
void smart_light_style_changed(Widget* widget, const Style* old, const Style* new) {
    // Only redraw if colors actually changed
    if (old->base.background.r != new->base.background.r ||
        old->base.background.g != new->base.background.g ||
        old->base.background.b != new->base.background.b) {
        widget_invalidate_background(widget);
    }
}
```

**Key point**: Most widgets don't need observers. Only use for data-driven style changes.

## 10. Implementation Order

Implement in this specific order to avoid circular dependencies:

1. **Font Manager** (`font_manager.h/c`)
   - Layout system already expects fonts to exist
   - Other components need FontManager for testing

2. **Color Utilities** (`color_utils.h/c`)
   - Pure functions with no dependencies
   - Needed for style templates

3. **Style Core** (`style_core.h/c`)
   - Define StyleBase, Style, and basic functions
   - Depends on color utils for initialization

4. **Style Constants** (`style_constants.h`)
   - Color and dimension constants
   - Used by templates

5. **Widget Integration** 
   - Add style fields to Widget struct
   - Implement widget_set_style functions
   - Update widget lifecycle

6. **Style Templates** (`style_templates.h`)
   - Standard widget styles
   - Depends on all above

7. **Observer System**
   - Optional enhancement
   - Can be added after core works

8. **Debug Tools** (`style_debug.h/c`)
   - Last - needs full system to debug

## 11. Widget Integration

Add these fields to Widget struct:

```c
struct Widget {
    // ... existing fields ...
    
    // Style fields
    Style* style;                   // Widget's style (owned or referenced)
    bool style_owned;               // true = free on destroy
    StyleBase* active_style;        // Current state resolution
    StyleObserver* style_observer;  // Optional dynamic updates
}
```

**Widget lifecycle updates**:

```c
// Creation
Widget* widget_create(...) {
    // ... existing code ...
    widget->style = NULL;
    widget->style_owned = false;
    widget->active_style = NULL;
    widget->style_observer = NULL;
}

// State changes
void widget_set_state(Widget* widget, uint32_t flags) {
    widget->state_flags = flags;
    widget_update_active_style(widget);  // Resolve new active style
}

// Destruction
void widget_destroy(Widget* widget) {
    if (widget->style_owned && widget->style) {
        style_destroy(widget->style);
    }
    if (widget->style_observer) {
        free(widget->style_observer);
    }
    // ... existing cleanup ...
}

// Rendering
void widget_render(Widget* widget, SDL_Renderer* renderer) {
    if (!widget->active_style) return;
    
    SDL_SetRenderDrawColor(renderer, 
        widget->active_style->background.r,
        widget->active_style->background.g,
        widget->active_style->background.b,
        widget->active_style->background.a);
    // ... draw widget ...
}
```

## 12. Debug Support

Implement debug utilities for style inspection:

**Console output** (primary):
```c
void style_debug_dump_widget(const Widget* widget) {
    if (!widget || !widget->active_style) return;
    
    StyleBase* s = widget->active_style;
    log_debug("=== Style: %s ===", widget->id);
    log_debug("Background: #%02X%02X%02X", s->background.r, s->background.g, s->background.b);
    log_debug("Foreground: #%02X%02X%02X", s->foreground.r, s->foreground.g, s->foreground.b);
    log_debug("Font: %s %d", s->font_name, s->font_weight);
    log_debug("Border: %dpx #%02X%02X%02X", s->border_width, 
              s->border.r, s->border.g, s->border.b);
    
    // Show available states
    if (widget->style) {
        log_debug("States: hover=%s pressed=%s disabled=%s",
                  widget->style->hover ? "yes" : "no",
                  widget->style->pressed ? "yes" : "no",
                  widget->style->disabled ? "yes" : "no");
    }
}
```

**Visual feedback** (secondary):
```c
void style_debug_show_click(int x, int y) {
    // Draw red circle at click point
    debug_overlay_add_marker(x, y, 10, COLOR_ERROR);
    
    // Find and dump widgets at this point
    Widget* hit = widget_manager_hit_test(x, y);
    if (hit) {
        style_debug_dump_widget(hit);
    }
}
```

**Purpose**: Console for SSH debugging, visual for touch alignment issues.

## 13. Validation

Implement minimal validation focused on preventing crashes:

```c
StyleValidation style_validate(const Style* style, const Widget* widget) {
    if (!style) return STYLE_VALID;  // NULL style is valid
    
    // CRITICAL: Font must exist
    Font* font = font_manager_get(g_font_manager, style->base.font_name);
    if (!font) {
        log_error("Font '%s' not found for widget '%s'", 
                  style->base.font_name, widget->id);
        return STYLE_ERROR_MISSING_FONT;
    }
    
    // WARNING: Check contrast
    float ratio = color_contrast_ratio(style->base.foreground, 
                                      style->base.background);
    if (ratio < 3.0) {
        log_warn("Low contrast %.1f for widget '%s'", ratio, widget->id);
        // Don't fail - designer choice
    }
    
    return STYLE_VALID;
}
```

**What to validate**:
- Font existence (prevents TTF_RenderText crashes)
- Memory integrity (owned styles properly freed)
- Contrast ratio (warn only, don't enforce)

**What NOT to validate**:
- Color aesthetics
- Missing state styles (optional)
- Border width reasonableness

## 14. File Organization

Create this directory structure:

```
src/ui/style/
├── font_manager.h         # Font loading and caching
├── font_manager.c
├── color_utils.h          # Color manipulation functions
├── color_utils.c
├── style_core.h           # StyleBase, Style types and core functions
├── style_core.c
├── style_constants.h      # COLOR_* and dimension constants
├── style_templates.h      # Standard widget styles
└── style_debug.h          # Debug utilities
    └── style_debug.c

fonts/
├── embed_font.sh          # Script to generate .h files
└── embedded/
    ├── font_default_sans.h # Generated by embed_font.sh
    └── font_default_mono.h
```

**Key organization principles**:
- `style_constants.h` has no dependencies (just macros)
- `style_templates.h` includes all other style headers
- Font embedded data kept separate from code
- No subdirectories in src/ui/style/ (keep it flat)

## 15. Implementation Rules

**Absolute requirements**:
1. Style NEVER affects layout calculations (no border affecting size, etc.)
2. Always check font existence before use - missing font = crash
3. All styles heap allocated for consistency (even if copying template)
4. NULL style is valid - widget renders with SDL defaults

**PanelKit patterns to follow**:
```c
// Error handling
PkError style_create(Style** out_style) {
    if (!out_style) {
        pk_set_last_error_with_context(PK_ERROR_NULL_PARAM, 
                                       "NULL output in style_create");
        return PK_ERROR_NULL_PARAM;
    }
    // ...
}

// Logging
log_debug("Created style for widget '%s'", widget->id);
log_warn("Low contrast for '%s'", widget->id);
log_error("Font '%s' not found", font_name);
```

**Design constraints**:
- State styles (hover/pressed/disabled) are optional - NULL is valid
- Observer callbacks run synchronously - no threading
- Color utilities must be pure functions - same input = same output
- Template modifications affect future uses, not existing widgets

## Examples

### 8 Different Colored Buttons

Your requirement: 8 buttons with different colors but shared properties.

```c
// style_templates.h
static const StyleBase BUTTON_STATE_BASES[] = {
    [0] = { /* hover */ .background = COLOR_GRAY_LIGHT, .foreground = COLOR_BLACK },
    [1] = { /* pressed */ .background = COLOR_GRAY_DARK, .foreground = COLOR_WHITE },
    [2] = { /* disabled */ .background = COLOR_GRAY, .foreground = COLOR_GRAY_LIGHT }
};

static const Style BUTTON_STYLES[8] = {
    { .base = { .background = COLOR_RED, .foreground = COLOR_WHITE, .border_width = 2 },
      .hover = &BUTTON_STATE_BASES[0], .pressed = &BUTTON_STATE_BASES[1] },
    { .base = { .background = COLOR_GREEN, .foreground = COLOR_WHITE, .border_width = 2 },
      .hover = &BUTTON_STATE_BASES[0], .pressed = &BUTTON_STATE_BASES[1] },
    // ... 6 more colors
};

// Usage
for (int i = 0; i < 8; i++) {
    ButtonWidget* btn = button_widget_create(button_names[i]);
    widget_set_style_ref((Widget*)btn, &BUTTON_STYLES[i]);
}
```

### Runtime Style Updates

Change all button text colors:

```c
void update_button_text_colors(SDL_Color new_color) {
    // Get all buttons
    Widget** buttons;
    size_t count = widget_manager_find_by_type(WIDGET_TYPE_BUTTON, &buttons);
    
    for (size_t i = 0; i < count; i++) {
        if (buttons[i]->style_owned) {
            // Owned style - modify directly
            buttons[i]->style->base.foreground = new_color;
            widget_update_active_style(buttons[i]);
        } else {
            // Shared template - need to clone first
            Style* custom = style_create_from(buttons[i]->style);
            custom->base.foreground = new_color;
            widget_set_style(buttons[i], custom);
        }
    }
}
```

### Smart Device Buttons

Create buttons dynamically based on API data:

```c
void create_device_controls(Widget* container, DeviceList* devices) {
    for (size_t i = 0; i < devices->count; i++) {
        Device* dev = &devices->items[i];
        
        // Create button
        ButtonWidget* btn = button_widget_create(dev->id);
        button_set_text(btn, dev->name);
        
        // Style based on device state
        Style* style = style_create_from(&BUTTON_DEVICE_TEMPLATE);
        style->base.background = dev->is_on ? COLOR_SUCCESS : COLOR_GRAY_DARK;
        
        // Observer for future state changes
        if (dev->can_change_state) {
            StyleObserver* obs = calloc(1, sizeof(StyleObserver));
            obs->on_style_changed = on_device_state_changed;
            obs->user_data = dev;
            widget_set_style_observer((Widget*)btn, obs);
        }
        
        widget_set_style((Widget*)btn, style);
        widget_add_child(container, (Widget*)btn);
    }
}

// When device state changes
void on_device_state_changed(Widget* widget, const Style* old, const Style* new) {
    Device* dev = (Device*)widget->style_observer->user_data;
    new->base.background = dev->is_on ? COLOR_SUCCESS : COLOR_GRAY_DARK;
    widget_update_active_style(widget);
}
```

### Weather Widget with Temperature Colors

```c
void update_weather_display(WeatherWidget* widget, float temperature) {
    Style* style = widget->base.style_owned ? widget->base.style 
                                            : style_create_from(widget->base.style);
    
    // Map temperature to color
    style->base.background = temperature_to_color(temperature);
    
    // Ensure readable text
    style->base.foreground = color_make_readable_on(style->base.background);
    
    if (!widget->base.style_owned) {
        widget_set_style((Widget*)widget, style);
    } else {
        widget_update_active_style((Widget*)widget);
    }
}
```