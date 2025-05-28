#include "font_manager.h"
#include "core/logger.h"
#include <string.h>
#include <stdlib.h>

#define MAX_FONT_NAME_LENGTH 64
#define MAX_CACHED_SIZES 16
#define INITIAL_FONT_CAPACITY 8

// Font cache entry for a specific size/style combination
typedef struct FontCacheEntry {
    uint16_t size;
    uint16_t style;
    TTF_Font* font;
} FontCacheEntry;

// Font entry for a loaded font file
struct FontEntry {
    char name[MAX_FONT_NAME_LENGTH];
    uint8_t* data;  // For embedded fonts
    size_t data_size;
    char* path;     // For file fonts
    FontCacheEntry* cache;
    size_t cache_count;
    size_t cache_capacity;
};

struct FontManager {
    FontEntry* fonts;
    size_t font_count;
    size_t font_capacity;
    char default_font[MAX_FONT_NAME_LENGTH];
    uint32_t next_id;
};

// Helper functions
static FontEntry* find_font_by_name(FontManager* manager, const char* name) {
    if (!manager || !name) return NULL;
    
    for (size_t i = 0; i < manager->font_count; i++) {
        if (strcmp(manager->fonts[i].name, name) == 0) {
            return &manager->fonts[i];
        }
    }
    return NULL;
}

static TTF_Font* load_font_size(FontEntry* entry, uint16_t size) {
    TTF_Font* font = NULL;
    
    if (entry->data) {
        SDL_RWops* rw = SDL_RWFromConstMem(entry->data, entry->data_size);
        if (rw) {
            font = TTF_OpenFontRW(rw, 1, size);
        }
    } else if (entry->path) {
        font = TTF_OpenFont(entry->path, size);
    }
    
    if (!font) {
        log_error("Failed to load font %s at size %d: %s", 
                  entry->name, size, TTF_GetError());
    }
    
    return font;
}

static void apply_font_style(TTF_Font* font, uint16_t style) {
    if (!font) return;
    
    int ttf_style = TTF_STYLE_NORMAL;
    if (style & FONT_STYLE_BOLD) ttf_style |= TTF_STYLE_BOLD;
    if (style & FONT_STYLE_ITALIC) ttf_style |= TTF_STYLE_ITALIC;
    if (style & FONT_STYLE_UNDERLINE) ttf_style |= TTF_STYLE_UNDERLINE;
    if (style & FONT_STYLE_STRIKETHROUGH) ttf_style |= TTF_STYLE_STRIKETHROUGH;
    
    TTF_SetFontStyle(font, ttf_style);
}

static FontCacheEntry* find_cached_font(FontEntry* entry, uint16_t size, uint16_t style) {
    for (size_t i = 0; i < entry->cache_count; i++) {
        if (entry->cache[i].size == size && entry->cache[i].style == style) {
            return &entry->cache[i];
        }
    }
    return NULL;
}

static PkError cache_font(FontEntry* entry, uint16_t size, uint16_t style, TTF_Font* font) {
    if (entry->cache_count >= entry->cache_capacity) {
        size_t new_capacity = entry->cache_capacity * 2;
        FontCacheEntry* new_cache = realloc(entry->cache, 
                                           new_capacity * sizeof(FontCacheEntry));
        if (!new_cache) {
            pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                          "Failed to expand font cache");
            return PK_ERROR_OUT_OF_MEMORY;
        }
        entry->cache = new_cache;
        entry->cache_capacity = new_capacity;
    }
    
    FontCacheEntry* cache_entry = &entry->cache[entry->cache_count++];
    cache_entry->size = size;
    cache_entry->style = style;
    cache_entry->font = font;
    
    return PK_OK;
}

// Public functions
PkError font_manager_create(FontManager** manager) {
    if (!manager) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Manager pointer is NULL");
        return PK_ERROR_INVALID_PARAM;
    }
    
    *manager = calloc(1, sizeof(FontManager));
    if (!*manager) {
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate FontManager");
        return PK_ERROR_OUT_OF_MEMORY;
    }
    
    (*manager)->fonts = calloc(INITIAL_FONT_CAPACITY, sizeof(FontEntry));
    if (!(*manager)->fonts) {
        free(*manager);
        *manager = NULL;
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate font array");
        return PK_ERROR_OUT_OF_MEMORY;
    }
    
    (*manager)->font_capacity = INITIAL_FONT_CAPACITY;
    (*manager)->next_id = 1;
    
    log_debug("FontManager created");
    return PK_OK;
}

void font_manager_destroy(FontManager* manager) {
    if (!manager) return;
    
    // Clean up all fonts and their caches
    for (size_t i = 0; i < manager->font_count; i++) {
        FontEntry* entry = &manager->fonts[i];
        
        // Close all cached TTF fonts
        for (size_t j = 0; j < entry->cache_count; j++) {
            if (entry->cache[j].font) {
                TTF_CloseFont(entry->cache[j].font);
            }
        }
        free(entry->cache);
        
        // Free font data
        free(entry->data);
        free(entry->path);
    }
    
    free(manager->fonts);
    free(manager);
    
    log_debug("FontManager destroyed");
}

