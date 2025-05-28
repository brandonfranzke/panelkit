#include "unity/unity.h"
#include "ui/widget.h"
#include "ui/style/style_core.h"
#include "ui/style/style_templates.h"
#include "ui/style/style_observer.h"
#include <string.h>

// Mock event system functions
EventSystem* event_system_create(void) { return (EventSystem*)0x1234; }
void event_system_destroy(EventSystem* es) { (void)es; }
bool event_subscribe(EventSystem* es, const char* name, void* cb, void* ctx) {
    (void)es; (void)name; (void)cb; (void)ctx;
    return true;
}
void event_unsubscribe(EventSystem* es, const char* name, void* cb) {
    (void)es; (void)name; (void)cb;
}

// Observer callback data
static struct {
    int callback_count;
    Widget* widget;
    const Style* old_style;
    const Style* new_style;
} observer_data;

static void style_change_observer(Widget* widget, const Style* old_style,
                                const Style* new_style, void* user_data) {
    (void)user_data;
    observer_data.callback_count++;
    observer_data.widget = widget;
    observer_data.old_style = old_style;
    observer_data.new_style = new_style;
}


void test_widget_style_initialization(void) {
    Widget* widget = widget_create("test", WIDGET_TYPE_BUTTON);
    TEST_ASSERT_NOT_NULL(widget);
    
    // Should have no style initially
    TEST_ASSERT_NULL(widget->style);
    TEST_ASSERT_FALSE(widget->style_owned);
    TEST_ASSERT_NULL(widget->active_style);
    
    widget_destroy(widget);
}

void test_widget_set_style_ref(void) {
    Widget* widget = widget_create("test", WIDGET_TYPE_BUTTON);
    Style* style = style_template_button_blue();
    
    TEST_ASSERT_NOT_NULL(widget);
    TEST_ASSERT_NOT_NULL(style);
    
    widget_set_style_ref(widget, style);
    
    TEST_ASSERT_TRUE(style == widget->style);
    TEST_ASSERT_FALSE(widget->style_owned);
    TEST_ASSERT_NOT_NULL(widget->active_style);
    TEST_ASSERT_TRUE(&style->base == widget->active_style);
    
    // Widget should not destroy the style
    widget_destroy(widget);
    
    // We still own the style
    style_destroy(style);
}

void test_widget_set_style_owned(void) {
    Widget* widget = widget_create("test", WIDGET_TYPE_BUTTON);
    Style* style = style_template_button_red();
    
    TEST_ASSERT_NOT_NULL(widget);
    TEST_ASSERT_NOT_NULL(style);
    
    widget_set_style_owned(widget, style);
    
    TEST_ASSERT_TRUE(style == widget->style);
    TEST_ASSERT_TRUE(widget->style_owned);
    TEST_ASSERT_NOT_NULL(widget->active_style);
    
    // Widget should destroy the style
    widget_destroy(widget);
    // Don't destroy style - widget owns it
}

void test_widget_style_state_changes(void) {
    Widget* widget = widget_create("test", WIDGET_TYPE_BUTTON);
    Style* style = style_template_button_blue();
    
    widget_set_style_ref(widget, style);
    
    // Normal state
    const StyleBase* normal = widget_get_active_style(widget);
    TEST_ASSERT_TRUE(&style->base == normal);
    
    // Hover state
    widget_set_state(widget, WIDGET_STATE_HOVERED, true);
    const StyleBase* hover = widget_get_active_style(widget);
    TEST_ASSERT_NOT_NULL(hover);
    TEST_ASSERT_TRUE(normal != hover);
    TEST_ASSERT_TRUE(style->hover == hover);
    
    // Pressed state (higher priority than hover)
    widget_set_state(widget, WIDGET_STATE_PRESSED, true);
    const StyleBase* pressed = widget_get_active_style(widget);
    TEST_ASSERT_NOT_NULL(pressed);
    TEST_ASSERT_TRUE(hover != pressed);
    TEST_ASSERT_TRUE(style->pressed == pressed);
    
    // Disabled state (highest priority)
    widget_set_state(widget, WIDGET_STATE_DISABLED, true);
    const StyleBase* disabled = widget_get_active_style(widget);
    TEST_ASSERT_NOT_NULL(disabled);
    TEST_ASSERT_TRUE(style->disabled == disabled);
    
    widget_destroy(widget);
    style_destroy(style);
}

