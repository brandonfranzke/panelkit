# Widget System Technical Debt

## Critical Issues (Fix Before Testing)

### 1. **Button Text Rendering**
- **Issue**: Buttons draw placeholder rectangles instead of actual text
- **Impact**: Completely broken UI in widget mode
- **Solution**: Make buttons composable containers that can hold text widgets

### 2. **Global Font Dependencies**
- **Issue**: Widgets rely on `extern TTF_Font*` variables
- **Impact**: Tight coupling, hard to test, not reusable
- **Solution**: Pass fonts through widget factory or initialization

### 3. **Inconsistent Widget Patterns**
- **Issue**: Each widget type has different initialization patterns
- **Impact**: Confusing API, prone to errors
- **Solution**: Standardize widget creation with consistent factory pattern

### 4. **Missing Child Support in Buttons**
- **Issue**: Buttons have `child_capacity = 0`, can't compose
- **Impact**: Can't add text widgets or icons to buttons
- **Solution**: Make buttons proper containers

## Major Architectural Issues (Fix After Basic Functionality)

### 5. **Widget Integration Monolith**
- **Issue**: widget_integration.c is 940 lines doing too many things
- **Impact**: Hard to maintain, test, and understand
- **Solution**: Split into focused modules (creation, events, state sync)

### 6. **No Layout Management**
- **Issue**: All positioning is manual with hardcoded values
- **Impact**: No responsive design, lots of magic numbers
- **Solution**: Implement proper layout managers (flex, grid, etc.)

### 7. **Event System String Coupling**
- **Issue**: Events use string names, prone to typos
- **Impact**: Runtime errors, no compile-time checking
- **Solution**: Use enums or strongly-typed events

### 8. **No Style/Theme System**
- **Issue**: Colors and sizes hardcoded everywhere
- **Impact**: Can't change appearance without recompiling
- **Solution**: Implement proper theming/styling system

## Code Quality Issues (Ongoing Cleanup)

### 9. **Duplicate Logging Macros**
- **Issue**: Every file redefines logging macros
- **Impact**: Code duplication, inconsistent logging
- **Solution**: Use centralized logger.h properly

### 10. **Memory Management**
- **Issue**: Manual malloc/free without consistent patterns
- **Impact**: Potential leaks, crashes
- **Solution**: Implement consistent allocation/cleanup patterns

### 11. **No Error Recovery**
- **Issue**: Many functions silently fail or just return NULL
- **Impact**: Hard to debug, poor user experience
- **Solution**: Proper error handling and reporting

### 12. **Type Safety**
- **Issue**: Excessive use of void* and manual casting
- **Impact**: Runtime errors, hard to refactor
- **Solution**: Use proper types and interfaces

## Performance Issues (Optimize Later)

### 13. **No Dirty Region Tracking**
- **Issue**: Always redraw entire screen
- **Impact**: Poor performance, battery drain
- **Solution**: Implement dirty rectangle system

### 14. **Inefficient Hit Testing**
- **Issue**: Linear search through widget tree
- **Impact**: Slow with many widgets
- **Solution**: Spatial indexing or caching

## Testing Infrastructure (Add When Stable)

### 15. **No Unit Tests**
- **Issue**: No automated testing for widgets
- **Impact**: Regressions, hard to refactor
- **Solution**: Add unit test framework and tests

### 16. **No Mock Implementations**
- **Issue**: Can't test widgets in isolation
- **Impact**: Tests require full SDL setup
- **Solution**: Create mock renderers and event systems

## Documentation Debt

### 17. **Missing Architecture Docs**
- **Issue**: No high-level design documentation
- **Impact**: Hard for new developers to understand
- **Solution**: Create architecture diagrams and guides

### 18. **Inconsistent API Docs**
- **Issue**: Some functions documented, many aren't
- **Impact**: Have to read source to understand API
- **Solution**: Document all public APIs consistently

## Legacy Integration Issues

### 19. **Shadow Widget Complexity**
- **Issue**: Complex mirroring between old and new systems
- **Impact**: Bugs, performance overhead, confusion
- **Solution**: Complete migration and remove legacy system

### 20. **State Synchronization**
- **Issue**: Manual sync between widget and app state
- **Impact**: Bugs when states get out of sync
- **Solution**: Single source of truth for state

---

## Priority Order for BREAKTHROUGH Development

### Phase 1: Make It Render Correctly (Before Testing)
1. Fix button text rendering (#1)
2. Remove global font dependencies (#2)
3. Enable child widgets in buttons (#4)
4. Standardize widget creation (#3)

### Phase 2: Make It Maintainable (After Basic Testing)
5. Split widget_integration.c (#5)
6. Add basic layout management (#6)
7. Fix event system coupling (#7)
8. Add error handling (#11)

### Phase 3: Make It Professional (After Full Migration)
9. Add theme system (#8)
10. Remove duplicate logging (#9)
11. Add unit tests (#15)
12. Add documentation (#17)

### Phase 4: Make It Fast (When Feature Complete)
13. Add dirty region tracking (#13)
14. Optimize hit testing (#14)
15. Add widget pooling
16. Profile and optimize