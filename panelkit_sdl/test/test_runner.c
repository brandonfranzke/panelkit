/**
 * @file test_runner.c
 * @brief Main test runner for PanelKit test suite
 */

#include "unity.h"
#include <stdio.h>
#include <string.h>

// Test module declarations
extern void test_layout_core_register(void);
extern void test_absolute_layout_register(void);
extern void test_flex_layout_register(void);
extern void test_style_resolution_register(void);
extern void test_font_manager_simple_register(void);
extern void test_color_utils_register(void);
extern void test_style_core_register(void);
extern void test_style_constants_register(void);
extern void test_style_validation_register(void);
extern void test_style_templates_register(void);
extern void test_style_observer_register(void);
extern void test_style_system_register(void);
extern void test_widget_style_integration_register(void);

// Forward declarations for cleanup functions
void style_template_clear_registry(void);
void style_observer_clear_all(void);

// Global test setup/teardown
void setUp(void) {
    // Called before each test
    // Clear any global state
    style_template_clear_registry();
    style_observer_clear_all();
}

void tearDown(void) {
    // Called after each test
    // Clean up any allocated resources
    style_template_clear_registry();
    style_observer_clear_all();
}

void suiteSetUp(void) {
    // Called once at start
    printf("\n=== PanelKit Test Suite ===\n");
}

int suiteTearDown(int num_failures) {
    // Called once at end
    printf("\n=== Test Summary: %d failures ===\n", num_failures);
    return num_failures;
}

int main(int argc, char* argv[]) {
    // Initialize Unity
    UnityBegin("PanelKit Tests");
    
    // Check for specific test suite argument
    const char* suite = (argc > 1) ? argv[1] : "all";
    
    suiteSetUp();
    
    // Run test suites based on argument
    if (strcmp(suite, "all") == 0 || strcmp(suite, "layout") == 0) {
        printf("\n--- Layout Tests ---\n");
        test_layout_core_register();
        test_absolute_layout_register();
        test_flex_layout_register();
    }
    
    if (strcmp(suite, "all") == 0 || strcmp(suite, "style") == 0) {
        printf("\n--- Style Tests ---\n");
        test_style_resolution_register();
        // TODO: Fix font manager tests
        // test_font_manager_simple_register();
        test_color_utils_register();
        test_style_core_register();
        test_style_constants_register();
        test_style_validation_register();
        test_style_templates_register();
        test_style_observer_register();
        test_style_system_register();
        test_widget_style_integration_register();
    }
    
    // Individual style test suites for debugging
    if (strcmp(suite, "style_resolution") == 0) {
        printf("\n--- Style Resolution Tests ---\n");
        test_style_resolution_register();
    }
    if (strcmp(suite, "color_utils") == 0) {
        printf("\n--- Color Utils Tests ---\n");
        test_color_utils_register();
    }
    if (strcmp(suite, "style_core") == 0) {
        printf("\n--- Style Core Tests ---\n");
        test_style_core_register();
    }
    if (strcmp(suite, "style_constants") == 0) {
        printf("\n--- Style Constants Tests ---\n");
        test_style_constants_register();
    }
    if (strcmp(suite, "style_validation") == 0) {
        printf("\n--- Style Validation Tests ---\n");
        test_style_validation_register();
    }
    if (strcmp(suite, "style_templates") == 0) {
        printf("\n--- Style Templates Tests ---\n");
        test_style_templates_register();
    }
    if (strcmp(suite, "style_observer") == 0) {
        printf("\n--- Style Observer Tests ---\n");
        test_style_observer_register();
    }
    if (strcmp(suite, "style_system") == 0) {
        printf("\n--- Style System Tests ---\n");
        test_style_system_register();
    }
    if (strcmp(suite, "widget_style") == 0) {
        printf("\n--- Widget Style Integration Tests ---\n");
        test_widget_style_integration_register();
    }
    
    // Get results
    int failures = UnityEnd();
    int result = suiteTearDown(failures);
    return result;
}