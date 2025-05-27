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

3. **Memory Management**
   - **Issue**: Manual malloc/free without consistent patterns
   - **Impact**: Potential leaks, crashes
   - **Solution**: Implement consistent allocation/cleanup patterns

4. **No Error Recovery**
   - **Issue**: Many functions silently fail or just return NULL
   - **Impact**: Hard to debug, poor user experience
   - **Solution**: Proper error handling and reporting

### Performance

5. **No Dirty Region Tracking**
    - **Issue**: Always redraw entire screen
    - **Impact**: Poor performance, battery drain
    - **Solution**: Implement dirty rectangle system

6. **Inefficient Hit Testing**
    - **Issue**: Linear search through widget tree
    - **Impact**: Slow with many widgets
    - **Solution**: Spatial indexing or caching

### Testing & Documentation

7. **No Unit Tests**
    - **Issue**: No automated testing for widgets
    - **Impact**: Regressions, hard to refactor
    - **Solution**: Add unit test framework and tests

8. **Inconsistent API Documentation**
    - **Issue**: Some functions documented, many aren't
    - **Impact**: Have to read source to understand API
    - **Solution**: Document all public APIs consistently

## Next Planned Improvements

### 1. Memory Management - Pool Allocator
**Approach**: Arena allocator for widgets with clear lifetime boundaries

**Design**:
```c
typedef struct {
    uint8_t* memory;
    size_t capacity;
    size_t used;
    size_t alignment;
} MemoryPool;

// Pool management
MemoryPool* pool_create(size_t size);
void* pool_alloc(MemoryPool* pool, size_t size);
void pool_reset(MemoryPool* pool);  // Clear all allocations
void pool_destroy(MemoryPool* pool);
```

**Implementation strategy**:
- Create separate pools for different lifetimes:
  - Widget pool: All widgets for a page (reset on page change)
  - Event pool: Temporary event data (reset after dispatch)
  - System pool: Long-lived system objects
- Add pool parameter to widget creation functions
- Replace individual widget free() calls with pool resets
- Keep traditional malloc/free for system objects with complex lifetimes

**Integration points**:
- widget_manager to own and manage widget pools
- widget_factory to accept pool parameter
- Page transitions to trigger pool resets
- Event system to use temporary event pool

**Benefits over current approach**:
- No individual free() calls needed for widgets
- Guaranteed cleanup on page transitions
- Better cache locality for related widgets
- Eliminates most leak potential in UI code

## Priority Order

### Completed ✅
- Logging cleanup (#4) - Fixed duplicate macros
- Type Safety Phase 1+2 (#7) - Replaced anonymous structs, added safe casting
- Event System String Coupling (#2) - Implemented strongly typed event system

### High Priority (Next Tasks)
1. Memory Management Pool Allocator (#5) - Uses types and events

### Medium Priority (Future)
2. Error handling (#4)
3. Layout management (#1)

### Low Priority (Nice to Have)
4. Theme system (#2)
5. Performance optimizations (#5, #6)
6. Testing infrastructure (#7)
7. API documentation (#8)

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
