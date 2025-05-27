# Legacy System Migration Plan

## Overview

This document outlines the plan to remove the legacy rendering system and complete the migration to the widget-based system.

## Effort Estimate: 8-14 hours

### Breakdown
- 2-3 hours: Remove legacy files and fix compilation
- 3-4 hours: Refactor app.c event loop
- 2-3 hours: Update initialization/cleanup
- 1-2 hours: Test and fix regressions
- 2 hours: Update build system/configs

## Phase 1: Remove Legacy Files (2-3 hours)

### Files to Delete (~850 lines)
- `src/ui/pages.c` (304 lines)
- `src/ui/pages.h` (55 lines)
- `src/ui/rendering.c` (334 lines)
- `src/ui/rendering.h` (31 lines)
- `src/ui/gestures.c` (204 lines)
- `src/ui/gestures.h` (56 lines)

### Actions
1. Delete the files
2. Remove from CMakeLists.txt
3. Fix compilation errors by removing includes
4. Remove any function calls to deleted code

## Phase 2: Refactor app.c (3-4 hours)

### Current State
- ~37 legacy function calls in app.c
- Dual rendering paths (lines 589-731)
- Legacy event handlers (lines 510-582)
- Legacy initialization (lines 241-361)

### Target State
- Single widget-based rendering path
- Simplified event loop
- Widget-only initialization

### Key Changes

#### Remove Dual Rendering Paths
```c
// DELETE: Lines checking for WIDGET_RENDER environment variable
// DELETE: All legacy rendering code
// KEEP: Only widget rendering path
```

#### Simplify Event Loop
```c
// DELETE: Legacy touch handlers (handle_touch_down, handle_touch_up, etc.)
// DELETE: Widget integration mirroring
// KEEP: Direct SDL event forwarding to widget manager
```

#### Clean Initialization
```c
// DELETE: pages_init(), rendering_init(), gestures_init()
// DELETE: Legacy font loading
// KEEP: Widget system initialization only
```

## Phase 3: Update Initialization (2-3 hours)

### Simplify main() Function
1. Remove all legacy system initialization
2. Remove environment variable checks
3. Always use widget rendering
4. Clean up configuration handling

### Update Cleanup
1. Remove legacy cleanup calls
2. Ensure widget system properly cleaned up
3. Verify no memory leaks

## Phase 4: Testing & Regression Fixes (1-2 hours)

### Feature Parity Checklist
Using LEGACY_UI.md as reference:

- [ ] All buttons function correctly
- [ ] Page transitions smooth and correct
- [ ] Swipe gestures work properly
- [ ] Background color changes persist
- [ ] Time display toggles correctly
- [ ] API data displays properly
- [ ] Exit functionality works
- [ ] Text colors cycle correctly
- [ ] Scroll behavior matches legacy

### Missing Features to Implement
1. **Page Indicators**
   - Show during swipe and 2 seconds after
   - Fade out with 400ms duration
   - Pills with dots at bottom of screen

2. **Debug Overlay** 
   - Widget-based FPS counter
   - Current page indicator
   - Touch state display

## Phase 5: Build System Updates (2 hours)

### Configuration Changes
1. Remove WIDGET_RENDER checks from configs
2. Update default configs for widget-only mode
3. Remove legacy-specific settings

### Documentation Updates
1. Update README for widget-only system
2. Remove references to dual-mode operation
3. Update build instructions

## Risk Mitigation

### Before Starting
1. Create git branch for migration
2. Ensure current tests pass
3. Document current behavior with screenshots/videos

### During Migration
1. Commit after each phase
2. Test functionality after each major change
3. Keep detailed notes of any issues

### Rollback Plan
If critical issues found:
1. Git revert to checkpoint
2. Re-evaluate migration approach
3. Fix issues in widget system first

## Success Criteria

### Functional
- All features from LEGACY_UI.md working
- No visual regressions
- Performance equal or better
- Memory usage equal or better

### Code Quality
- No legacy code remaining
- Clean, single rendering path
- Simplified event handling
- Better maintainability

## Post-Migration Tasks

See TECH_DEBT.md for prioritized improvements:
1. Global font dependency cleanup
2. Widget integration refactor
3. Layout management system
4. Theme/style system
5. Performance optimizations