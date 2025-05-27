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

### Performance

3. **No Dirty Region Tracking**
    - **Issue**: Always redraw entire screen
    - **Impact**: Poor performance, battery drain
    - **Solution**: Implement dirty rectangle system

4. **Inefficient Hit Testing**
    - **Issue**: Linear search through widget tree
    - **Impact**: Slow with many widgets
    - **Solution**: Spatial indexing or caching

### Testing & Documentation

5. **No Unit Tests**
    - **Issue**: No automated testing for widgets
    - **Impact**: Regressions, hard to refactor
    - **Solution**: Add unit test framework and tests

6. **Inconsistent API Documentation**
    - **Issue**: Some functions documented, many aren't
    - **Impact**: Have to read source to understand API
    - **Solution**: Document all public APIs consistently

## Next Planned Improvements

### 1. Error Recovery System (Comprehensive)
**Status**: Foundation built, comprehensive implementation needed

**What's Done**:
- Created PkError enum and error.h/error.c infrastructure
- Thread-local error storage with pk_get/set_last_error()
- Updated 3 example functions (widget_create, widget_add_child, event_subscribe)

**What's Needed for Comprehensive Implementation**:

**Phase 1: Systematic Error Propagation (1 day)**
- Update every function that can fail to call pk_set_last_error()
- Functions to update:
  - All widget creation functions (30+ functions)
  - Config loading/parsing (10+ functions)
  - API communication (15+ functions)
  - State store operations (10+ functions)
  - Display/Input initialization (10+ functions)

**Phase 2: Error Propagation Patterns (4 hours)**
- Implement error bubbling up call stacks
- Add PK_RETURN_IF_ERROR macro for propagation
- Ensure errors don't get lost in wrapper functions

**Phase 3: Recovery Strategies (1 day)**
- Network failures: Exponential backoff retry
- Config errors: Fallback to defaults with warnings
- Memory exhaustion: Graceful degradation
- Widget creation failures: Cleanup partial state

**Phase 4: User-Visible Error Handling (4 hours)**
- Create error notification widget
- Status bar for non-fatal errors
- Error logging to file
- Debug mode error display overlay

**Verification**:
- Ensure all error paths clean up resources
- No silent failures anywhere

### 2. Layout Management System
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

### Completed ✅
- Logging cleanup - Fixed duplicate macros across 13 files
- Type Safety Phase 1+2 - Replaced anonymous structs, added safe casting
- Type Safety Phase 3 - Replaced void* with typed unions for impl data
- Event System String Coupling - Implemented strongly typed event system
- Memory Management - Fixed event system memory leak, documented ownership patterns
- Error Recovery - Added PkError system with thread-local error context

### High Priority (Next Tasks)
1. Layout management (#1) - Replace hardcoded positioning

### Medium Priority (Future)
2. Theme system (#2) - Configurable colors and styles
3. Performance optimizations (#3, #4) - Dirty regions and hit testing

### Low Priority (Nice to Have)
4. Testing infrastructure (#5) - Unit tests for widgets
5. API documentation (#6) - Complete documentation

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
