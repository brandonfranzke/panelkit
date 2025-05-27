# Test Directory

This directory contains test programs and utilities for debugging PanelKit components.

## Structure

```
test/
├── core/           # Core functionality tests
├── input/          # Input system tests  
├── display/        # Display tests (placeholder)
└── integration/    # Integration tests (placeholder)
```

## Input Tests

The input tests are the most comprehensive, with various touch testing utilities:

- `test_input_handler.c` - Test the input abstraction layer
- `test_touch_raw.c` - Test raw Linux touch events
- `test_sdl_touch.c` - Test SDL touch handling
- `test_integrated_touch.c` - Full integration test

See [input/TESTING_PLAN.md](input/TESTING_PLAN.md) for detailed test procedures.

## Building Tests

Individual test directories may have their own build systems. For input tests:

```bash
cd test/input
./build.sh          # Build all input tests
./test_touch_raw    # Run specific test
```

## Future Tests

Planned test coverage includes:
- Widget system unit tests
- Event system tests
- State store tests
- API client mocking
- Full integration tests