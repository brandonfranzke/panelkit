/**
 * @file test_absolute_layout.c
 * @brief Unit tests for absolute layout
 */

#include "unity.h"
#include "ui/layout/layout_absolute.h"
#include "ui/layout/layout_core.h"
#include "ui/layout/widget_layout_adapter.h"
#include "ui/widget.h"
#include <stdlib.h>
#include <string.h>

// Helper to create test widget
static Widget* create_test_widget(const char* id, int x, int y, int w, int h) {
    Widget* widget = calloc(1, sizeof(Widget));
    strncpy(widget->id, id, sizeof(widget->id) - 1);
    widget->type = WIDGET_TYPE_BASE;
    widget->bounds = (SDL_Rect){x, y, w, h};
    widget->relative_bounds = widget->bounds;
    widget->child_capacity = 4;
    widget->children = calloc(widget->child_capacity, sizeof(Widget*));
    return widget;
}

// Helper to add child to widget
static void add_child_widget(Widget* parent, Widget* child) {
    if (parent->child_count >= parent->child_capacity) {
        parent->child_capacity *= 2;
        parent->children = realloc(parent->children, 
                                  parent->child_capacity * sizeof(Widget*));
    }
    parent->children[parent->child_count++] = child;
    child->parent = parent;
    
    // Update relative bounds
    child->relative_bounds.x = child->bounds.x - parent->bounds.x;
    child->relative_bounds.y = child->bounds.y - parent->bounds.y;
}

// Helper to free widget tree
static void free_widget_tree(Widget* widget) {
    if (!widget) return;
    
    for (size_t i = 0; i < widget->child_count; i++) {
        free_widget_tree(widget->children[i]);
    }
    free(widget->children);
    free(widget);
}

// Test absolute layout creation
void test_absolute_layout_create(void) {
    LayoutSpec* spec = layout_absolute_create();
    TEST_ASSERT_NOT_NULL(spec);
    TEST_ASSERT_EQUAL(LAYOUT_TYPE_ABSOLUTE, spec->type);
    TEST_ASSERT_NOT_NULL(spec->data.absolute);
    TEST_ASSERT_TRUE(spec->clip_overflow);
    
    layout_spec_destroy(spec);
}

// Test simple absolute positioning
void test_absolute_layout_simple(void) {
    LayoutSpec* spec = layout_absolute_create();
    TEST_ASSERT_NOT_NULL(spec);
    
    Widget* root = create_test_widget("root", 0, 0, 200, 100);
    Widget* child = create_test_widget("child", 10, 20, 50, 30);
    add_child_widget(root, child);
    
    LayoutContext context = {
        .available_rect = {0, 0, 200, 100},
        .transform = NULL,
        .scale_factor = 1.0f,
        .reference_width = 200,
        .reference_height = 100
    };
    
    LayoutResult results[2];
    
    LayoutEngine* engine = layout_absolute_get_engine();
    PkError err = engine->calculate(root, spec, &context, results);
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    // Check root position
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, results[0].computed_rect.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, results[0].computed_rect.y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 200.0f, results[0].computed_rect.width);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, results[0].computed_rect.height);
    
    // Check child position (relative to parent)
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, results[1].computed_rect.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, results[1].computed_rect.y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, results[1].computed_rect.width);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 30.0f, results[1].computed_rect.height);
    
    free_widget_tree(root);
    layout_spec_destroy(spec);
}

// Test relative positioning
void test_absolute_layout_relative(void) {
    LayoutSpec* spec = layout_absolute_create();
    
    Widget* root = create_test_widget("root", 0, 0, 200, 100);
    Widget* child = create_test_widget("child", 0, 0, 0, 0);
    add_child_widget(root, child);
    
    // Set relative bounds (50% width, 75% height)
    layout_absolute_set_bounds(child, 0.1f, 0.2f, 0.5f, 0.75f);
    
    LayoutContext context = {
        .available_rect = {0, 0, 200, 100},
        .transform = NULL,
        .scale_factor = 1.0f,
        .reference_width = 200,
        .reference_height = 100
    };
    
    LayoutResult results[2];
    
    LayoutEngine* engine = layout_absolute_get_engine();
    PkError err = engine->calculate(root, spec, &context, results);
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    // Check child converted to pixels
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, results[1].computed_rect.x);    // 0.1 * 200
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, results[1].computed_rect.y);    // 0.2 * 100
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, results[1].computed_rect.width);  // 0.5 * 200
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 75.0f, results[1].computed_rect.height);  // 0.75 * 100
    
    free_widget_tree(root);
    layout_spec_destroy(spec);
}

