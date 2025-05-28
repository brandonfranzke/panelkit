#include "unity/unity.h"
#include "ui/style/style_core.h"
#include <string.h>

// Widget state flags (matching style_core.c)
#define WIDGET_STATE_DISABLED   (1 << 0)
#define WIDGET_STATE_PRESSED    (1 << 1)
#define WIDGET_STATE_HOVER      (1 << 2)
#define WIDGET_STATE_FOCUSED    (1 << 3)

void test_style_create_destroy(void) {
    Style* style = style_create();
    TEST_ASSERT_NOT_NULL(style);
    
    // Check defaults
    TEST_ASSERT_EQUAL(255, style->base.background.r);
    TEST_ASSERT_EQUAL(255, style->base.background.g);
    TEST_ASSERT_EQUAL(255, style->base.background.b);
    TEST_ASSERT_EQUAL(255, style->base.background.a);
    
    TEST_ASSERT_EQUAL(0, style->base.foreground.r);
    TEST_ASSERT_EQUAL(0, style->base.foreground.g);
    TEST_ASSERT_EQUAL(0, style->base.foreground.b);
    TEST_ASSERT_EQUAL(255, style->base.foreground.a);
    
    TEST_ASSERT_EQUAL_STRING("default", style->base.font_family);
    TEST_ASSERT_EQUAL(16, style->base.font_size);
    TEST_ASSERT_EQUAL(400, style->base.font_weight);
    
    TEST_ASSERT_NULL(style->hover);
    TEST_ASSERT_NULL(style->pressed);
    TEST_ASSERT_NULL(style->disabled);
    TEST_ASSERT_NULL(style->focused);
    TEST_ASSERT_FALSE(style->states_owned);
    
    style_destroy(style);
}

void test_style_create_from_template(void) {
    // Create template
    Style template = {0};
    style_base_copy(&template.base, &(StyleBase){
        .background = {100, 150, 200, 255},
        .foreground = {50, 50, 50, 255},
        .font_family = "custom-font",
        .font_size = 24,
        .font_weight = 700,
        .padding = {10, 20, 10, 20},
        .border_radius = 5
    });
    strcpy(template.name, "TestTemplate");
    
    // Create from template
    Style* style = style_create_from(&template);
    TEST_ASSERT_NOT_NULL(style);
    
    // Check copied values
    TEST_ASSERT_EQUAL(100, style->base.background.r);
    TEST_ASSERT_EQUAL(150, style->base.background.g);
    TEST_ASSERT_EQUAL(200, style->base.background.b);
    TEST_ASSERT_EQUAL_STRING("custom-font", style->base.font_family);
    TEST_ASSERT_EQUAL(24, style->base.font_size);
    TEST_ASSERT_EQUAL(700, style->base.font_weight);
    TEST_ASSERT_EQUAL(10, style->base.padding.top);
    TEST_ASSERT_EQUAL(20, style->base.padding.right);
    TEST_ASSERT_EQUAL_STRING("TestTemplate", style->name);
    
    // State variants should not be copied
    TEST_ASSERT_NULL(style->hover);
    TEST_ASSERT_NULL(style->pressed);
    TEST_ASSERT_NULL(style->disabled);
    TEST_ASSERT_NULL(style->focused);
    TEST_ASSERT_FALSE(style->states_owned);
    
    style_destroy(style);
}

void test_style_base_operations(void) {
    // Create and test StyleBase
    StyleBase* base = style_base_create();
    TEST_ASSERT_NOT_NULL(base);
    
    // Modify some values
    base->background = pk_color_create(50, 100, 150, 200);
    base->border = border_solid(PK_COLOR_BLACK, 2);
    base->padding = spacing_uniform(15);
    
    // Create copy
    StyleBase* copy = style_base_create_from(base);
    TEST_ASSERT_NOT_NULL(copy);
    
    // Verify copy
    TEST_ASSERT(pk_color_equals(base->background, copy->background));
    TEST_ASSERT(pk_color_equals(base->border.color, copy->border.color));
    TEST_ASSERT_EQUAL(base->border.width, copy->border.width);
    TEST_ASSERT_EQUAL(base->padding.top, copy->padding.top);
    TEST_ASSERT_EQUAL(base->padding.right, copy->padding.right);
    
    // Test equality
    TEST_ASSERT_TRUE(style_base_equals(base, copy));
    
    // Modify copy
    copy->background.r = 100;
    TEST_ASSERT_FALSE(style_base_equals(base, copy));
    
    style_base_destroy(base);
    style_base_destroy(copy);
}

void test_style_state_resolution(void) {
    Style style = {0};
    style_base_copy(&style.base, &(StyleBase){
        .background = {255, 255, 255, 255}
    });
    
    // Create state variants
    StyleBase hover = {.background = {200, 200, 200, 255}};
    StyleBase pressed = {.background = {150, 150, 150, 255}};
    StyleBase disabled = {.background = {100, 100, 100, 255}};
    StyleBase focused = {.background = {50, 50, 50, 255}};
    
    style.hover = &hover;
    style.pressed = &pressed;
    style.disabled = &disabled;
    style.focused = &focused;
    
    // Test state resolution priority
    const StyleBase* resolved;
    
    // Base state
    resolved = style_resolve_state(&style, 0);
    TEST_ASSERT(resolved == &style.base);
    
    // Hover
    resolved = style_resolve_state(&style, WIDGET_STATE_HOVER);
    TEST_ASSERT(resolved == &hover);
    
    // Pressed (higher priority than hover)
    resolved = style_resolve_state(&style, WIDGET_STATE_HOVER | WIDGET_STATE_PRESSED);
    TEST_ASSERT(resolved == &pressed);
    
    // Disabled (highest priority)
    resolved = style_resolve_state(&style, WIDGET_STATE_HOVER | WIDGET_STATE_PRESSED | WIDGET_STATE_DISABLED);
    TEST_ASSERT(resolved == &disabled);
    
    // Focused
    resolved = style_resolve_state(&style, WIDGET_STATE_FOCUSED);
    TEST_ASSERT(resolved == &focused);
    
    // Focused + hover (focused has higher priority)
    resolved = style_resolve_state(&style, WIDGET_STATE_FOCUSED | WIDGET_STATE_HOVER);
    TEST_ASSERT(resolved == &focused);
}