void test_widget_style_observer_notification(void) {
    Widget* widget = widget_create("test", WIDGET_TYPE_BUTTON);
    
    // Register observer
    style_observer_register_widget(widget, style_change_observer, NULL);
    
    // Set initial style
    Style* style1 = style_template_button_red();
    widget_set_style_owned(widget, style1);
    
    TEST_ASSERT_TRUE(1 == observer_data.callback_count);
    TEST_ASSERT_TRUE(widget == observer_data.widget);
    TEST_ASSERT_NULL(observer_data.old_style);
    TEST_ASSERT_TRUE(style1 == observer_data.new_style);
    
    // Change style
    Style* style2 = style_template_button_green();
    widget_set_style_owned(widget, style2);
    
    TEST_ASSERT_TRUE(2 == observer_data.callback_count);
    TEST_ASSERT_NULL(observer_data.old_style); // Old style was destroyed
    TEST_ASSERT_TRUE(style2 == observer_data.new_style);
    
    widget_destroy(widget);
}

void test_widget_style_validation(void) {
    Widget* widget = widget_create("test", WIDGET_TYPE_BUTTON);
    
    // Create invalid style
    Style* style = style_create();
    style->base.font_size = 0; // Invalid
    
    // Should reject invalid style
    widget_set_style_owned(widget, style);
    
    // Style should not be set
    TEST_ASSERT_NULL(widget->style);
    TEST_ASSERT_FALSE(widget->style_owned);
    
    // Check error was set
    PkError err = pk_get_last_error();
    TEST_ASSERT_TRUE(PK_ERROR_INVALID_PARAM == err);
    
    widget_destroy(widget);
}

void test_widget_style_null_handling(void) {
    Widget* widget = widget_create("test", WIDGET_TYPE_BUTTON);
    
    // Setting NULL style should work
    widget_set_style_ref(widget, NULL);
    TEST_ASSERT_NULL(widget->style);
    TEST_ASSERT_NULL(widget->active_style);
    
    // Update active style with NULL should work
    widget_update_active_style(widget);
    TEST_ASSERT_NULL(widget->active_style);
    
    // Get active style should handle NULL
    const StyleBase* active = widget_get_active_style(widget);
    TEST_ASSERT_NULL(active);
    
    widget_destroy(widget);
}

void test_widget_style_replacement(void) {
    Widget* widget = widget_create("test", WIDGET_TYPE_BUTTON);
    
    // Set initial style (referenced)
    Style* style1 = style_template_button_red();
    widget_set_style_ref(widget, style1);
    TEST_ASSERT_TRUE(style1 == widget->style);
    TEST_ASSERT_FALSE(widget->style_owned);
    
    // Replace with owned style
    Style* style2 = style_template_button_blue();
    widget_set_style_owned(widget, style2);
    TEST_ASSERT_TRUE(style2 == widget->style);
    TEST_ASSERT_TRUE(widget->style_owned);
    
    // Replace owned with another owned
    Style* style3 = style_template_button_green();
    widget_set_style_owned(widget, style3);
    TEST_ASSERT_TRUE(style3 == widget->style);
    TEST_ASSERT_TRUE(widget->style_owned);
    
    // Replace owned with referenced
    widget_set_style_ref(widget, style1);
    TEST_ASSERT_TRUE(style1 == widget->style);
    TEST_ASSERT_FALSE(widget->style_owned);
    
    widget_destroy(widget);
    style_destroy(style1);
}

void test_widget_style_integration_register(void) {
    RUN_TEST(test_widget_set_style_ref);
    RUN_TEST(test_widget_set_style_owned);
    RUN_TEST(test_widget_style_state_changes);
    RUN_TEST(test_widget_style_observer_notification);
    RUN_TEST(test_widget_style_validation);
    RUN_TEST(test_widget_style_null_handling);
    RUN_TEST(test_widget_style_replacement);
}