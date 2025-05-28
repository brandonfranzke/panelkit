#include "color_utils.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Predefined colors
const PkColor PK_COLOR_BLACK = {0, 0, 0, 255};
const PkColor PK_COLOR_WHITE = {255, 255, 255, 255};
const PkColor PK_COLOR_RED = {255, 0, 0, 255};
const PkColor PK_COLOR_GREEN = {0, 255, 0, 255};
const PkColor PK_COLOR_BLUE = {0, 0, 255, 255};
const PkColor PK_COLOR_YELLOW = {255, 255, 0, 255};
const PkColor PK_COLOR_CYAN = {0, 255, 255, 255};
const PkColor PK_COLOR_MAGENTA = {255, 0, 255, 255};
const PkColor PK_COLOR_TRANSPARENT = {0, 0, 0, 0};

// Helper functions
static uint8_t clamp_uint8(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (uint8_t)value;
}

static float clamp_float(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static float max3(float a, float b, float c) {
    float max = a;
    if (b > max) max = b;
    if (c > max) max = c;
    return max;
}

static float min3(float a, float b, float c) {
    float min = a;
    if (b < min) min = b;
    if (c < min) min = c;
    return min;
}

// Color creation
PkColor pk_color_create(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    PkColor color = {r, g, b, a};
    return color;
}

PkColor pk_color_from_hex(uint32_t hex) {
    PkColor color;
    color.r = (hex >> 24) & 0xFF;
    color.g = (hex >> 16) & 0xFF;
    color.b = (hex >> 8) & 0xFF;
    color.a = hex & 0xFF;
    return color;
}

PkColor pk_color_from_hex_string(const char* hex_str) {
    if (!hex_str) return PK_COLOR_BLACK;
    
    // Skip leading # if present
    if (hex_str[0] == '#') hex_str++;
    
    // Parse hex string
    uint32_t hex_value = 0;
    size_t len = strlen(hex_str);
    
    // Support both RGB and RGBA formats
    if (len == 6) {
        sscanf(hex_str, "%06x", &hex_value);
        hex_value = (hex_value << 8) | 0xFF; // Add full alpha
    } else if (len == 8) {
        sscanf(hex_str, "%08x", &hex_value);
    } else if (len == 3) {
        // Short form: #RGB -> #RRGGBB
        char r = hex_str[0];
        char g = hex_str[1];
        char b = hex_str[2];
        char full[7];
        sprintf(full, "%c%c%c%c%c%c", r, r, g, g, b, b);
        sscanf(full, "%06x", &hex_value);
        hex_value = (hex_value << 8) | 0xFF;
    } else {
        return PK_COLOR_BLACK;
    }
    
    return pk_color_from_hex(hex_value);
}

uint32_t pk_color_to_hex(PkColor color) {
    return ((uint32_t)color.r << 24) |
           ((uint32_t)color.g << 16) |
           ((uint32_t)color.b << 8) |
           (uint32_t)color.a;
}

// Color manipulation
PkColor pk_color_lighten(PkColor color, float factor) {
    factor = clamp_float(factor, 0.0f, 1.0f);
    
    PkColor result;
    result.r = clamp_uint8(color.r + (255 - color.r) * factor);
    result.g = clamp_uint8(color.g + (255 - color.g) * factor);
    result.b = clamp_uint8(color.b + (255 - color.b) * factor);
    result.a = color.a;
    
    return result;
}

PkColor pk_color_darken(PkColor color, float factor) {
    factor = clamp_float(factor, 0.0f, 1.0f);
    
    PkColor result;
    result.r = clamp_uint8(color.r * (1.0f - factor));
    result.g = clamp_uint8(color.g * (1.0f - factor));
    result.b = clamp_uint8(color.b * (1.0f - factor));
    result.a = color.a;
    
    return result;
}

PkColor pk_color_saturate(PkColor color, float factor) {
    float h, s, l;
    pk_color_to_hsl(color, &h, &s, &l);
    
    s = clamp_float(s * (1.0f + factor), 0.0f, 1.0f);
    
    return pk_color_from_hsl(h, s, l);
}

PkColor pk_color_desaturate(PkColor color, float factor) {
    return pk_color_saturate(color, -factor);
}

PkColor pk_color_fade(PkColor color, float alpha) {
    PkColor result = color;
    result.a = clamp_uint8(color.a * clamp_float(alpha, 0.0f, 1.0f));
    return result;
}

PkColor pk_color_blend(PkColor fg, PkColor bg, float alpha) {
    alpha = clamp_float(alpha, 0.0f, 1.0f);
    
    PkColor result;
    result.r = clamp_uint8(fg.r * alpha + bg.r * (1.0f - alpha));
    result.g = clamp_uint8(fg.g * alpha + bg.g * (1.0f - alpha));
    result.b = clamp_uint8(fg.b * alpha + bg.b * (1.0f - alpha));
    result.a = clamp_uint8(fg.a * alpha + bg.a * (1.0f - alpha));
    
    return result;
}

// Color comparison
bool pk_color_equals(PkColor a, PkColor b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

bool pk_color_equals_rgb(PkColor a, PkColor b) {
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

// Color conversion - RGB to HSV
void pk_color_to_hsv(PkColor color, float* h, float* s, float* v) {
    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;
    
    float max = max3(r, g, b);
    float min = min3(r, g, b);
    float delta = max - min;
    
    // Value
    *v = max;
    
    // Saturation
    if (max == 0) {
        *s = 0;
    } else {
        *s = delta / max;
    }
    
    // Hue
    if (delta == 0) {
        *h = 0;
    } else if (max == r) {
        *h = 60.0f * fmodf((g - b) / delta, 6.0f);
    } else if (max == g) {
        *h = 60.0f * ((b - r) / delta + 2.0f);
    } else {
        *h = 60.0f * ((r - g) / delta + 4.0f);
    }
    
    if (*h < 0) *h += 360.0f;
}

PkColor pk_color_from_hsv(float h, float s, float v) {
    h = fmodf(h, 360.0f);
    if (h < 0) h += 360.0f;
    s = clamp_float(s, 0.0f, 1.0f);
    v = clamp_float(v, 0.0f, 1.0f);
    
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    
    float r, g, b;
    
    if (h < 60) {
        r = c; g = x; b = 0;
    } else if (h < 120) {
        r = x; g = c; b = 0;
    } else if (h < 180) {
        r = 0; g = c; b = x;
    } else if (h < 240) {
        r = 0; g = x; b = c;
    } else if (h < 300) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }
    
    PkColor color;
    color.r = clamp_uint8((r + m) * 255);
    color.g = clamp_uint8((g + m) * 255);
    color.b = clamp_uint8((b + m) * 255);
    color.a = 255;
    
    return color;
}

// Helper function for HSL to RGB conversion
static float hue_to_rgb(float p, float q, float t) {
    if (t < 0) t += 1.0f;
    if (t > 1) t -= 1.0f;
    if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f/2.0f) return q;
    if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
    return p;
}

// Color conversion - RGB to HSL
void pk_color_to_hsl(PkColor color, float* h, float* s, float* l) {
    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;
    
    float max = max3(r, g, b);
    float min = min3(r, g, b);
    float delta = max - min;
    
    // Lightness
    *l = (max + min) / 2.0f;
    
    if (delta == 0) {
        *h = 0;
        *s = 0;
    } else {
        // Saturation
        if (*l < 0.5f) {
            *s = delta / (max + min);
        } else {
            *s = delta / (2.0f - max - min);
        }
        
        // Hue (same as HSV)
        if (max == r) {
            *h = 60.0f * fmodf((g - b) / delta, 6.0f);
        } else if (max == g) {
            *h = 60.0f * ((b - r) / delta + 2.0f);
        } else {
            *h = 60.0f * ((r - g) / delta + 4.0f);
        }
        
        if (*h < 0) *h += 360.0f;
    }
}

PkColor pk_color_from_hsl(float h, float s, float l) {
    h = fmodf(h, 360.0f);
    if (h < 0) h += 360.0f;
    s = clamp_float(s, 0.0f, 1.0f);
    l = clamp_float(l, 0.0f, 1.0f);
    
    if (s == 0) {
        // Achromatic
        uint8_t gray = clamp_uint8(l * 255);
        return pk_color_create(gray, gray, gray, 255);
    }
    
    float q;
    if (l < 0.5f) {
        q = l * (1.0f + s);
    } else {
        q = l + s - l * s;
    }
    
    float p = 2.0f * l - q;
    
    float hue = h / 360.0f;
    
    float r = hue_to_rgb(p, q, hue + 1.0f/3.0f);
    float g = hue_to_rgb(p, q, hue);
    float b = hue_to_rgb(p, q, hue - 1.0f/3.0f);
    
    PkColor color;
    color.r = clamp_uint8(r * 255);
    color.g = clamp_uint8(g * 255);
    color.b = clamp_uint8(b * 255);
    color.a = 255;
    
    return color;
}

// Utility functions
float pk_color_brightness(PkColor color) {
    // Using perceived brightness formula
    return (0.299f * color.r + 0.587f * color.g + 0.114f * color.b) / 255.0f;
}

float pk_color_contrast(PkColor fg, PkColor bg) {
    // WCAG contrast ratio calculation
    float l1 = pk_color_brightness(fg);
    float l2 = pk_color_brightness(bg);
    
    float lighter = fmaxf(l1, l2);
    float darker = fminf(l1, l2);
    
    return (lighter + 0.05f) / (darker + 0.05f);
}

PkColor pk_color_contrast_text(PkColor bg) {
    // Return black or white text based on background brightness
    float brightness = pk_color_brightness(bg);
    return brightness > 0.5f ? PK_COLOR_BLACK : PK_COLOR_WHITE;
}

// SDL integration
SDL_Color pk_color_to_sdl(PkColor color) {
    SDL_Color sdl_color = {color.r, color.g, color.b, color.a};
    return sdl_color;
}

PkColor pk_color_from_sdl(SDL_Color sdl_color) {
    return pk_color_create(sdl_color.r, sdl_color.g, sdl_color.b, sdl_color.a);
}

PkColor pk_color_with_alpha(PkColor color, uint8_t alpha) {
    color.a = alpha;
    return color;
}