# UI Feature Implementation Tracking

## Overview

This document captures all architectural decisions, design philosophy, and implementation plans for the PanelKit layout and style systems. This is the primary reference for understanding the UI system design.

## Core Philosophy

1. **Separate Concerns**: Layout (WHERE things go) and Style (HOW things look) are separate systems
2. **Immutable Styles**: Styles are resolved once and consumed read-only
3. **Declarative Layout**: CSS-like declarative approach without CSS complexity
4. **Embedded-First**: Designed for fixed-display kiosk/embedded systems
5. **Performance**: Pre-calculate where possible, minimize runtime overhead

## Architecture Decisions

### System Separation

```
Widget
  ├── Layout Engine (positioning, sizing, spacing)
  │     ├── Absolute positioning
  │     ├── Flexbox-style layouts  
  │     └── Grid layouts
  │
  └── Style Engine (appearance)
        ├── Theme resolver
        ├── Property cascading  
        └── Render properties
```

### Key Design Choices

1. **Layout and Style are separate systems**
   - Layout: WHERE things go (positioning, sizing, spacing)
   - Style: HOW things look (colors, fonts, borders)
   - Different update patterns and responsibilities
   - One-way dependency: Layout may influence style, never vice versa

2. **Coordinate System**
   - Logical coordinates: x=horizontal, y=vertical (always)
   - Display transformation layer handles hardware mounting
   - All rotations/flips handled by transformation matrix
   - Touch events automatically transformed to logical coordinates

3. **Layout Types**
   - **Absolute**: Direct x,y,width,height positioning
   - **Flexbox**: Row/column with flex properties
   - **Grid**: Cell-based with spans

4. **Fractional Positioning**
   - Use float (0.0-1.0) internally
   - Convert to pixels only at render time
   - Enables responsive percentage-based layouts
   - Simplifies transformation calculations

5. **Property Resolution Hierarchy**
   1. Widget-specific override
   2. Theme/style definition
   3. Config file defaults
   4. Hardcoded compile-time fallbacks

6. **Immutable Styles**
   - Styles cannot change after creation
   - Thread-safe without locks
   - Share styles across widgets
   - Explicit change tracking via new style creation

7. **Layout Invalidation**
   - Automatic marking when child bounds change
   - Batch recalculations until next frame
   - Explicit freeze/thaw for multiple changes

## Implementation Plan

### Phase 1: Layout System (High Priority)

1. **Core Layout Engine**
   - Layout specification system
   - Property resolution
   - Calculation pipeline
   - Pixel conversion

2. **Layout Types**
   - Absolute positioning (current method)
   - Flexbox implementation
   - Grid implementation

3. **Display Transform**
   ```c
   typedef struct {
       DisplayRotation rotation;  // ROTATE_0, ROTATE_90, ROTATE_180, ROTATE_270
       bool flip_horizontal;      // Mirror across vertical axis
       bool flip_vertical;        // Mirror across horizontal axis
   } DisplayTransform;
   ```

### Phase 2: Minimal Style System

1. **Core Properties**
   - Colors (background, text, border)
   - Basic box model (padding, margin)
   - Font properties (size for layout, family/weight for style)

2. **Style Resolution**
   - Immutable style objects
   - Style sharing/caching
   - State-based styles (normal, pressed, disabled)

### Phase 3: Integration

1. Replace hardcoded UI in `ui_init.c`
2. Migrate existing widgets to new system
3. Remove temporary code

## Technical Specifications

### Layout Data Structures

```c
typedef struct {
    float x, y, width, height;  // Logical units (0.0-1.0 or absolute)
} LayoutRect;

typedef enum {
    LAYOUT_ABSOLUTE,
    LAYOUT_FLEX,
    LAYOUT_GRID
} LayoutType;
```

### Style Ownership

- **Layout handles**: Font size (affects measurement)
- **Style handles**: Font family, weight, color, decoration

### Constraints

1. **No window resizing** - Display size fixed at startup
2. **Minimal runtime dynamism** - Most UI is statically defined
3. **Memory available** - 2GB minimum, clean design over memory micro-optimization
4. **Fixed displays** - May need rotation for hardware mounting

## Debug Support

Minimal but useful debugging aids:
- `layout_debug_draw_bounds()` - Widget boundary visualization
- `layout_debug_get_widget_at()` - Touch registration debugging
- `LAYOUT_LOG()` macro - Compile-time removable logging
- Layout statistics (calculation count, timing)

## Future Considerations

- Orientation changes via gyroscope (runtime rotation)
- Animation support (layout transitions)
- Scrollable areas
- Advanced styling (gradients, shadows)

## Success Metrics

1. **Code Reduction**: Eliminate hardcoded positions
2. **Maintainability**: UI changes without recompilation
3. **Performance**: Sub-millisecond layout calculation
4. **Memory**: Shared immutable styles
5. **Flexibility**: Easy to add new layout types

## References

- Original discussion: Feature branch `feature/remove-widget-integration`
- Config system: `docs/CONFIGURATION.md`
- Widget system: `docs/WIDGETS.md`
- Technical debt: `docs/tracking/TECH_DEBT.md`