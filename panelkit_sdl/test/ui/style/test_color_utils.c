#include "unity/unity.h"
#include "ui/style/color_utils.h"
#include <math.h>

// Helper to check if floats are approximately equal
#define FLOAT_EPSILON 0.01f
#define ASSERT_FLOAT_EQUAL(expected, actual) \
    TEST_ASSERT(fabs((expected) - (actual)) < FLOAT_EPSILON)

void test_color_create(void) {
    PkColor color = pk_color_create(128, 64, 32, 255);
    TEST_ASSERT_EQUAL(128, color.r);
    TEST_ASSERT_EQUAL(64, color.g);
    TEST_ASSERT_EQUAL(32, color.b);
    TEST_ASSERT_EQUAL(255, color.a);
}

void test_color_from_hex(void) {
    // Test RGBA
    PkColor color = pk_color_from_hex(0xFF8040C0);
    TEST_ASSERT_EQUAL(255, color.r);
    TEST_ASSERT_EQUAL(128, color.g);
    TEST_ASSERT_EQUAL(64, color.b);
    TEST_ASSERT_EQUAL(192, color.a);
    
    // Test conversion back
    uint32_t hex = pk_color_to_hex(color);
    TEST_ASSERT_EQUAL(0xFF8040C0, hex);
}

void test_color_from_hex_string(void) {
    // Test #RRGGBB format
    PkColor color = pk_color_from_hex_string("#FF8040");
    TEST_ASSERT_EQUAL(255, color.r);
    TEST_ASSERT_EQUAL(128, color.g);
    TEST_ASSERT_EQUAL(64, color.b);
    TEST_ASSERT_EQUAL(255, color.a); // Default alpha
    
    // Test RRGGBB format (no #)
    color = pk_color_from_hex_string("FF8040");
    TEST_ASSERT_EQUAL(255, color.r);
    TEST_ASSERT_EQUAL(128, color.g);
    TEST_ASSERT_EQUAL(64, color.b);
    
    // Test #RRGGBBAA format
    color = pk_color_from_hex_string("#FF8040C0");
    TEST_ASSERT_EQUAL(255, color.r);
    TEST_ASSERT_EQUAL(128, color.g);
    TEST_ASSERT_EQUAL(64, color.b);
    TEST_ASSERT_EQUAL(192, color.a);
    
    // Test short form #RGB
    color = pk_color_from_hex_string("#F84");
    TEST_ASSERT_EQUAL(255, color.r);
    TEST_ASSERT_EQUAL(136, color.g);
    TEST_ASSERT_EQUAL(68, color.b);
    
    // Test invalid
    color = pk_color_from_hex_string(NULL);
    TEST_ASSERT(pk_color_equals(color, PK_COLOR_BLACK));
    
    color = pk_color_from_hex_string("invalid");
    TEST_ASSERT(pk_color_equals(color, PK_COLOR_BLACK));
}

void test_color_predefined(void) {
    // Test some predefined colors
    TEST_ASSERT(pk_color_equals(PK_COLOR_BLACK, pk_color_create(0, 0, 0, 255)));
    TEST_ASSERT(pk_color_equals(PK_COLOR_WHITE, pk_color_create(255, 255, 255, 255)));
    TEST_ASSERT(pk_color_equals(PK_COLOR_RED, pk_color_create(255, 0, 0, 255)));
    TEST_ASSERT(pk_color_equals(PK_COLOR_TRANSPARENT, pk_color_create(0, 0, 0, 0)));
}

void test_color_lighten_darken(void) {
    PkColor gray = pk_color_create(128, 128, 128, 255);
    
    // Test lighten
    PkColor lighter = pk_color_lighten(gray, 0.5f);
    TEST_ASSERT(lighter.r > gray.r);
    TEST_ASSERT(lighter.g > gray.g);
    TEST_ASSERT(lighter.b > gray.b);
    TEST_ASSERT_EQUAL(gray.a, lighter.a);
    
    // Test darken
    PkColor darker = pk_color_darken(gray, 0.5f);
    TEST_ASSERT(darker.r < gray.r);
    TEST_ASSERT(darker.g < gray.g);
    TEST_ASSERT(darker.b < gray.b);
    TEST_ASSERT_EQUAL(gray.a, darker.a);
    
    // Test extremes
    PkColor white = pk_color_lighten(gray, 1.0f);
    TEST_ASSERT_EQUAL(255, white.r);
    TEST_ASSERT_EQUAL(255, white.g);
    TEST_ASSERT_EQUAL(255, white.b);
    
    PkColor black = pk_color_darken(gray, 1.0f);
    TEST_ASSERT_EQUAL(0, black.r);
    TEST_ASSERT_EQUAL(0, black.g);
    TEST_ASSERT_EQUAL(0, black.b);
}

