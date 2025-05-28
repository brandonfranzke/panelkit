# Widget System Technical Debt

## Recently Completed

### Widget Integration Layer Removal (2025-01-27)
- **What was removed**: The "shadow widget" transitional system (widget_integration_*.c/h files)
- **Current state**: All UI setup temporarily consolidated in `ui_init.c`
- **Why temporary**: This hardcoded UI setup will be replaced by proper layout/theme systems
- **Known issues**: 
  - Button color changes not working (event propagation issue)
  - All UI positioning still hardcoded
- **Next steps**: Implement layout management and theme systems to replace ui_init.c

## Active Issues

### Architecture & Design

1. **Layout Management** ✅ PARTIALLY COMPLETE
   - **Issue**: All positioning is manual with hardcoded values
   - **Status**: Layout system core implemented (2025-01-28)
     - ✅ Absolute layout engine
     - ✅ Flexbox layout engine
     - ⏳ Grid layout engine (pending)
     - ⏳ Integration with existing widgets
   - **Remaining**: Migrate widgets from hardcoded positions to layout system

2. **Style/Theme System** 🔄 IN PROGRESS
   - **Issue**: Colors and sizes hardcoded everywhere
   - **Status**: Style system designed and documented
     - ✅ Complete specification in docs/planning/UI_STYLE_SYSTEM.md
     - ⏳ Implementation pending
   - **Solution**: Implement style system per specification

### Code Quality

3. **Global State in app.c**
   - **Issue**: 20+ global variables (window, renderer, fonts, etc.)
   - **Impact**: Hard to test, not thread-safe, tight coupling
   - **Solution**: Create application context struct, pass through call chain

### Performance

4. **No Dirty Region Tracking**
    - **Issue**: Always redraw entire screen
    - **Impact**: Poor performance, battery drain
    - **Solution**: Implement dirty rectangle system

5. **Inefficient Hit Testing**
    - **Issue**: Linear search through widget tree
    - **Impact**: Slow with many widgets
    - **Solution**: Spatial indexing or caching

### Testing & Documentation

6. **No Unit Tests**
    - **Issue**: No automated testing for widgets
    - **Impact**: Regressions, hard to refactor
    - **Solution**: Add unit test framework and tests

## Planned Improvements

### Layout Management System ✅ CORE COMPLETE
**Status**: Core layout system implemented (2025-01-28)

**Completed**:
- ✅ Type-safe layout engine architecture
- ✅ Absolute positioning (x, y, width, height)
- ✅ Flexbox layout (row/column, justify, align, grow/shrink)
- ✅ Float-based coordinates with relative/absolute support
- ✅ Comprehensive test suite (25 tests passing)

**Remaining**:
- ⏳ Grid layout implementation
- ⏳ Widget integration (migrate from hardcoded positions)
- ⏳ Remove temporary ui_init.c

## Priority Order

### High Priority
1. Style system implementation - Implement per UI_STYLE_SYSTEM.md spec
2. Widget-layout integration - Migrate widgets to use layout system

### Medium Priority
3. Grid layout - Complete the layout system trio
4. Global state cleanup (#3) - Create app context structure
5. Performance optimizations (#4, #5) - Dirty regions and hit testing

### Low Priority
5. Testing infrastructure (#6) - Unit tests for widgets

## Optional Enhancements

### UI Features
1. **Page Indicators**
   - Pills with dots at bottom of screen
   - Show during swipe and 2 seconds after
   - Fade out with 400ms duration

2. **Widget Debug Overlay**
   - Widget-based FPS counter
   - Current page indicator
   - Touch state display