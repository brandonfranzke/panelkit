#ifndef RENDERING_H
#define RENDERING_H

#include "../core/sdl_includes.h"
#include <SDL_ttf.h>
#include <stdbool.h>

// Button states - exact enum from app.c
typedef enum {
    BUTTON_NORMAL,
    BUTTON_HOVER,
    BUTTON_PRESSED,
    BUTTON_HELD
} ButtonState;

// Initialize rendering system
void rendering_init(SDL_Renderer* renderer, TTF_Font* font, TTF_Font* large_font, TTF_Font* small_font);

// Core rendering functions - exact signatures from app.c
void render_button(int x, int y, int w, int h, const char* text, SDL_Color color, ButtonState state);
void draw_text(const char* text, int x, int y, SDL_Color color);
void draw_text_left(const char* text, int x, int y, SDL_Color color);
void draw_small_text_left(const char* text, int x, int y, SDL_Color color, int max_width);
void draw_large_text(const char* text, int x, int y, SDL_Color color);

// Page indicator rendering
void render_page_indicators(int current, int total, float transition);

// Set screen dimensions
void rendering_set_dimensions(int width, int height);

#endif // RENDERING_H