#ifndef PK_FONT_MANAGER_H
#define PK_FONT_MANAGER_H

#include "core/error.h"
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct FontManager FontManager;
typedef struct FontEntry FontEntry;

// Font size presets
typedef enum {
    FONT_SIZE_SMALL = 12,
    FONT_SIZE_NORMAL = 16,
    FONT_SIZE_LARGE = 20,
    FONT_SIZE_XLARGE = 24,
    FONT_SIZE_XXLARGE = 32
} FontSizePreset;

// Font style flags
typedef enum {
    FONT_STYLE_NORMAL = 0,
    FONT_STYLE_BOLD = 1 << 0,
    FONT_STYLE_ITALIC = 1 << 1,
    FONT_STYLE_UNDERLINE = 1 << 2,
    FONT_STYLE_STRIKETHROUGH = 1 << 3
} FontStyle;

// Font handle - opaque reference to loaded font
typedef struct {
    uint32_t id;
    uint16_t size;
    uint16_t style;
} FontHandle;

// Font manager lifecycle
PkError font_manager_create(FontManager** manager);
void font_manager_destroy(FontManager* manager);

// Font loading
PkError font_manager_load_font(FontManager* manager, const char* path, const char* name);
PkError font_manager_load_embedded_font(FontManager* manager, const uint8_t* data, size_t size, const char* name);

// Font access
PkError font_manager_get_font(FontManager* manager, const char* name, uint16_t size, uint16_t style, FontHandle* handle);
TTF_Font* font_manager_get_ttf_font(FontManager* manager, FontHandle handle);

// Font metrics
PkError font_manager_measure_text(FontManager* manager, FontHandle handle, const char* text, int* width, int* height);
PkError font_manager_get_line_height(FontManager* manager, FontHandle handle, int* height);

// Cache management
void font_manager_clear_cache(FontManager* manager);
size_t font_manager_get_cache_size(FontManager* manager);

// Default font
PkError font_manager_set_default_font(FontManager* manager, const char* name);
const char* font_manager_get_default_font(FontManager* manager);

#endif // PK_FONT_MANAGER_H