PkError font_manager_load_font(FontManager* manager, const char* path, const char* name) {
    if (!manager || !path || !name) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Invalid arguments to load_font");
        return PK_ERROR_INVALID_PARAM;
    }
    
    if (strlen(name) >= MAX_FONT_NAME_LENGTH) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Font name too long");
        return PK_ERROR_INVALID_PARAM;
    }
    
    // Check if font already exists
    if (find_font_by_name(manager, name)) {
        pk_set_last_error_with_context(PK_ERROR_ALREADY_EXISTS,
                                       "Font with this name already loaded");
        return PK_ERROR_ALREADY_EXISTS;
    }
    
    // Expand array if needed
    if (manager->font_count >= manager->font_capacity) {
        size_t new_capacity = manager->font_capacity * 2;
        FontEntry* new_fonts = realloc(manager->fonts, 
                                       new_capacity * sizeof(FontEntry));
        if (!new_fonts) {
            pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                          "Failed to expand font array");
            return PK_ERROR_OUT_OF_MEMORY;
        }
        manager->fonts = new_fonts;
        manager->font_capacity = new_capacity;
    }
    
    // Add new font entry
    FontEntry* entry = &manager->fonts[manager->font_count++];
    memset(entry, 0, sizeof(FontEntry));
    strncpy(entry->name, name, MAX_FONT_NAME_LENGTH - 1);
    
    entry->path = strdup(path);
    if (!entry->path) {
        manager->font_count--;
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate path string");
        return PK_ERROR_OUT_OF_MEMORY;
    }
    
    entry->cache = calloc(MAX_CACHED_SIZES, sizeof(FontCacheEntry));
    if (!entry->cache) {
        free(entry->path);
        manager->font_count--;
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate font cache");
        return PK_ERROR_OUT_OF_MEMORY;
    }
    entry->cache_capacity = MAX_CACHED_SIZES;
    
    log_info("Loaded font '%s' from %s", name, path);
    return PK_OK;
}

PkError font_manager_load_embedded_font(FontManager* manager, const uint8_t* data, 
                                       size_t size, const char* name) {
    if (!manager || !data || size == 0 || !name) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Invalid arguments to load_embedded_font");
        return PK_ERROR_INVALID_PARAM;
    }
    
    if (strlen(name) >= MAX_FONT_NAME_LENGTH) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Font name too long");
        return PK_ERROR_INVALID_PARAM;
    }
    
    // Check if font already exists
    if (find_font_by_name(manager, name)) {
        pk_set_last_error_with_context(PK_ERROR_ALREADY_EXISTS,
                                       "Font with this name already loaded");
        return PK_ERROR_ALREADY_EXISTS;
    }
    
    // Expand array if needed
    if (manager->font_count >= manager->font_capacity) {
        size_t new_capacity = manager->font_capacity * 2;
        FontEntry* new_fonts = realloc(manager->fonts, 
                                       new_capacity * sizeof(FontEntry));
        if (!new_fonts) {
            pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                          "Failed to expand font array");
            return PK_ERROR_OUT_OF_MEMORY;
        }
        manager->fonts = new_fonts;
        manager->font_capacity = new_capacity;
    }
    
    // Copy font data
    uint8_t* font_data = malloc(size);
    if (!font_data) {
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate font data");
        return PK_ERROR_OUT_OF_MEMORY;
    }
    memcpy(font_data, data, size);
    
    // Add new font entry
    FontEntry* entry = &manager->fonts[manager->font_count++];
    memset(entry, 0, sizeof(FontEntry));
    strncpy(entry->name, name, MAX_FONT_NAME_LENGTH - 1);
    entry->data = font_data;
    entry->data_size = size;
    
    entry->cache = calloc(MAX_CACHED_SIZES, sizeof(FontCacheEntry));
    if (!entry->cache) {
        free(font_data);
        manager->font_count--;
        pk_set_last_error_with_context(PK_ERROR_OUT_OF_MEMORY,
                                       "Failed to allocate font cache");
        return PK_ERROR_OUT_OF_MEMORY;
    }
    entry->cache_capacity = MAX_CACHED_SIZES;
    
    log_info("Loaded embedded font '%s' (%zu bytes)", name, size);
    return PK_OK;
}

