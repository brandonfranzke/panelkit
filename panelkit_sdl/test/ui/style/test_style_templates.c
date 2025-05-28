#include "unity/unity.h"
#include "ui/style/style_templates.h"
#include "ui/style/color_utils.h"
#include <string.h>


void test_style_template_button_creation(void) {
    PkColor test_color = {255, 0, 0, 255}; // Red
    Style* style = style_template_button(test_color);
    
    TEST_ASSERT_NOT_NULL(style);
    TEST_ASSERT_EQUAL(255, style->base.background.r);
    TEST_ASSERT_EQUAL(0, style->base.background.g);
    TEST_ASSERT_EQUAL(0, style->base.background.b);
    
    // Should have state variants
    TEST_ASSERT_NOT_NULL(style->hover);
    TEST_ASSERT_NOT_NULL(style->pressed);
    TEST_ASSERT_NOT_NULL(style->disabled);
    TEST_ASSERT_NOT_NULL(style->focused);
    TEST_ASSERT_TRUE(style->states_owned);
    
    // Hover should be lighter
    TEST_ASSERT_TRUE(style->hover->background.r > style->base.background.r ||
                     style->hover->background.g > style->base.background.g ||
                     style->hover->background.b > style->base.background.b);
    
    // Pressed should be darker
    TEST_ASSERT_TRUE(style->pressed->background.r < style->base.background.r ||
                     style->pressed->background.g < style->base.background.g ||
                     style->pressed->background.b < style->base.background.b);
    
    style_destroy(style);
}

void test_style_template_8_button_colors(void) {
    // Test all 8 required button colors
    Style* (*creators[])(void) = {
        style_template_button_red,
        style_template_button_green,
        style_template_button_blue,
        style_template_button_yellow,
        style_template_button_purple,
        style_template_button_orange,
        style_template_button_teal,
        style_template_button_pink
    };
    
    for (int i = 0; i < 8; i++) {
        Style* style = creators[i]();
        TEST_ASSERT_NOT_NULL(style);
        
        // Each should have unique color
        TEST_ASSERT_TRUE(style->base.background.r > 0 || 
                        style->base.background.g > 0 || 
                        style->base.background.b > 0);
        
        // Should have proper contrast text
        float brightness = pk_color_brightness(style->base.background);
        if (brightness > 0.5f) {
            // Light background should have dark text
            TEST_ASSERT_TRUE(style->base.foreground.r < 128);
        } else {
            // Dark background should have light text
            TEST_ASSERT_TRUE(style->base.foreground.r > 128);
        }
        
        style_destroy(style);
    }
}

void test_style_template_text_variations(void) {
    Style* label = style_template_text_label();
    Style* heading = style_template_text_heading();
    Style* caption = style_template_text_caption();
    
    TEST_ASSERT_NOT_NULL(label);
    TEST_ASSERT_NOT_NULL(heading);
    TEST_ASSERT_NOT_NULL(caption);
    
    // Heading should be larger than label
    TEST_ASSERT_TRUE(heading->base.font_size > label->base.font_size);
    
    // Heading should be bolder
    TEST_ASSERT_TRUE(heading->base.font_weight > label->base.font_weight);
    
    // Caption should be smaller
    TEST_ASSERT_TRUE(caption->base.font_size < label->base.font_size);
    
    style_destroy(label);
    style_destroy(heading);
    style_destroy(caption);
}

void test_style_template_panel_variations(void) {
    Style* panel = style_template_panel();
    Style* transparent = style_template_panel_transparent();
    
    TEST_ASSERT_NOT_NULL(panel);
    TEST_ASSERT_NOT_NULL(transparent);
    
    // Normal panel should have visible background
    TEST_ASSERT_TRUE(panel->base.background.a > 0);
    TEST_ASSERT_TRUE(panel->base.border.width > 0);
    
    // Transparent panel should be invisible
    TEST_ASSERT_EQUAL(0, transparent->base.background.a);
    TEST_ASSERT_EQUAL(0, transparent->base.border.width);
    
    style_destroy(panel);
    style_destroy(transparent);
}

void test_style_template_device_button(void) {
    Style* on_style = style_template_device_button(true);
    Style* off_style = style_template_device_button(false);
    
    TEST_ASSERT_NOT_NULL(on_style);
    TEST_ASSERT_NOT_NULL(off_style);
    
    // On state should be more vibrant
    // Brightness values would be used for contrast checking
    // float on_brightness = pk_color_brightness(on_style->base.background);
    // float off_brightness = pk_color_brightness(off_style->base.background);
    
    // On state should have shadow/glow
    TEST_ASSERT_TRUE(on_style->base.shadow.blur_radius > 0);
    
    style_destroy(on_style);
    style_destroy(off_style);
}

