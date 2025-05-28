#include "unity/unity.h"
#include "ui/style/font_manager.h"
#include <string.h>

// Simple tests that don't require SDL_ttf initialization

void test_font_manager_create_destroy_simple(void) {
    FontManager* manager = NULL;
    
    // Test NULL pointer
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, font_manager_create(NULL));
    
    // Test creation
    TEST_ASSERT_EQUAL(PK_OK, font_manager_create(&manager));
    TEST_ASSERT_NOT_NULL(manager);
    
    // Test destruction (should handle NULL gracefully)
    font_manager_destroy(manager);
    font_manager_destroy(NULL);
}

void test_font_manager_invalid_params(void) {
    FontManager* manager = NULL;
    TEST_ASSERT_EQUAL(PK_OK, font_manager_create(&manager));
    
    // Test invalid load parameters
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, 
                      font_manager_load_font(NULL, "path", "name"));
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, 
                      font_manager_load_font(manager, NULL, "name"));
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, 
                      font_manager_load_font(manager, "path", NULL));
    
    // Test invalid embedded font parameters
    uint8_t data[] = {1, 2, 3};
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, 
                      font_manager_load_embedded_font(NULL, data, 3, "name"));
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, 
                      font_manager_load_embedded_font(manager, NULL, 3, "name"));
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, 
                      font_manager_load_embedded_font(manager, data, 0, "name"));
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, 
                      font_manager_load_embedded_font(manager, data, 3, NULL));
    
    // Test long font name
    char long_name[100];
    memset(long_name, 'a', 99);
    long_name[99] = '\0';
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, 
                      font_manager_load_font(manager, "path", long_name));
    
    font_manager_destroy(manager);
}

void test_font_manager_default_font(void) {
    FontManager* manager = NULL;
    TEST_ASSERT_EQUAL(PK_OK, font_manager_create(&manager));
    
    // Test getting default when none set
    const char* default_font = font_manager_get_default_font(manager);
    TEST_ASSERT_NOT_NULL(default_font);
    TEST_ASSERT_EQUAL_STRING("", default_font);
    
    // Test setting non-existent font as default
    TEST_ASSERT_EQUAL(PK_ERROR_NOT_FOUND, 
                      font_manager_set_default_font(manager, "non-existent"));
    
    // Test NULL parameters
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, 
                      font_manager_set_default_font(NULL, "font"));
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, 
                      font_manager_set_default_font(manager, NULL));
    TEST_ASSERT_NULL(font_manager_get_default_font(NULL));
    
    font_manager_destroy(manager);
}

void test_font_manager_cache_size(void) {
    FontManager* manager = NULL;
    TEST_ASSERT_EQUAL(PK_OK, font_manager_create(&manager));
    
    // Test initial cache size
    TEST_ASSERT_EQUAL(0, font_manager_get_cache_size(manager));
    TEST_ASSERT_EQUAL(0, font_manager_get_cache_size(NULL));
    
    // Test cache clear (should work even with empty cache)
    font_manager_clear_cache(manager);
    font_manager_clear_cache(NULL); // Should handle NULL
    
    font_manager_destroy(manager);
}

void test_font_manager_get_operations(void) {
    FontManager* manager = NULL;
    TEST_ASSERT_EQUAL(PK_OK, font_manager_create(&manager));
    
    FontHandle handle;
    int width, height;
    
    // Test get_font with invalid params
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, 
                      font_manager_get_font(NULL, "name", 16, 0, &handle));
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, 
                      font_manager_get_font(manager, "name", 16, 0, NULL));
    
    // Test measure_text with invalid params
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, 
                      font_manager_measure_text(NULL, handle, "text", &width, &height));
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, 
                      font_manager_measure_text(manager, handle, NULL, &width, &height));
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, 
                      font_manager_measure_text(manager, handle, "text", NULL, &height));
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, 
                      font_manager_measure_text(manager, handle, "text", &width, NULL));
    
    // Test get_line_height with invalid params
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, 
                      font_manager_get_line_height(NULL, handle, &height));
    TEST_ASSERT_EQUAL(PK_ERROR_INVALID_PARAM, 
                      font_manager_get_line_height(manager, handle, NULL));
    
    // Test get_ttf_font with NULL
    TEST_ASSERT_NULL(font_manager_get_ttf_font(NULL, handle));
    
    font_manager_destroy(manager);
}

// Test registration
void test_font_manager_simple_register(void) {
    RUN_TEST(test_font_manager_create_destroy_simple);
    RUN_TEST(test_font_manager_invalid_params);
    RUN_TEST(test_font_manager_default_font);
    RUN_TEST(test_font_manager_cache_size);
    RUN_TEST(test_font_manager_get_operations);
}