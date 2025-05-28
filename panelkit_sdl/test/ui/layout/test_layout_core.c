/**
 * @file test_layout_core.c
 * @brief Unit tests for core layout functionality
 */

#include "unity.h"
#include "ui/layout/layout_core.h"
#include <math.h>


// Test layout rect contains point
void test_layout_rect_contains_point_inside(void) {
    LayoutRect rect = {10.0f, 20.0f, 100.0f, 50.0f};
    
    // Test center point
    TEST_ASSERT_TRUE(layout_rect_contains_point(&rect, 60.0f, 45.0f));
    
    // Test corners
    TEST_ASSERT_TRUE(layout_rect_contains_point(&rect, 10.0f, 20.0f));
    TEST_ASSERT_TRUE(layout_rect_contains_point(&rect, 109.9f, 69.9f));
}

void test_layout_rect_contains_point_outside(void) {
    LayoutRect rect = {10.0f, 20.0f, 100.0f, 50.0f};
    
    // Test points outside
    TEST_ASSERT_FALSE(layout_rect_contains_point(&rect, 9.9f, 45.0f));    // Left
    TEST_ASSERT_FALSE(layout_rect_contains_point(&rect, 110.1f, 45.0f));  // Right
    TEST_ASSERT_FALSE(layout_rect_contains_point(&rect, 60.0f, 19.9f));   // Top
    TEST_ASSERT_FALSE(layout_rect_contains_point(&rect, 60.0f, 70.1f));   // Bottom
}

// Test rectangle intersection
void test_layout_rect_intersect_overlapping(void) {
    LayoutRect a = {0.0f, 0.0f, 100.0f, 100.0f};
    LayoutRect b = {50.0f, 50.0f, 100.0f, 100.0f};
    LayoutRect result;
    
    TEST_ASSERT_TRUE(layout_rect_intersect(&a, &b, &result));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, result.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, result.y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, result.width);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, result.height);
}

void test_layout_rect_intersect_non_overlapping(void) {
    LayoutRect a = {0.0f, 0.0f, 50.0f, 50.0f};
    LayoutRect b = {100.0f, 100.0f, 50.0f, 50.0f};
    LayoutRect result;
    
    TEST_ASSERT_FALSE(layout_rect_intersect(&a, &b, &result));
}

// Test coordinate conversion
void test_layout_to_pixels_absolute(void) {
    // Absolute coordinates should pass through unchanged
    LayoutRect layout = {10.0f, 20.0f, 200.0f, 150.0f};
    LayoutRect pixels = layout_to_pixels(&layout, 800.0f, 600.0f);
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, pixels.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, pixels.y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 200.0f, pixels.width);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 150.0f, pixels.height);
}

void test_layout_to_pixels_relative(void) {
    // Relative coordinates (0.0-1.0) should scale to parent size
    LayoutRect layout = {0.1f, 0.2f, 0.5f, 0.25f};
    LayoutRect pixels = layout_to_pixels(&layout, 800.0f, 600.0f);
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 80.0f, pixels.x);      // 0.1 * 800
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 120.0f, pixels.y);     // 0.2 * 600
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 400.0f, pixels.width); // 0.5 * 800
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 150.0f, pixels.height);// 0.25 * 600
}

// Test display transformation
void test_layout_transform_rotation_90(void) {
    LayoutRect logical = {100.0f, 50.0f, 200.0f, 100.0f};
    DisplayTransform transform = {DISPLAY_ROTATION_90, false, false};
    
    LayoutRect physical = layout_transform_to_display(&logical, &transform, 800.0f, 600.0f);
    
    // After 90° rotation: x' = height - y - h, y' = x
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 450.0f, physical.x);  // 600 - 50 - 100
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, physical.y);  // x
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, physical.width);  // height becomes width
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 200.0f, physical.height); // width becomes height
}

// Register all tests
void test_layout_core_register(void) {
    RUN_TEST(test_layout_rect_contains_point_inside);
    RUN_TEST(test_layout_rect_contains_point_outside);
    RUN_TEST(test_layout_rect_intersect_overlapping);
    RUN_TEST(test_layout_rect_intersect_non_overlapping);
    RUN_TEST(test_layout_to_pixels_absolute);
    RUN_TEST(test_layout_to_pixels_relative);
    RUN_TEST(test_layout_transform_rotation_90);
}