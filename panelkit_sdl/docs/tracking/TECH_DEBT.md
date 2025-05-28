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

1. **No Layout Management**
   - **Issue**: All positioning is manual with hardcoded values
   - **Impact**: No responsive design, lots of magic numbers
   - **Solution**: Implement proper layout managers (flex, grid, etc.)

2. **No Style/Theme System**
   - **Issue**: Colors and sizes hardcoded everywhere
   - **Impact**: Can't change appearance without recompiling
   - **Solution**: Implement proper theming/styling system

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

### Layout Management System
**Approach**: Replace hardcoded positioning with flexible layout managers

**Implementation ideas**:
- Flexbox-style layout for rows/columns
- Grid layout for structured content
- Constraints system for responsive design
- Relative positioning and sizing

**Benefits**:
- Responsive to different screen sizes
- Easier to maintain and modify layouts
- Less magic numbers in code

## Priority Order

### High Priority
1. Layout management (#1) - Replace hardcoded positioning

### Medium Priority
2. Global state cleanup (#3) - Create app context structure
3. Theme system (#2) - Configurable colors and styles
4. Performance optimizations (#4, #5) - Dirty regions and hit testing

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