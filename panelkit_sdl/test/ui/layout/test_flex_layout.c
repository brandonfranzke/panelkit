/**
 * @file test_flex_layout.c
 * @brief Unit tests for flexbox layout
 */

#include "unity.h"
#include "ui/layout/layout_flex.h"
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

// Test flex layout creation
void test_flex_layout_create(void) {
    LayoutSpec* spec = layout_flex_create(FLEX_DIRECTION_ROW);
    TEST_ASSERT_NOT_NULL(spec);
    TEST_ASSERT_EQUAL(LAYOUT_TYPE_FLEX, spec->type);
    TEST_ASSERT_NOT_NULL(spec->data.flex);
    
    FlexLayoutData* data = spec->data.flex;
    TEST_ASSERT_EQUAL(FLEX_DIRECTION_ROW, data->direction);
    TEST_ASSERT_EQUAL(FLEX_JUSTIFY_START, data->justify);
    TEST_ASSERT_EQUAL(FLEX_ALIGN_STRETCH, data->align_items);
    
    layout_spec_destroy(spec);
}

// Test flex container properties
void test_flex_container_properties(void) {
    LayoutSpec* spec = layout_flex_create(FLEX_DIRECTION_COLUMN);
    
    PkError err = layout_flex_set_container(spec, FLEX_JUSTIFY_CENTER, FLEX_ALIGN_CENTER);
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    FlexLayoutData* data = spec->data.flex;
    TEST_ASSERT_EQUAL(FLEX_JUSTIFY_CENTER, data->justify);
    TEST_ASSERT_EQUAL(FLEX_ALIGN_CENTER, data->align_items);
    
    layout_spec_destroy(spec);
}

// Test simple row layout
void test_flex_row_layout(void) {
    LayoutSpec* spec = layout_flex_create(FLEX_DIRECTION_ROW);
    
    Widget* root = create_test_widget("root", 0, 0, 300, 100);
    Widget* child1 = create_test_widget("child1", 0, 0, 50, 50);
    Widget* child2 = create_test_widget("child2", 0, 0, 50, 50);
    Widget* child3 = create_test_widget("child3", 0, 0, 50, 50);
    
    add_child_widget(root, child1);
    add_child_widget(root, child2);
    add_child_widget(root, child3);
    
    LayoutContext context = {
        .available_rect = {0, 0, 300, 100},
        .transform = NULL,
        .scale_factor = 1.0f,
        .reference_width = 300,
        .reference_height = 100
    };
    
    LayoutResult results[4];
    
    LayoutEngine* engine = layout_flex_get_engine();
    PkError err = engine->calculate(root, spec, &context, results);
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    // Children should be laid out horizontally
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, results[1].computed_rect.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, results[2].computed_rect.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, results[3].computed_rect.x);
    
    // All children should stretch to container height
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, results[1].computed_rect.height);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, results[2].computed_rect.height);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, results[3].computed_rect.height);
    
    free_widget_tree(root);
    layout_spec_destroy(spec);
}

// Test flex grow
void test_flex_grow(void) {
    LayoutSpec* spec = layout_flex_create(FLEX_DIRECTION_ROW);
    
    Widget* root = create_test_widget("root", 0, 0, 300, 100);
    Widget* child1 = create_test_widget("child1", 0, 0, 50, 50);
    Widget* child2 = create_test_widget("child2", 0, 0, 50, 50);
    
    add_child_widget(root, child1);
    add_child_widget(root, child2);
    
    // Set flex grow
    layout_flex_set_child(spec, child1, 1.0f, 1.0f, 50.0f);
    layout_flex_set_child(spec, child2, 2.0f, 1.0f, 50.0f);
    
    LayoutContext context = {
        .available_rect = {0, 0, 300, 100},
        .transform = NULL,
        .scale_factor = 1.0f,
        .reference_width = 300,
        .reference_height = 100
    };
    
    LayoutResult results[3];
    
    LayoutEngine* engine = layout_flex_get_engine();
    PkError err = engine->calculate(root, spec, &context, results);
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    // Available space: 300 - 100 (basis) = 200
    // child1 gets 1/3 * 200 = 66.67
    // child2 gets 2/3 * 200 = 133.33
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 116.67f, results[1].computed_rect.width);  // 50 + 66.67
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 183.33f, results[2].computed_rect.width);  // 50 + 133.33
    
    free_widget_tree(root);
    layout_spec_destroy(spec);
}