void test_color_fade(void) {
    PkColor opaque = pk_color_create(255, 128, 64, 255);
    
    PkColor half_fade = pk_color_fade(opaque, 0.5f);
    TEST_ASSERT_EQUAL(opaque.r, half_fade.r);
    TEST_ASSERT_EQUAL(opaque.g, half_fade.g);
    TEST_ASSERT_EQUAL(opaque.b, half_fade.b);
    TEST_ASSERT_EQUAL(127, half_fade.a);
    
    PkColor transparent = pk_color_fade(opaque, 0.0f);
    TEST_ASSERT_EQUAL(0, transparent.a);
}

void test_color_blend(void) {
    PkColor red = PK_COLOR_RED;
    PkColor blue = PK_COLOR_BLUE;
    
    // 50/50 blend
    PkColor purple = pk_color_blend(red, blue, 0.5f);
    TEST_ASSERT_EQUAL(127, purple.r);
    TEST_ASSERT_EQUAL(0, purple.g);
    TEST_ASSERT_EQUAL(127, purple.b);
    
    // Full red
    PkColor full_red = pk_color_blend(red, blue, 1.0f);
    TEST_ASSERT(pk_color_equals(full_red, red));
    
    // Full blue
    PkColor full_blue = pk_color_blend(red, blue, 0.0f);
    TEST_ASSERT(pk_color_equals(full_blue, blue));
}

void test_color_comparison(void) {
    PkColor c1 = pk_color_create(100, 150, 200, 255);
    PkColor c2 = pk_color_create(100, 150, 200, 255);
    PkColor c3 = pk_color_create(100, 150, 200, 128);
    PkColor c4 = pk_color_create(100, 150, 201, 255);
    
    // Test equals
    TEST_ASSERT_TRUE(pk_color_equals(c1, c2));
    TEST_ASSERT_FALSE(pk_color_equals(c1, c3)); // Different alpha
    TEST_ASSERT_FALSE(pk_color_equals(c1, c4)); // Different blue
    
    // Test equals_rgb (ignores alpha)
    TEST_ASSERT_TRUE(pk_color_equals_rgb(c1, c2));
    TEST_ASSERT_TRUE(pk_color_equals_rgb(c1, c3)); // Same RGB
    TEST_ASSERT_FALSE(pk_color_equals_rgb(c1, c4)); // Different blue
}

void test_color_hsv_conversion(void) {
    // Test red
    PkColor red = PK_COLOR_RED;
    float h, s, v;
    pk_color_to_hsv(red, &h, &s, &v);
    ASSERT_FLOAT_EQUAL(0.0f, h);
    ASSERT_FLOAT_EQUAL(1.0f, s);
    ASSERT_FLOAT_EQUAL(1.0f, v);
    
    // Convert back
    PkColor red2 = pk_color_from_hsv(h, s, v);
    TEST_ASSERT(pk_color_equals_rgb(red, red2));
    
    // Test gray (no saturation)
    PkColor gray = pk_color_create(128, 128, 128, 255);
    pk_color_to_hsv(gray, &h, &s, &v);
    ASSERT_FLOAT_EQUAL(0.0f, s);
    
    // Test cyan
    PkColor cyan = PK_COLOR_CYAN;
    pk_color_to_hsv(cyan, &h, &s, &v);
    ASSERT_FLOAT_EQUAL(180.0f, h);
    ASSERT_FLOAT_EQUAL(1.0f, s);
    ASSERT_FLOAT_EQUAL(1.0f, v);
}

void test_color_hsl_conversion(void) {
    // Test red
    PkColor red = PK_COLOR_RED;
    float h, s, l;
    pk_color_to_hsl(red, &h, &s, &l);
    ASSERT_FLOAT_EQUAL(0.0f, h);
    ASSERT_FLOAT_EQUAL(1.0f, s);
    ASSERT_FLOAT_EQUAL(0.5f, l);
    
    // Convert back
    PkColor red2 = pk_color_from_hsl(h, s, l);
    TEST_ASSERT(pk_color_equals_rgb(red, red2));
    
    // Test white (max lightness)
    PkColor white = PK_COLOR_WHITE;
    pk_color_to_hsl(white, &h, &s, &l);
    ASSERT_FLOAT_EQUAL(0.0f, s);
    ASSERT_FLOAT_EQUAL(1.0f, l);
    
    // Test mid-gray
    PkColor gray = pk_color_create(128, 128, 128, 255);
    pk_color_to_hsl(gray, &h, &s, &l);
    ASSERT_FLOAT_EQUAL(0.0f, s);
    ASSERT_FLOAT_EQUAL(0.5f, l);
}