PkError font_manager_get_font(FontManager* manager, const char* name, 
                             uint16_t size, uint16_t style, FontHandle* handle) {
    if (!manager || !handle) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Invalid arguments to get_font");
        return PK_ERROR_INVALID_PARAM;
    }
    
    // Use default font if name is NULL
    const char* font_name = name ? name : manager->default_font;
    if (!font_name[0]) {
        pk_set_last_error_with_context(PK_ERROR_NOT_FOUND,
                                       "No font specified and no default set");
        return PK_ERROR_NOT_FOUND;
    }
    
    FontEntry* entry = find_font_by_name(manager, font_name);
    if (!entry) {
        pk_set_last_error_with_context(PK_ERROR_NOT_FOUND,
                                       "Font not found");
        return PK_ERROR_NOT_FOUND;
    }
    
    // Check cache first
    FontCacheEntry* cached = find_cached_font(entry, size, style);
    if (!cached) {
        // Load font at requested size
        TTF_Font* font = load_font_size(entry, size);
        if (!font) {
            pk_set_last_error_with_context(PK_ERROR_SDL,
                                          "Failed to load font");
            return PK_ERROR_SDL;
        }
        
        // Apply style
        apply_font_style(font, style);
        
        // Cache it
        PkError err = cache_font(entry, size, style, font);
        if (err != PK_OK) {
            TTF_CloseFont(font);
            return err;
        }
        
        cached = find_cached_font(entry, size, style);
    }
    
    // Create handle
    handle->id = manager->next_id++;
    handle->size = size;
    handle->style = style;
    
    return PK_OK;
}

TTF_Font* font_manager_get_ttf_font(FontManager* manager, FontHandle handle) {
    if (!manager) return NULL;
    
    // For now, we'll do a simple linear search
    // In a production system, we'd maintain a handle->font mapping
    for (size_t i = 0; i < manager->font_count; i++) {
        FontEntry* entry = &manager->fonts[i];
        FontCacheEntry* cached = find_cached_font(entry, handle.size, handle.style);
        if (cached) {
            return cached->font;
        }
    }
    
    return NULL;
}

PkError font_manager_measure_text(FontManager* manager, FontHandle handle, 
                                 const char* text, int* width, int* height) {
    if (!manager || !text || !width || !height) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Invalid arguments to measure_text");
        return PK_ERROR_INVALID_PARAM;
    }
    
    TTF_Font* font = font_manager_get_ttf_font(manager, handle);
    if (!font) {
        pk_set_last_error_with_context(PK_ERROR_NOT_FOUND,
                                       "Font not found for handle");
        return PK_ERROR_NOT_FOUND;
    }
    
    if (TTF_SizeText(font, text, width, height) != 0) {
        pk_set_last_error_with_context(PK_ERROR_SDL,
                                       "Failed to measure text");
        return PK_ERROR_SDL;
    }
    
    return PK_OK;
}

PkError font_manager_get_line_height(FontManager* manager, FontHandle handle, int* height) {
    if (!manager || !height) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Invalid arguments to get_line_height");
        return PK_ERROR_INVALID_PARAM;
    }
    
    TTF_Font* font = font_manager_get_ttf_font(manager, handle);
    if (!font) {
        pk_set_last_error_with_context(PK_ERROR_NOT_FOUND,
                                       "Font not found for handle");
        return PK_ERROR_NOT_FOUND;
    }
    
    *height = TTF_FontLineSkip(font);
    return PK_OK;
}

void font_manager_clear_cache(FontManager* manager) {
    if (!manager) return;
    
    for (size_t i = 0; i < manager->font_count; i++) {
        FontEntry* entry = &manager->fonts[i];
        for (size_t j = 0; j < entry->cache_count; j++) {
            if (entry->cache[j].font) {
                TTF_CloseFont(entry->cache[j].font);
            }
        }
        entry->cache_count = 0;
    }
    
    log_debug("Font cache cleared");
}

size_t font_manager_get_cache_size(FontManager* manager) {
    if (!manager) return 0;
    
    size_t total = 0;
    for (size_t i = 0; i < manager->font_count; i++) {
        total += manager->fonts[i].cache_count;
    }
    return total;
}

PkError font_manager_set_default_font(FontManager* manager, const char* name) {
    if (!manager || !name) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Invalid arguments to set_default_font");
        return PK_ERROR_INVALID_PARAM;
    }
    
    if (strlen(name) >= MAX_FONT_NAME_LENGTH) {
        pk_set_last_error_with_context(PK_ERROR_INVALID_PARAM,
                                       "Font name too long");
        return PK_ERROR_INVALID_PARAM;
    }
    
    // Verify font exists
    if (!find_font_by_name(manager, name)) {
        pk_set_last_error_with_context(PK_ERROR_NOT_FOUND,
                                       "Font not found");
        return PK_ERROR_NOT_FOUND;
    }
    
    strncpy(manager->default_font, name, MAX_FONT_NAME_LENGTH - 1);
    log_debug("Default font set to '%s'", name);
    return PK_OK;
}

const char* font_manager_get_default_font(FontManager* manager) {
    return manager ? manager->default_font : NULL;
}