// Test justify content
void test_flex_justify_content(void) {
    LayoutSpec* spec = layout_flex_create(FLEX_DIRECTION_ROW);
    layout_flex_set_container(spec, FLEX_JUSTIFY_SPACE_BETWEEN, FLEX_ALIGN_CENTER);
    
    Widget* root = create_test_widget("root", 0, 0, 300, 100);
    Widget* child1 = create_test_widget("child1", 0, 0, 50, 50);
    Widget* child2 = create_test_widget("child2", 0, 0, 50, 50);
    Widget* child3 = create_test_widget("child3", 0, 0, 50, 50);
    
    add_child_widget(root, child1);
    add_child_widget(root, child2);
    add_child_widget(root, child3);
    
    LayoutContext context = {
        .available_rect = {0, 0, 300, 100},
        .transform = NULL,
        .scale_factor = 1.0f,
        .reference_width = 300,
        .reference_height = 100
    };
    
    LayoutResult results[4];
    
    LayoutEngine* engine = layout_flex_get_engine();
    PkError err = engine->calculate(root, spec, &context, results);
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    // With space-between, items should be at start, middle, and end
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, results[1].computed_rect.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 125.0f, results[2].computed_rect.x);  // (300-150)/2 + 50
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 250.0f, results[3].computed_rect.x);  // 300 - 50
    
    // With center alignment, items should be vertically centered
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, results[1].computed_rect.y);  // (100-50)/2
    
    free_widget_tree(root);
    layout_spec_destroy(spec);
}

// Test align items
void test_flex_align_items(void) {
    LayoutSpec* spec = layout_flex_create(FLEX_DIRECTION_ROW);
    layout_flex_set_container(spec, FLEX_JUSTIFY_START, FLEX_ALIGN_CENTER);
    
    Widget* root = create_test_widget("root", 0, 0, 300, 100);
    Widget* child1 = create_test_widget("child1", 0, 0, 50, 30);
    Widget* child2 = create_test_widget("child2", 0, 0, 50, 50);
    Widget* child3 = create_test_widget("child3", 0, 0, 50, 20);
    
    add_child_widget(root, child1);
    add_child_widget(root, child2);
    add_child_widget(root, child3);
    
    LayoutContext context = {
        .available_rect = {0, 0, 300, 100},
        .transform = NULL,
        .scale_factor = 1.0f,
        .reference_width = 300,
        .reference_height = 100
    };
    
    LayoutResult results[4];
    
    LayoutEngine* engine = layout_flex_get_engine();
    PkError err = engine->calculate(root, spec, &context, results);
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    // With center alignment, items should be vertically centered
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 35.0f, results[1].computed_rect.y);  // (100-30)/2
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, results[2].computed_rect.y);  // (100-50)/2
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 40.0f, results[3].computed_rect.y);  // (100-20)/2
    
    free_widget_tree(root);
    layout_spec_destroy(spec);
}

// Test column layout
void test_flex_column_layout(void) {
    LayoutSpec* spec = layout_flex_create(FLEX_DIRECTION_COLUMN);
    
    Widget* root = create_test_widget("root", 0, 0, 100, 300);
    Widget* child1 = create_test_widget("child1", 0, 0, 50, 50);
    Widget* child2 = create_test_widget("child2", 0, 0, 50, 50);
    Widget* child3 = create_test_widget("child3", 0, 0, 50, 50);
    
    add_child_widget(root, child1);
    add_child_widget(root, child2);
    add_child_widget(root, child3);
    
    LayoutContext context = {
        .available_rect = {0, 0, 100, 300},
        .transform = NULL,
        .scale_factor = 1.0f,
        .reference_width = 100,
        .reference_height = 300
    };
    
    LayoutResult results[4];
    
    LayoutEngine* engine = layout_flex_get_engine();
    PkError err = engine->calculate(root, spec, &context, results);
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    // Children should be laid out vertically
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, results[1].computed_rect.y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, results[2].computed_rect.y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, results[3].computed_rect.y);
    
    // All children should stretch to container width
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, results[1].computed_rect.width);
    
    free_widget_tree(root);
    layout_spec_destroy(spec);
}

// Test flex with gap
void test_flex_gap(void) {
    LayoutSpec* spec = layout_flex_create(FLEX_DIRECTION_ROW);
    layout_spec_set_gap(spec, 10.0f);
    
    Widget* root = create_test_widget("root", 0, 0, 300, 100);
    Widget* child1 = create_test_widget("child1", 0, 0, 50, 50);
    Widget* child2 = create_test_widget("child2", 0, 0, 50, 50);
    Widget* child3 = create_test_widget("child3", 0, 0, 50, 50);
    
    add_child_widget(root, child1);
    add_child_widget(root, child2);
    add_child_widget(root, child3);
    
    LayoutContext context = {
        .available_rect = {0, 0, 300, 100},
        .transform = NULL,
        .scale_factor = 1.0f,
        .reference_width = 300,
        .reference_height = 100
    };
    
    LayoutResult results[4];
    
    LayoutEngine* engine = layout_flex_get_engine();
    PkError err = engine->calculate(root, spec, &context, results);
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    // Children should have gap between them
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, results[1].computed_rect.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 60.0f, results[2].computed_rect.x);   // 50 + 10
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 120.0f, results[3].computed_rect.x);  // 110 + 10
    
    free_widget_tree(root);
    layout_spec_destroy(spec);
}