void test_color_brightness(void) {
    // Test brightness calculation
    float black_brightness = pk_color_brightness(PK_COLOR_BLACK);
    float white_brightness = pk_color_brightness(PK_COLOR_WHITE);
    float gray_brightness = pk_color_brightness(pk_color_create(128, 128, 128, 255));
    
    ASSERT_FLOAT_EQUAL(0.0f, black_brightness);
    ASSERT_FLOAT_EQUAL(1.0f, white_brightness);
    ASSERT_FLOAT_EQUAL(0.5f, gray_brightness);
    
    // Test that green contributes more to brightness than red or blue
    float red_brightness = pk_color_brightness(PK_COLOR_RED);
    float green_brightness = pk_color_brightness(PK_COLOR_GREEN);
    float blue_brightness = pk_color_brightness(PK_COLOR_BLUE);
    
    TEST_ASSERT(green_brightness > red_brightness);
    TEST_ASSERT(red_brightness > blue_brightness);
}

void test_color_contrast(void) {
    // Test maximum contrast
    float max_contrast = pk_color_contrast(PK_COLOR_WHITE, PK_COLOR_BLACK);
    TEST_ASSERT(max_contrast > 20.0f); // Should be 21:1
    
    // Test minimum contrast
    float min_contrast = pk_color_contrast(PK_COLOR_BLACK, PK_COLOR_BLACK);
    ASSERT_FLOAT_EQUAL(1.0f, min_contrast);
    
    // Test contrast text selection
    PkColor dark_bg = pk_color_create(32, 32, 32, 255);
    PkColor light_bg = pk_color_create(224, 224, 224, 255);
    
    PkColor dark_text = pk_color_contrast_text(light_bg);
    PkColor light_text = pk_color_contrast_text(dark_bg);
    
    TEST_ASSERT(pk_color_equals(dark_text, PK_COLOR_BLACK));
    TEST_ASSERT(pk_color_equals(light_text, PK_COLOR_WHITE));
}

void test_color_saturate_desaturate(void) {
    // Start with a moderately saturated color
    PkColor color = pk_color_create(200, 100, 100, 255);
    
    // Saturate
    PkColor saturated = pk_color_saturate(color, 0.5f);
    float h1, s1, l1;
    float h2, s2, l2;
    pk_color_to_hsl(color, &h1, &s1, &l1);
    pk_color_to_hsl(saturated, &h2, &s2, &l2);
    
    TEST_ASSERT(s2 > s1); // More saturated
    ASSERT_FLOAT_EQUAL(h1, h2); // Same hue
    ASSERT_FLOAT_EQUAL(l1, l2); // Same lightness
    
    // Desaturate
    PkColor desaturated = pk_color_desaturate(color, 0.5f);
    float h3, s3, l3;
    pk_color_to_hsl(desaturated, &h3, &s3, &l3);
    
    TEST_ASSERT(s3 < s1); // Less saturated
    ASSERT_FLOAT_EQUAL(h1, h3); // Same hue
    ASSERT_FLOAT_EQUAL(l1, l3); // Same lightness
}

void test_color_sdl_conversion(void) {
    PkColor pk_color = pk_color_create(100, 150, 200, 250);
    
    // Convert to SDL
    SDL_Color sdl_color = pk_color_to_sdl(pk_color);
    TEST_ASSERT_EQUAL(100, sdl_color.r);
    TEST_ASSERT_EQUAL(150, sdl_color.g);
    TEST_ASSERT_EQUAL(200, sdl_color.b);
    TEST_ASSERT_EQUAL(250, sdl_color.a);
    
    // Convert back
    PkColor pk_color2 = pk_color_from_sdl(sdl_color);
    TEST_ASSERT(pk_color_equals(pk_color, pk_color2));
}

// Test registration
void test_color_utils_register(void) {
    RUN_TEST(test_color_create);
    RUN_TEST(test_color_from_hex);
    RUN_TEST(test_color_from_hex_string);
    RUN_TEST(test_color_predefined);
    RUN_TEST(test_color_lighten_darken);
    RUN_TEST(test_color_fade);
    RUN_TEST(test_color_blend);
    RUN_TEST(test_color_comparison);
    RUN_TEST(test_color_hsv_conversion);
    RUN_TEST(test_color_hsl_conversion);
    RUN_TEST(test_color_brightness);
    RUN_TEST(test_color_contrast);
    RUN_TEST(test_color_saturate_desaturate);
    RUN_TEST(test_color_sdl_conversion);
}