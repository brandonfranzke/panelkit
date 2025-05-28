#include "unity/unity.h"
#include "ui/style/style_validation.h"
#include "ui/style/style_core.h"
#include "ui/style/style_system.h"
#include "ui/style/style_constants.h"
#include "ui/widget.h"
#include <string.h>

// Mock widget for testing
static Widget test_widget = {
    .id = "test_widget",
    .type = WIDGET_TYPE_BUTTON
};


void test_style_validate_null_style(void) {
    // NULL style should be valid
    StyleValidation result = style_validate(NULL, &test_widget);
    TEST_ASSERT_EQUAL(STYLE_VALID, result);
}

void test_style_validate_valid_style(void) {
    Style* style = style_create();
    TEST_ASSERT_NOT_NULL(style);
    
    // Default style should be valid
    StyleValidation result = style_validate(style, &test_widget);
    TEST_ASSERT_EQUAL(STYLE_VALID, result);
    
    style_destroy(style);
}

void test_style_validate_missing_font(void) {
    Style* style = style_create();
    TEST_ASSERT_NOT_NULL(style);
    
    // Set a non-existent font
    strncpy(style->base.font_family, "non-existent-font", sizeof(style->base.font_family) - 1);
    
    StyleValidation result = style_validate(style, &test_widget);
    // Without font manager, fonts are assumed valid
    // TODO: Update this test when font manager is integrated
    TEST_ASSERT_EQUAL(STYLE_VALID, result);
    
    style_destroy(style);
}

void test_style_validate_invalid_font_size(void) {
    Style* style = style_create();
    TEST_ASSERT_NOT_NULL(style);
    
    // Set invalid font size
    style->base.font_size = 0;
    
    StyleValidation result = style_validate(style, &test_widget);
    TEST_ASSERT_EQUAL(STYLE_ERROR_INVALID_FONT_SIZE, result);
    
    style_destroy(style);
}

void test_style_validate_low_contrast(void) {
    Style* style = style_create();
    TEST_ASSERT_NOT_NULL(style);
    
    // Set similar colors for low contrast
    style->base.background = (PkColor){100, 100, 100, 255};
    style->base.foreground = (PkColor){110, 110, 110, 255};
    
    // Low contrast should still be valid (just a warning)
    StyleValidation result = style_validate(style, &test_widget);
    TEST_ASSERT_EQUAL(STYLE_VALID, result);
    
    style_destroy(style);
}

void test_style_validate_state_styles(void) {
    Style* style = style_create();
    TEST_ASSERT_NOT_NULL(style);
    
    // Add state styles
    style->hover = style_base_create();
    style->pressed = style_base_create();
    style->disabled = style_base_create();
    style->states_owned = true;
    
    // Set invalid font in hover state
    if (style->hover) {
        strncpy(style->hover->font_family, "bad-font", sizeof(style->hover->font_family) - 1);
    }
    
    // Should still be valid (state validation doesn't fail the whole style)
    StyleValidation result = style_validate(style, &test_widget);
    TEST_ASSERT_EQUAL(STYLE_VALID, result);
    
    style_destroy(style);
}

void test_style_validation_message(void) {
    const char* msg;
    
    msg = style_validation_message(STYLE_VALID);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_TRUE(strlen(msg) > 0);
    
    msg = style_validation_message(STYLE_ERROR_MISSING_FONT);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_TRUE(strstr(msg, "font") != NULL);
    
    msg = style_validation_message(STYLE_ERROR_INVALID_FONT_SIZE);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_TRUE(strstr(msg, "font size") != NULL);
}

void test_style_font_exists(void) {
    // Empty font family should be valid
    TEST_ASSERT_TRUE(style_font_exists(""));
    
    // "default" should always exist
    TEST_ASSERT_TRUE(style_font_exists("default"));
    
    // Without font manager, should assume valid
    TEST_ASSERT_TRUE(style_font_exists("any-font"));
}

void test_style_base_validate(void) {
    StyleBase base = BUTTON_BASE_STYLE;
    
    // Valid base style
    StyleValidation result = style_base_validate(&base, "test");
    TEST_ASSERT_EQUAL(STYLE_VALID, result);
    
    // Invalid font size
    base.font_size = 0;
    result = style_base_validate(&base, "test");
    TEST_ASSERT_EQUAL(STYLE_ERROR_INVALID_FONT_SIZE, result);
    
    // NULL base should be valid
    result = style_base_validate(NULL, "test");
    TEST_ASSERT_EQUAL(STYLE_VALID, result);
}

// Test registration
void test_style_validation_register(void) {
    RUN_TEST(test_style_validate_null_style);
    RUN_TEST(test_style_validate_valid_style);
    RUN_TEST(test_style_validate_missing_font);
    RUN_TEST(test_style_validate_invalid_font_size);
    RUN_TEST(test_style_validate_low_contrast);
    RUN_TEST(test_style_validate_state_styles);
    RUN_TEST(test_style_validation_message);
    RUN_TEST(test_style_font_exists);
    RUN_TEST(test_style_base_validate);
}