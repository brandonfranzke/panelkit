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

// Global test setup/teardown
void setUp(void) {
    // Called before each test
}

void tearDown(void) {
    // Called after each test
}

void suiteSetUp(void) {
    // Called once at start
    printf("\n=== PanelKit Test Suite ===\n");
}

int suiteTearDown(int num_failures) {
    // Called once at end
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
    }
    
    // Get results
    int failures = UnityEnd();
    return suiteTearDown(failures);
}