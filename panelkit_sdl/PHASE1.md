# Phase 1: Button Architecture Redesign

## Overview
Redesign buttons as composable containers that handle interaction while delegating rendering to child widgets.

## Current Problem
- Buttons try to do everything: interaction AND rendering
- Draw placeholder rectangles instead of text
- Cannot be extended without modification
- Depend on global font variables

## New Architecture

### Design Principles
1. **Single Responsibility**: Each widget does ONE thing well
2. **Composition over Inheritance**: Build complex widgets from simple ones
3. **Open/Closed**: Open for extension, closed for modification

### Implementation

#### Button Widget (Interaction Only)
```c
typedef struct ButtonWidget {
    Widget base;              // Must be first for casting
    
    // Visual states
    SDL_Color normal_color;
    SDL_Color hover_color; 
    SDL_Color pressed_color;
    SDL_Color disabled_color;
    
    // Interaction
    button_click_callback on_click;
    void* user_data;
    
    // Event publishing (optional)
    char* publish_event;
    void* publish_data;
    size_t publish_data_size;
} ButtonWidget;

// Constructor - note NO font or label parameters
ButtonWidget* button_widget_create(const char* id);
```

#### Key Changes
1. Remove `label` field from ButtonWidget
2. Change `child_capacity` from 0 to allow children
3. Button renders background/border only
4. Children handle their own rendering

#### Usage Examples
```c
// Simple text button
ButtonWidget* btn = button_widget_create("submit_btn");
TextWidget* label = text_widget_create("submit_label", "Submit", font);
widget_add_child((Widget*)btn, (Widget*)label);
button_widget_set_colors(btn, blue, light_blue, dark_blue, gray);

// Button with icon and text
ButtonWidget* btn = button_widget_create("save_btn");
IconWidget* icon = icon_widget_create("save_icon", ICON_SAVE);
TextWidget* label = text_widget_create("save_label", "Save", font);
widget_add_child((Widget*)btn, (Widget*)icon);
widget_add_child((Widget*)btn, (Widget*)label);

// Button with custom content
ButtonWidget* btn = button_widget_create("progress_btn");
ProgressWidget* progress = progress_widget_create("progress", 0.75);
widget_add_child((Widget*)btn, (Widget*)progress);
```

### Migration Strategy

#### Step 1: Modify ButtonWidget Structure
- Remove `char label[256]` field
- Remove `SDL_Color text_color` field (text widget handles this)
- Change `base->child_capacity = 0` to `base->child_capacity = 2`
- Allocate children array

#### Step 2: Update Button Rendering
- Remove label drawing code (rectangles)
- Keep background and border rendering
- Let base widget render children

#### Step 3: Update Button Creation
- Modify widget_integration.c to create text widget children
- Pass fonts to text widgets, not buttons
- Set proper bounds for text within button

#### Step 4: Layout Management
- Text widget should fill button area minus padding
- Center text by default
- Allow override for custom positioning

### Benefits
1. **Clean Architecture**: Each widget has single responsibility
2. **Extensible**: Any widget can be a button's content
3. **No Global Dependencies**: Fonts passed where needed
4. **Reusable**: Text widgets can be used anywhere
5. **Testable**: Can test interaction separately from rendering

### Success Criteria
- Buttons display actual text (not rectangles)
- Buttons can contain any widget type
- No global font dependencies
- Clean separation of concerns
- All existing buttons work correctly