// Test reverse directions
void test_flex_reverse_direction(void) {
    LayoutSpec* spec = layout_flex_create(FLEX_DIRECTION_ROW_REVERSE);
    
    Widget* root = create_test_widget("root", 0, 0, 300, 100);
    Widget* child1 = create_test_widget("child1", 0, 0, 50, 50);
    Widget* child2 = create_test_widget("child2", 0, 0, 50, 50);
    Widget* child3 = create_test_widget("child3", 0, 0, 50, 50);
    
    add_child_widget(root, child1);
    add_child_widget(root, child2);
    add_child_widget(root, child3);
    
    LayoutContext context = {
        .available_rect = {0, 0, 300, 100},
        .transform = NULL,
        .scale_factor = 1.0f,
        .reference_width = 300,
        .reference_height = 100
    };
    
    LayoutResult results[4];
    
    LayoutEngine* engine = layout_flex_get_engine();
    PkError err = engine->calculate(root, spec, &context, results);
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    // Children should be laid out right to left
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 250.0f, results[1].computed_rect.x);  // 300 - 50
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 200.0f, results[2].computed_rect.x);  // 250 - 50
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 150.0f, results[3].computed_rect.x);  // 200 - 50
    
    free_widget_tree(root);
    layout_spec_destroy(spec);
}

// Test flex with padding
void test_flex_with_padding(void) {
    LayoutSpec* spec = layout_flex_create(FLEX_DIRECTION_ROW);
    layout_spec_set_padding(spec, 10.0f);
    
    Widget* root = create_test_widget("root", 0, 0, 300, 100);
    Widget* child1 = create_test_widget("child1", 0, 0, 50, 50);
    Widget* child2 = create_test_widget("child2", 0, 0, 50, 50);
    
    add_child_widget(root, child1);
    add_child_widget(root, child2);
    
    LayoutContext context = {
        .available_rect = {0, 0, 300, 100},
        .transform = NULL,
        .scale_factor = 1.0f,
        .reference_width = 300,
        .reference_height = 100
    };
    
    LayoutResult results[3];
    
    LayoutEngine* engine = layout_flex_get_engine();
    PkError err = engine->calculate(root, spec, &context, results);
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    // Children should be offset by padding
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, results[1].computed_rect.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, results[1].computed_rect.y);
    
    // Height should be constrained by padding
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 80.0f, results[1].computed_rect.height);  // 100 - 20
    
    free_widget_tree(root);
    layout_spec_destroy(spec);
}

// Test overflow clipping
void test_flex_overflow_clipping(void) {
    LayoutSpec* spec = layout_flex_create(FLEX_DIRECTION_ROW);
    spec->clip_overflow = true;
    
    Widget* root = create_test_widget("root", 0, 0, 100, 100);
    Widget* child1 = create_test_widget("child1", 0, 0, 60, 50);
    Widget* child2 = create_test_widget("child2", 0, 0, 60, 50);
    
    add_child_widget(root, child1);
    add_child_widget(root, child2);
    
    LayoutContext context = {
        .available_rect = {0, 0, 100, 100},
        .transform = NULL,
        .scale_factor = 1.0f,
        .reference_width = 100,
        .reference_height = 100
    };
    
    LayoutResult results[3];
    
    LayoutEngine* engine = layout_flex_get_engine();
    PkError err = engine->calculate(root, spec, &context, results);
    TEST_ASSERT_EQUAL(PK_OK, err);
    
    // Second child should be clipped
    TEST_ASSERT_FALSE(results[1].clipped);
    TEST_ASSERT_TRUE(results[2].clipped);
    
    free_widget_tree(root);
    layout_spec_destroy(spec);
}

// Register all tests
void test_flex_layout_register(void) {
    RUN_TEST(test_flex_layout_create);
    RUN_TEST(test_flex_container_properties);
    RUN_TEST(test_flex_row_layout);
    RUN_TEST(test_flex_grow);
    RUN_TEST(test_flex_justify_content);
    RUN_TEST(test_flex_align_items);
    RUN_TEST(test_flex_column_layout);
    RUN_TEST(test_flex_gap);
    RUN_TEST(test_flex_reverse_direction);
    RUN_TEST(test_flex_with_padding);
    RUN_TEST(test_flex_overflow_clipping);
}