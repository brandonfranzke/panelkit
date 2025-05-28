#include "unity/unity.h"
#include "ui/style/style_constants.h"

void test_color_constants(void) {
    // Test that color constants are properly defined
    TEST_ASSERT_EQUAL(33, COLOR_PRIMARY.r);
    TEST_ASSERT_EQUAL(150, COLOR_PRIMARY.g);
    TEST_ASSERT_EQUAL(243, COLOR_PRIMARY.b);
    TEST_ASSERT_EQUAL(255, COLOR_PRIMARY.a);
    
    // Test button colors are distinct
    TEST_ASSERT_FALSE(pk_color_equals(COLOR_BUTTON_RED, COLOR_BUTTON_GREEN));
    TEST_ASSERT_FALSE(pk_color_equals(COLOR_BUTTON_BLUE, COLOR_BUTTON_YELLOW));
    TEST_ASSERT_FALSE(pk_color_equals(COLOR_BUTTON_PURPLE, COLOR_BUTTON_ORANGE));
    TEST_ASSERT_FALSE(pk_color_equals(COLOR_BUTTON_TEAL, COLOR_BUTTON_PINK));
}

void test_spacing_constants(void) {
    // Test spacing values
    TEST_ASSERT_EQUAL(0, SPACING_NONE.top);
    TEST_ASSERT_EQUAL(0, SPACING_NONE.right);
    
    TEST_ASSERT_EQUAL(8, SPACING_SM.top);
    TEST_ASSERT_EQUAL(8, SPACING_SM.right);
    TEST_ASSERT_EQUAL(8, SPACING_SM.bottom);
    TEST_ASSERT_EQUAL(8, SPACING_SM.left);
    
    // Test button padding is asymmetric (more horizontal)
    TEST_ASSERT_EQUAL(12, BUTTON_PADDING.top);
    TEST_ASSERT_EQUAL(24, BUTTON_PADDING.right);
    TEST_ASSERT_EQUAL(12, BUTTON_PADDING.bottom);
    TEST_ASSERT_EQUAL(24, BUTTON_PADDING.left);
}

void test_border_constants(void) {
    // Test default border
    TEST_ASSERT_EQUAL(1, BORDER_DEFAULT.width);
    TEST_ASSERT_EQUAL(BORDER_STYLE_SOLID, BORDER_DEFAULT.style);
    
    // Test focused border is thicker
    TEST_ASSERT_EQUAL(2, BORDER_FOCUSED.width);
    TEST_ASSERT(pk_color_equals(COLOR_PRIMARY, BORDER_FOCUSED.color));
    
    // Test error border
    TEST_ASSERT(pk_color_equals(COLOR_ERROR, BORDER_ERROR.color));
}

void test_shadow_constants(void) {
    // Test shadows get progressively larger
    TEST_ASSERT(SHADOW_SM.blur_radius < SHADOW_MD.blur_radius);
    TEST_ASSERT(SHADOW_MD.blur_radius < SHADOW_LG.blur_radius);
    
    // Test shadows have decreasing opacity (higher alpha = less opaque)
    TEST_ASSERT(SHADOW_SM.color.a > SHADOW_MD.color.a);
    TEST_ASSERT(SHADOW_MD.color.a > SHADOW_LG.color.a);
}

void test_style_templates(void) {
    // Test button base style
    TEST_ASSERT_EQUAL(4, BUTTON_BASE_STYLE.border_radius);
    TEST_ASSERT_EQUAL(TEXT_ALIGN_CENTER, BUTTON_BASE_STYLE.text_align);
    TEST_ASSERT_EQUAL(500, BUTTON_BASE_STYLE.font_weight);
    
    // Test text base style
    TEST_ASSERT_EQUAL(0, TEXT_BASE_STYLE.background.a); // Transparent
    TEST_ASSERT_EQUAL(TEXT_ALIGN_LEFT, TEXT_BASE_STYLE.text_align);
    TEST_ASSERT_EQUAL(400, TEXT_BASE_STYLE.font_weight);
    
    // Test panel base style
    TEST_ASSERT_EQUAL(8, PANEL_BASE_STYLE.border_radius);
    TEST_ASSERT_EQUAL(1, PANEL_BASE_STYLE.border.width);
    TEST_ASSERT_EQUAL(16, PANEL_BASE_STYLE.padding.top);
}

void test_helper_functions(void) {
    // Test create colored button
    Style red_button = create_colored_button_style(COLOR_BUTTON_RED);
    TEST_ASSERT(pk_color_equals(COLOR_BUTTON_RED, red_button.base.background));
    
    // Should have white text on red background
    TEST_ASSERT(pk_color_equals(PK_COLOR_WHITE, red_button.base.foreground));
    
    // Test with light color - should get dark text
    Style yellow_button = create_colored_button_style(COLOR_BUTTON_YELLOW);
    TEST_ASSERT(pk_color_equals(COLOR_BUTTON_YELLOW, yellow_button.base.background));
    TEST_ASSERT(pk_color_equals(PK_COLOR_BLACK, yellow_button.base.foreground));
}

void test_setup_button_states(void) {
    Style* button = style_create();
    TEST_ASSERT_NOT_NULL(button);
    
    setup_button_style_states(button, COLOR_BUTTON_BLUE);
    
    // Verify base state
    TEST_ASSERT(pk_color_equals(COLOR_BUTTON_BLUE, button->base.background));
    
    // Verify states were created
    TEST_ASSERT_NOT_NULL(button->hover);
    TEST_ASSERT_NOT_NULL(button->pressed);
    TEST_ASSERT_NOT_NULL(button->disabled);
    TEST_ASSERT_TRUE(button->states_owned);
    
    // Hover should be darker
    TEST_ASSERT(button->hover->background.r < button->base.background.r);
    TEST_ASSERT(button->hover->background.g < button->base.background.g);
    TEST_ASSERT(button->hover->background.b < button->base.background.b);
    
    // Pressed should be even darker
    TEST_ASSERT(button->pressed->background.r < button->hover->background.r);
    TEST_ASSERT(button->pressed->background.g < button->hover->background.g);
    TEST_ASSERT(button->pressed->background.b < button->hover->background.b);
    
    // Clean up
    style_destroy(button);
}

// Test registration
void test_style_constants_register(void) {
    RUN_TEST(test_color_constants);
    RUN_TEST(test_spacing_constants);
    RUN_TEST(test_border_constants);
    RUN_TEST(test_shadow_constants);
    RUN_TEST(test_style_templates);
    RUN_TEST(test_helper_functions);
    RUN_TEST(test_setup_button_states);
}