# Testing Strategy and Architecture

## Overview

This document defines the testing strategy, architecture, and implementation approach for PanelKit. It captures decisions made during initial planning and serves as the primary reference for test development.

## Core Philosophy

1. **Test What Matters**: Focus on logic and calculations, not UI rendering
2. **Lightweight Approach**: Avoid complex UI testing harnesses
3. **Developer-Friendly**: Easy to write, run, and understand tests
4. **Embedded-Compatible**: Testing approach suitable for embedded targets
5. **Incremental Adoption**: Start with new features, expand coverage over time

## Framework Selection: Unity

After evaluating options (Unity, Check, MinUnit), Unity was selected for these reasons:

1. **Single Header/Source**: Easy integration, minimal dependencies
2. **Embedded-Friendly**: Designed for embedded systems (our target)
3. **Clear Syntax**: Readable assertions like `TEST_ASSERT_EQUAL`
4. **Active Development**: Well-maintained and documented
5. **SDL Compatible**: No conflicts with graphics stack

## Test Architecture

### Directory Structure

```
test/
├── Makefile                    # Separate test build system
├── unity/                      # Unity framework files
│   ├── unity.h
│   ├── unity.c
│   └── unity_internals.h
├── test_runner.c              # Main test runner
├── ui/
│   ├── layout/                # Layout engine tests
│   │   ├── test_layout_core.c
│   │   ├── test_flex_layout.c
│   │   ├── test_grid_layout.c
│   │   └── test_absolute_layout.c
│   ├── style/                 # Style system tests
│   │   ├── test_style_resolution.c
│   │   ├── test_property_cascade.c
│   │   └── test_theme_constants.c
│   └── integration/           # Widget integration tests
│       └── test_widget_layout.c
└── visual/                    # Optional visual verification
    └── layout_visual_test.c   # Standalone visual test app
```

### Test Organization Principles

1. **Mirror Source Structure**: Test paths match source paths
2. **One Test File Per Module**: Clear mapping between code and tests
3. **Descriptive Test Names**: `test_flex_layout_distributes_space_evenly()`
4. **Setup/Teardown**: Use Unity's setUp/tearDown for common initialization

## Implementation Strategy

### Phase 1: Infrastructure (Current)

1. Add Unity framework to project
2. Create test Makefile with proper paths
3. Set up basic test runner
4. Create first example test
5. Add `make test` target to main Makefile

### Phase 2: Layout System Tests

Focus on testing calculations, not rendering:

```c
// Example: Test flex layout calculation
void test_flex_row_equal_distribution(void) {
    // Arrange
    LayoutSpec* spec = layout_create_flex(FLEX_ROW);
    layout_add_child(spec, "child1", FLEX_GROW, 1);
    layout_add_child(spec, "child2", FLEX_GROW, 1);
    
    // Act
    LayoutResult* result = layout_calculate(spec, 200, 100);
    
    // Assert
    TEST_ASSERT_EQUAL_FLOAT(100.0f, result->children[0].width);
    TEST_ASSERT_EQUAL_FLOAT(100.0f, result->children[1].width);
    
    // Cleanup
    layout_spec_destroy(spec);
    layout_result_destroy(result);
}
```

### Phase 3: Style System Tests

Test property resolution and cascading:

```c
void test_style_property_override_precedence(void) {
    // Test that widget-specific styles override theme defaults
    Style* theme_style = style_create_from_theme(THEME_BUTTON);
    Style* widget_style = style_create_override(theme_style);
    style_set_color(widget_style, STYLE_BG_COLOR, COLOR_RED);
    
    SDL_Color resolved = style_get_color(widget_style, STYLE_BG_COLOR);
    TEST_ASSERT_EQUAL_UINT8(255, resolved.r);
    TEST_ASSERT_EQUAL_UINT8(0, resolved.g);
}
```

### Phase 4: Integration Tests

Test widgets with layout/style systems:

```c
void test_button_respects_flex_layout(void) {
    // Create button in flex container
    // Verify button bounds match layout calculation
}
```

## Test Execution

### Running Tests

```bash
# Run all tests
make test

# Run specific test suite
make test-layout

# Run with verbose output
make test VERBOSE=1

# Run tests on target (if supported)
make test-target
```

### Continuous Integration

- Tests should run on every commit
- Fail the build on test failure
- Generate test coverage reports (optional)

## What We Test

### DO Test

1. **Layout Calculations**: Input dimensions → output positions
2. **Style Resolution**: Property lookups and cascading
3. **State Management**: Widget state changes
4. **Event Propagation**: Event flow through widget tree
5. **Configuration Parsing**: YAML → internal structures
6. **Error Conditions**: Null checks, invalid inputs

### DON'T Test

1. **Pixel-Perfect Rendering**: Leave to manual testing
2. **SDL Internals**: Trust SDL to work correctly
3. **Visual Appearance**: Colors, fonts rendering correctly
4. **Touch/Mouse Events**: SDL event generation
5. **Performance**: Unless specific benchmarks needed

## Best Practices

1. **Test One Thing**: Each test validates a single behavior
2. **Arrange-Act-Assert**: Clear test structure
3. **Descriptive Names**: Test name explains what and why
4. **Fast Tests**: Milliseconds, not seconds
5. **Independent Tests**: No shared state between tests
6. **Deterministic**: Same result every time

## Visual Testing

For aspects that need visual verification:

1. **Separate Visual Test App**: `test/visual/layout_visual_test`
2. **Not Part of Automated Suite**: Run manually
3. **Interactive Controls**: Adjust parameters live
4. **Screenshot Comparison**: Optional, if needed

## Future Considerations

1. **Code Coverage**: Add gcov/lcov integration
2. **Performance Tests**: Benchmark critical paths
3. **Fuzz Testing**: Random input generation
4. **Memory Testing**: Valgrind integration
5. **Cross-Platform**: Test on target hardware

## Success Metrics

1. **Test Execution Time**: Full suite < 5 seconds
2. **Coverage**: 80%+ for new code
3. **Reliability**: No flaky tests
4. **Maintainability**: Tests updated with code
5. **Documentation**: Clear test purposes

## Common Patterns

### Testing Float Calculations

```c
// Use TEST_ASSERT_FLOAT_WITHIN for float comparisons
TEST_ASSERT_FLOAT_WITHIN(0.01f, expected, actual);
```

### Testing Memory Management

```c
void test_widget_creation_and_destruction(void) {
    // Track allocations if needed
    Widget* w = widget_create("test");
    TEST_ASSERT_NOT_NULL(w);
    widget_destroy(w);
    // Verify cleanup in tearDown
}
```

### Testing Error Conditions

```c
void test_null_parameter_handling(void) {
    PkError err = layout_calculate(NULL, 100, 100);
    TEST_ASSERT_EQUAL(PK_ERROR_NULL_PARAM, err);
}
```

## References

- Unity Documentation: https://github.com/ThrowTheSwitch/Unity
- Original Discussion: `feature/layout-style-system` branch
- UI Architecture: `docs/tracking/UI_FEATURE_TRACKING.md`