# Widget System Technical Debt

## Recently Completed ✅

1. **Legacy System Removal** (Completed today)
   - Removed ~1,375 lines of legacy UI code
   - Eliminated dual rendering paths
   - Widget system now standalone

2. **Global Font Dependencies** (Issue #2 - Completed today)
   - Removed all `extern TTF_Font*` variables
   - Fonts now passed through widget_integration_set_fonts()
   - Widget factory and widgets receive fonts properly

3. **Widget Integration Monolith** (Completed today)
   - Split 1053-line file into 4 focused modules:
     - widget_integration_core.c (160 lines)
     - widget_integration_state.c (230 lines)
     - widget_integration_events.c (240 lines)
     - widget_integration_widgets.c (440 lines)

## Active Issues

### Architecture & Design

1. **No Layout Management**
   - **Issue**: All positioning is manual with hardcoded values
   - **Impact**: No responsive design, lots of magic numbers
   - **Solution**: Implement proper layout managers (flex, grid, etc.)

2. **Event System String Coupling**
   - **Issue**: Events use string names, prone to typos
   - **Impact**: Runtime errors, no compile-time checking
   - **Solution**: Use enums or strongly-typed events

3. **No Style/Theme System**
   - **Issue**: Colors and sizes hardcoded everywhere
   - **Impact**: Can't change appearance without recompiling
   - **Solution**: Implement proper theming/styling system

### Code Quality

4. **Duplicate Logging Macros**
   - **Issue**: Every file redefines logging macros
   - **Impact**: Code duplication, inconsistent logging
   - **Solution**: Use centralized logger.h properly

5. **Memory Management**
   - **Issue**: Manual malloc/free without consistent patterns
   - **Impact**: Potential leaks, crashes
   - **Solution**: Implement consistent allocation/cleanup patterns

6. **No Error Recovery**
   - **Issue**: Many functions silently fail or just return NULL
   - **Impact**: Hard to debug, poor user experience
   - **Solution**: Proper error handling and reporting

7. **Type Safety**
   - **Issue**: Excessive use of void* and manual casting
   - **Impact**: Runtime errors, hard to refactor
   - **Solution**: Use proper types and interfaces

### Performance

8. **No Dirty Region Tracking**
    - **Issue**: Always redraw entire screen
    - **Impact**: Poor performance, battery drain
    - **Solution**: Implement dirty rectangle system

9. **Inefficient Hit Testing**
    - **Issue**: Linear search through widget tree
    - **Impact**: Slow with many widgets
    - **Solution**: Spatial indexing or caching

### Testing & Documentation

10. **No Unit Tests**
    - **Issue**: No automated testing for widgets
    - **Impact**: Regressions, hard to refactor
    - **Solution**: Add unit test framework and tests

11. **Inconsistent API Documentation**
    - **Issue**: Some functions documented, many aren't
    - **Impact**: Have to read source to understand API
    - **Solution**: Document all public APIs consistently

## Priority Order

### High Priority (Blocking Issues)
1. Error handling (#6)
2. Memory management patterns (#5)
3. Layout management (#1)

### Medium Priority (Quality of Life)
4. Logging cleanup (#4)
5. Event system type safety (#2)
6. Type safety improvements (#7)

### Low Priority (Nice to Have)
7. Theme system (#3)
8. Performance optimizations (#8, #9)
9. Testing infrastructure (#10)
10. API documentation (#11)

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