void test_style_utility_functions(void) {
    // Test spacing helpers
    Spacing uniform = spacing_uniform(10);
    TEST_ASSERT_EQUAL(10, uniform.top);
    TEST_ASSERT_EQUAL(10, uniform.right);
    TEST_ASSERT_EQUAL(10, uniform.bottom);
    TEST_ASSERT_EQUAL(10, uniform.left);
    
    Spacing symmetric = spacing_symmetric(5, 15);
    TEST_ASSERT_EQUAL(5, symmetric.top);
    TEST_ASSERT_EQUAL(15, symmetric.right);
    TEST_ASSERT_EQUAL(5, symmetric.bottom);
    TEST_ASSERT_EQUAL(15, symmetric.left);
    
    Spacing custom = spacing_create(1, 2, 3, 4);
    TEST_ASSERT_EQUAL(1, custom.top);
    TEST_ASSERT_EQUAL(2, custom.right);
    TEST_ASSERT_EQUAL(3, custom.bottom);
    TEST_ASSERT_EQUAL(4, custom.left);
    
    // Test border helpers
    Border none = border_none();
    TEST_ASSERT_EQUAL(0, none.width);
    TEST_ASSERT_EQUAL(BORDER_STYLE_NONE, none.style);
    
    Border solid = border_solid(PK_COLOR_RED, 3);
    TEST_ASSERT_EQUAL(3, solid.width);
    TEST_ASSERT_EQUAL(BORDER_STYLE_SOLID, solid.style);
    TEST_ASSERT(pk_color_equals(PK_COLOR_RED, solid.color));
    
    // Test shadow helpers
    Shadow no_shadow = shadow_none();
    TEST_ASSERT_EQUAL(0, no_shadow.offset_x);
    TEST_ASSERT_EQUAL(0, no_shadow.offset_y);
    TEST_ASSERT_EQUAL(0, no_shadow.blur_radius);
    
    Shadow drop_shadow = shadow_create(PK_COLOR_BLACK, 2, 4, 8);
    TEST_ASSERT_EQUAL(2, drop_shadow.offset_x);
    TEST_ASSERT_EQUAL(4, drop_shadow.offset_y);
    TEST_ASSERT_EQUAL(8, drop_shadow.blur_radius);
    TEST_ASSERT(pk_color_equals(PK_COLOR_BLACK, drop_shadow.color));
}

void test_style_with_owned_states(void) {
    Style* style = style_create();
    TEST_ASSERT_NOT_NULL(style);
    
    // Create owned state variants
    style->hover = style_base_create();
    style->pressed = style_base_create();
    style->disabled = style_base_create();
    style->focused = style_base_create();
    style->states_owned = true;
    
    // Modify states
    style->hover->background = PK_COLOR_BLUE;
    style->pressed->background = PK_COLOR_GREEN;
    style->disabled->background = PK_COLOR_RED;
    style->focused->background = PK_COLOR_YELLOW;
    
    // Verify states
    TEST_ASSERT_NOT_NULL(style->hover);
    TEST_ASSERT_NOT_NULL(style->pressed);
    TEST_ASSERT_NOT_NULL(style->disabled);
    TEST_ASSERT_NOT_NULL(style->focused);
    
    // Destroy should free owned states
    style_destroy(style);
    // If this doesn't crash, the memory was properly freed
}

void test_style_copy_operations(void) {
    Style src = {0};
    Style dest = {0};
    
    // Setup source
    src.base.background = PK_COLOR_BLUE;
    src.base.font_size = 24;
    strcpy(src.name, "SourceStyle");
    
    // Setup dest with different values
    dest.base.background = PK_COLOR_RED;
    dest.base.font_size = 12;
    
    // Create state for dest (should be preserved)
    StyleBase hover_state = {0};
    dest.hover = &hover_state;
    dest.states_owned = true;
    
    // Copy
    style_copy(&dest, &src);
    
    // Verify base copied
    TEST_ASSERT(pk_color_equals(src.base.background, dest.base.background));
    TEST_ASSERT_EQUAL(src.base.font_size, dest.base.font_size);
    TEST_ASSERT_EQUAL_STRING(src.name, dest.name);
    
    // Verify states preserved
    TEST_ASSERT(dest.hover == &hover_state);
    TEST_ASSERT_TRUE(dest.states_owned);
}

// Test registration
void test_style_core_register(void) {
    RUN_TEST(test_style_create_destroy);
    RUN_TEST(test_style_create_from_template);
    RUN_TEST(test_style_base_operations);
    RUN_TEST(test_style_state_resolution);
    RUN_TEST(test_style_utility_functions);
    RUN_TEST(test_style_with_owned_states);
    RUN_TEST(test_style_copy_operations);
}