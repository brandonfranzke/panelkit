# Widget System Technical Debt

## Active Issues

### Architecture & Design

1. **Global Font Dependencies**
   - **Issue**: Widgets rely on `extern TTF_Font*` variables
   - **Impact**: Tight coupling, hard to test, not reusable
   - **Solution**: Pass fonts through widget factory or initialization

2. **Widget Integration Monolith**
   - **Issue**: widget_integration.c is 940+ lines doing too many things
   - **Impact**: Hard to maintain, test, and understand
   - **Solution**: Split into focused modules (creation, events, state sync)

3. **No Layout Management**
   - **Issue**: All positioning is manual with hardcoded values
   - **Impact**: No responsive design, lots of magic numbers
   - **Solution**: Implement proper layout managers (flex, grid, etc.)

4. **Event System String Coupling**
   - **Issue**: Events use string names, prone to typos
   - **Impact**: Runtime errors, no compile-time checking
   - **Solution**: Use enums or strongly-typed events

5. **No Style/Theme System**
   - **Issue**: Colors and sizes hardcoded everywhere
   - **Impact**: Can't change appearance without recompiling
   - **Solution**: Implement proper theming/styling system

### Code Quality

6. **Duplicate Logging Macros**
   - **Issue**: Every file redefines logging macros
   - **Impact**: Code duplication, inconsistent logging
   - **Solution**: Use centralized logger.h properly

7. **Memory Management**
   - **Issue**: Manual malloc/free without consistent patterns
   - **Impact**: Potential leaks, crashes
   - **Solution**: Implement consistent allocation/cleanup patterns

8. **No Error Recovery**
   - **Issue**: Many functions silently fail or just return NULL
   - **Impact**: Hard to debug, poor user experience
   - **Solution**: Proper error handling and reporting

9. **Type Safety**
   - **Issue**: Excessive use of void* and manual casting
   - **Impact**: Runtime errors, hard to refactor
   - **Solution**: Use proper types and interfaces

### Performance

10. **No Dirty Region Tracking**
    - **Issue**: Always redraw entire screen
    - **Impact**: Poor performance, battery drain
    - **Solution**: Implement dirty rectangle system

11. **Inefficient Hit Testing**
    - **Issue**: Linear search through widget tree
    - **Impact**: Slow with many widgets
    - **Solution**: Spatial indexing or caching

### Testing & Documentation

12. **No Unit Tests**
    - **Issue**: No automated testing for widgets
    - **Impact**: Regressions, hard to refactor
    - **Solution**: Add unit test framework and tests

13. **Inconsistent API Documentation**
    - **Issue**: Some functions documented, many aren't
    - **Impact**: Have to read source to understand API
    - **Solution**: Document all public APIs consistently

### Legacy Integration

14. **Shadow Widget Complexity**
    - **Issue**: Complex mirroring between old and new systems
    - **Impact**: Bugs, performance overhead, confusion
    - **Solution**: Complete migration and remove legacy system

15. **State Synchronization**
    - **Issue**: Manual sync between widget and app state
    - **Impact**: Bugs when states get out of sync
    - **Solution**: Single source of truth for state

## Priority Order

### High Priority (Blocking Issues)
1. Global font dependencies (#1)
2. Error handling (#8)
3. Memory management patterns (#7)

### Medium Priority (Quality of Life)
4. Widget integration refactor (#2)
5. Layout management (#3)
6. Logging cleanup (#6)

### Low Priority (Nice to Have)
7. Theme system (#5)
8. Performance optimizations (#10, #11)
9. Testing infrastructure (#12)

### Post-Legacy Removal
10. Remove shadow widgets (#14)
11. Simplify state management (#15)
12. Complete API documentation (#13)