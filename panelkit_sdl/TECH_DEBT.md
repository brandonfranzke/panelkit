# Widget System Technical Debt

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

3. **No Error Recovery**
   - **Issue**: Many functions silently fail or just return NULL
   - **Impact**: Hard to debug, poor user experience
   - **Solution**: Proper error handling and reporting

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

7. **Inconsistent API Documentation**
    - **Issue**: Some functions documented, many aren't
    - **Impact**: Have to read source to understand API
    - **Solution**: Document all public APIs consistently

## Next Planned Improvements

### 1. Error Recovery System
**Approach**: Add consistent error handling throughout the codebase

**Implementation**:
- Add error codes enum for all failure modes
- Return error codes instead of just NULL
- Add error context/messages where helpful
- Consider error callback system for async operations

**Priority areas**:
- Widget creation failures
- Event system errors
- API communication errors
- Resource allocation failures

## Priority Order

### Completed ✅
- Logging cleanup - Fixed duplicate macros across 13 files
- Type Safety Phase 1+2 - Replaced anonymous structs, added safe casting
- Event System String Coupling - Implemented strongly typed event system
- Memory Management - Fixed event system memory leak, documented ownership patterns

### High Priority (Next Tasks)
1. Error handling (#3) - Add proper error recovery
2. Layout management (#1) - Replace hardcoded positioning

### Medium Priority (Future)
3. Theme system (#2) - Configurable colors and styles
4. Performance optimizations (#4, #5) - Dirty regions and hit testing

### Low Priority (Nice to Have)
5. Testing infrastructure (#6) - Unit tests for widgets
6. API documentation (#7) - Complete documentation

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