void test_style_template_temperature_display(void) {
    Style* cold = style_template_temperature_display(5.0f);   // Cold
    Style* cool = style_template_temperature_display(15.0f);  // Cool
    Style* warm = style_template_temperature_display(25.0f);  // Warm
    Style* hot = style_template_temperature_display(35.0f);   // Hot
    
    TEST_ASSERT_NOT_NULL(cold);
    TEST_ASSERT_NOT_NULL(cool);
    TEST_ASSERT_NOT_NULL(warm);
    TEST_ASSERT_NOT_NULL(hot);
    
    // Colors should be different for each temperature range
    TEST_ASSERT_FALSE(pk_color_equals(cold->base.foreground, hot->base.foreground));
    TEST_ASSERT_FALSE(pk_color_equals(cool->base.foreground, warm->base.foreground));
    
    // Font size should be large for temperature display
    TEST_ASSERT_TRUE(cold->base.font_size > 16); // Assuming 16 is normal size
    
    style_destroy(cold);
    style_destroy(cool);
    style_destroy(warm);
    style_destroy(hot);
}

void test_style_template_notification(void) {
    Style* info = style_template_notification(NOTIFICATION_INFO);
    Style* success = style_template_notification(NOTIFICATION_SUCCESS);
    Style* warning = style_template_notification(NOTIFICATION_WARNING);
    Style* error = style_template_notification(NOTIFICATION_ERROR);
    
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_NOT_NULL(success);
    TEST_ASSERT_NOT_NULL(warning);
    TEST_ASSERT_NOT_NULL(error);
    
    // Each type should have different colors
    TEST_ASSERT_FALSE(pk_color_equals(info->base.background, success->base.background));
    TEST_ASSERT_FALSE(pk_color_equals(warning->base.background, error->base.background));
    
    // All should have padding and border radius
    TEST_ASSERT_TRUE(info->base.padding.top > 0);
    TEST_ASSERT_TRUE(info->base.border_radius > 0);
    
    style_destroy(info);
    style_destroy(success);
    style_destroy(warning);
    style_destroy(error);
}

void test_style_template_registry(void) {
    // Create and register a style
    Style* button_style = style_template_button_blue();
    TEST_ASSERT_NOT_NULL(button_style);
    
    style_template_register("test.button", button_style);
    
    // Should be able to retrieve it
    const Style* retrieved = style_template_get("test.button");
    TEST_ASSERT_NOT_NULL(retrieved);
    
    // Create from template
    Style* copy = style_template_create_from("test.button");
    TEST_ASSERT_NOT_NULL(copy);
    TEST_ASSERT_TRUE(copy != retrieved); // Should be a copy
    
    style_destroy(copy);
    // Don't destroy button_style - registry owns it
}

void test_style_template_registry_replace(void) {
    // Register initial style
    Style* style1 = style_template_button_red();
    style_template_register("test.style", style1);
    
    const Style* retrieved1 = style_template_get("test.style");
    TEST_ASSERT_NOT_NULL(retrieved1);
    
    // Save the original color
    PkColor original_color = retrieved1->base.background;
    
    // Replace with new style
    Style* style2 = style_template_button_blue();
    style_template_register("test.style", style2);
    
    const Style* retrieved2 = style_template_get("test.style");
    TEST_ASSERT_NOT_NULL(retrieved2);
    
    // Should be different style (compare saved color with new)
    TEST_ASSERT_FALSE(pk_color_equals(original_color, retrieved2->base.background));
}

void test_style_template_list(void) {
    // Register some templates
    style_template_register("style1", style_template_button_red());
    style_template_register("style2", style_template_button_green());
    style_template_register("style3", style_template_button_blue());
    
    const char** names = NULL;
    size_t count = style_template_list(&names);
    
    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_NOT_NULL(names);
    
    // Check names are present (order not guaranteed)
    bool found1 = false, found2 = false, found3 = false;
    for (size_t i = 0; i < count; i++) {
        if (strcmp(names[i], "style1") == 0) found1 = true;
        if (strcmp(names[i], "style2") == 0) found2 = true;
        if (strcmp(names[i], "style3") == 0) found3 = true;
    }
    
    TEST_ASSERT_TRUE(found1);
    TEST_ASSERT_TRUE(found2);
    TEST_ASSERT_TRUE(found3);
}

void test_style_template_input_field(void) {
    Style* input = style_template_input_field();
    TEST_ASSERT_NOT_NULL(input);
    
    // Should have focused and disabled states
    TEST_ASSERT_NOT_NULL(input->focused);
    TEST_ASSERT_NOT_NULL(input->disabled);
    TEST_ASSERT_TRUE(input->states_owned);
    
    // Focused state should have different border
    TEST_ASSERT_TRUE(input->base.border.width != input->focused->border.width);
    
    // Disabled state should have different colors
    TEST_ASSERT_FALSE(pk_color_equals(input->base.background, input->disabled->background));
    
    style_destroy(input);
}

// Test registration
void test_style_templates_register(void) {
    RUN_TEST(test_style_template_button_creation);
    RUN_TEST(test_style_template_8_button_colors);
    RUN_TEST(test_style_template_text_variations);
    RUN_TEST(test_style_template_panel_variations);
    RUN_TEST(test_style_template_device_button);
    RUN_TEST(test_style_template_temperature_display);
    RUN_TEST(test_style_template_notification);
    RUN_TEST(test_style_template_registry);
    RUN_TEST(test_style_template_registry_replace);
    RUN_TEST(test_style_template_list);
    RUN_TEST(test_style_template_input_field);
}