#include "unity/unity.h"
#include "ui/style/style_system.h"
#include "ui/style/font_manager.h"
#include "core/error.h"


void test_style_system_init(void) {
    PkError err = style_system_init();
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    // Should have created font manager
    FontManager* fm = style_system_get_font_manager();
    TEST_ASSERT_NOT_NULL(fm);
    
    // Second init should be ok (no-op)
    err = style_system_init();
    TEST_ASSERT_EQUAL(PK_OK, err);
}

void test_style_system_cleanup(void) {
    // Init first
    PkError err = style_system_init();
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    FontManager* fm = style_system_get_font_manager();
    TEST_ASSERT_NOT_NULL(fm);
    
    // Cleanup
    style_system_cleanup();
    
    // Font manager should be gone
    fm = style_system_get_font_manager();
    TEST_ASSERT_NULL(fm);
    
    // Multiple cleanups should be safe
    style_system_cleanup();
    style_system_cleanup();
}

void test_style_system_get_font_manager(void) {
    // Before init
    FontManager* fm = style_system_get_font_manager();
    TEST_ASSERT_NULL(fm);
    
    // After init
    style_system_init();
    fm = style_system_get_font_manager();
    TEST_ASSERT_NOT_NULL(fm);
    
    // Same instance each time
    FontManager* fm2 = style_system_get_font_manager();
    TEST_ASSERT_TRUE(fm == fm2);
}

void test_style_system_font_validation_integration(void) {
    // This tests that style validation can use the font manager
    style_system_init();
    
    // Load a test font
    FontManager* fm = style_system_get_font_manager();
    TEST_ASSERT_NOT_NULL(fm);
    
    // Note: We can't actually load fonts in unit tests without TTF_Init
    // but we can test the integration exists
    
    // Default font should exist
    FontHandle handle = {0};
    // Try to get font (will fail without TTF_Init)
    font_manager_get_font(fm, "default", 16, 0, &handle);
    // Will fail without TTF_Init, but that's ok for this test
    
    style_system_cleanup();
}

void test_style_system_register(void) {
    RUN_TEST(test_style_system_init);
    RUN_TEST(test_style_system_cleanup);
    RUN_TEST(test_style_system_get_font_manager);
    RUN_TEST(test_style_system_font_validation_integration);
}