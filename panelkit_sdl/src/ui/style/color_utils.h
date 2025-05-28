#ifndef PK_COLOR_UTILS_H
#define PK_COLOR_UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

// Color representation - RGBA with 8 bits per channel
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} PkColor;

// Predefined colors
extern const PkColor PK_COLOR_BLACK;
extern const PkColor PK_COLOR_WHITE;
extern const PkColor PK_COLOR_RED;
extern const PkColor PK_COLOR_GREEN;
extern const PkColor PK_COLOR_BLUE;
extern const PkColor PK_COLOR_YELLOW;
extern const PkColor PK_COLOR_CYAN;
extern const PkColor PK_COLOR_MAGENTA;
extern const PkColor PK_COLOR_TRANSPARENT;

// Color creation
PkColor pk_color_create(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
PkColor pk_color_from_hex(uint32_t hex);
PkColor pk_color_from_hex_string(const char* hex_str);
uint32_t pk_color_to_hex(PkColor color);

// Color manipulation
PkColor pk_color_lighten(PkColor color, float factor);
PkColor pk_color_darken(PkColor color, float factor);
PkColor pk_color_saturate(PkColor color, float factor);
PkColor pk_color_desaturate(PkColor color, float factor);
PkColor pk_color_fade(PkColor color, float alpha);
PkColor pk_color_blend(PkColor fg, PkColor bg, float alpha);

// Color comparison
bool pk_color_equals(PkColor a, PkColor b);
bool pk_color_equals_rgb(PkColor a, PkColor b);

// Color conversion
void pk_color_to_hsv(PkColor color, float* h, float* s, float* v);
PkColor pk_color_from_hsv(float h, float s, float v);
void pk_color_to_hsl(PkColor color, float* h, float* s, float* l);
PkColor pk_color_from_hsl(float h, float s, float l);

// Utility functions
float pk_color_brightness(PkColor color);
float pk_color_contrast(PkColor fg, PkColor bg);
PkColor pk_color_contrast_text(PkColor bg);

// SDL integration
SDL_Color pk_color_to_sdl(PkColor color);
PkColor pk_color_from_sdl(SDL_Color sdl_color);

#endif // PK_COLOR_UTILS_H