// Test with padding
void test_absolute_layout_with_padding(void) {
    LayoutSpec* spec = layout_absolute_create();
    layout_spec_set_padding(spec, 10.0f);
    
    Widget* root = create_test_widget("root", 0, 0, 200, 100);
    Widget* child = create_test_widget("child", 0, 0, 50, 50);
    add_child_widget(root, child);
    
    LayoutContext context = {
        .available_rect = {0, 0, 200, 100},
        .transform = NULL,
        .scale_factor = 1.0f,
        .reference_width = 200,
        .reference_height = 100
    };
    
    LayoutResult results[2];
    
    LayoutEngine* engine = layout_absolute_get_engine();
    PkError err = engine->calculate(root, spec, &context, results);
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    // Child position should be offset by padding
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, results[1].computed_rect.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, results[1].computed_rect.y);
    
    free_widget_tree(root);
    layout_spec_destroy(spec);
}

// Test clipping
void test_absolute_layout_clipping(void) {
    LayoutSpec* spec = layout_absolute_create();
    spec->clip_overflow = true;
    
    Widget* root = create_test_widget("root", 0, 0, 100, 100);
    Widget* child = create_test_widget("child", 80, 80, 50, 50); // Extends beyond parent
    add_child_widget(root, child);
    
    LayoutContext context = {
        .available_rect = {0, 0, 100, 100},
        .transform = NULL,
        .scale_factor = 1.0f,
        .reference_width = 100,
        .reference_height = 100
    };
    
    LayoutResult results[2];
    
    LayoutEngine* engine = layout_absolute_get_engine();
    PkError err = engine->calculate(root, spec, &context, results);
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    // Child should be marked as clipped
    TEST_ASSERT_TRUE(results[1].clipped);
    
    // Test with clipping disabled
    spec->clip_overflow = false;
    err = engine->calculate(root, spec, &context, results);
    TEST_ASSERT_EQUAL(PK_OK, err);
    TEST_ASSERT_FALSE(results[1].clipped);
    
    free_widget_tree(root);
    layout_spec_destroy(spec);
}

// Test minimum size calculation
void test_absolute_layout_min_size(void) {
    LayoutSpec* spec = layout_absolute_create();
    layout_spec_set_padding(spec, 10.0f);
    
    Widget* root = create_test_widget("root", 0, 0, 500, 500);
    Widget* child1 = create_test_widget("child1", 50, 50, 100, 100);
    Widget* child2 = create_test_widget("child2", 200, 150, 50, 50);
    
    add_child_widget(root, child1);
    add_child_widget(root, child2);
    
    float min_width, min_height;
    LayoutEngine* engine = layout_absolute_get_engine();
    PkError err = engine->get_min_size(root, spec, &min_width, &min_height);
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    // Minimum should be furthest child edge + padding
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 260.0f, min_width);  // 200 + 50 + 10 padding
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 210.0f, min_height); // 150 + 50 + 10 padding
    
    free_widget_tree(root);
    layout_spec_destroy(spec);
}

// Test widget layout adapter
void test_absolute_layout_apply_results(void) {
    LayoutSpec* spec = layout_absolute_create();
    
    Widget* root = create_test_widget("root", 0, 0, 200, 100);
    Widget* child = create_test_widget("child", 10, 20, 50, 30);
    add_child_widget(root, child);
    
    LayoutContext context = {
        .available_rect = {0, 0, 200, 100},
        .transform = NULL,
        .scale_factor = 1.0f,
        .reference_width = 200,
        .reference_height = 100
    };
    
    LayoutResult results[2];
    
    LayoutEngine* engine = layout_absolute_get_engine();
    PkError err = engine->calculate(root, spec, &context, results);
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    // Apply results back to widgets
    err = widget_apply_layout_results(root, results, 2);
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    // Check widget bounds were updated
    TEST_ASSERT_EQUAL(0, root->bounds.x);
    TEST_ASSERT_EQUAL(0, root->bounds.y);
    TEST_ASSERT_EQUAL(10, child->bounds.x);
    TEST_ASSERT_EQUAL(20, child->bounds.y);
    
    free_widget_tree(root);
    layout_spec_destroy(spec);
}

// Register all tests
void test_absolute_layout_register(void) {
    RUN_TEST(test_absolute_layout_create);
    RUN_TEST(test_absolute_layout_simple);
    RUN_TEST(test_absolute_layout_relative);
    RUN_TEST(test_absolute_layout_with_padding);
    RUN_TEST(test_absolute_layout_clipping);
    RUN_TEST(test_absolute_layout_min_size);
    RUN_TEST(test_absolute_layout_apply_results);
}