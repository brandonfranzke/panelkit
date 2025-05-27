# Legacy UI Implementation Reference

This document captures ALL user-facing aspects of the legacy UI implementation, serving as a reference to ensure the widget implementation matches the legacy behavior exactly.

## Screen Layout and Dimensions

### Base Dimensions
- Default screen: 640x480 pixels
- Screen dimensions are dynamic and set from display backend at runtime
- All UI elements scale based on actual_width and actual_height

### Button Layout
- **Button Width**: `actual_width / 2` (50% of screen width)
- **Button Height**: `(actual_height * 2) / 3` (66.67% of screen height)
- **Button Padding**: 20 pixels (all sides)
- **Buttons are LEFT-ALIGNED** with BUTTON_PADDING offset from left edge

### Page Structure
- **Total Pages**: 2 (indexed 0-1)
- **Starting Page**: Page 0 (Text Page)
- **Page Transition**: Horizontal swipe gesture

## Page 0: Text Page

### Layout
- **Title**: "Text Page"
  - Position: Centered horizontally, Y=60
  - Font: Large font (32px default)
  - Color: White (255, 255, 255)

- **Single Button**: "Change Text Color"
  - Position: Centered horizontally, Y=200
  - Width: BUTTON_WIDTH (50% screen width)
  - Height: BUTTON_HEIGHT / 2 (33.33% screen height)
  - Color: Blue (52, 152, 219)
  - Action: Cycles through text colors

- **Text Content** (two lines):
  - Line 1: "Welcome to Page 1!"
  - Line 2: "Swipe right to see buttons."
  - Position: Centered, below button (Y = button_y + button_height + 80/110)
  - Font: Regular font (18px default)
  - Color: Cycles through 7 colors on button press

### Text Color Cycle
```
1. White (255, 255, 255)
2. Red (255, 100, 100)
3. Green (100, 255, 100)
4. Blue (100, 100, 255)
5. Yellow (255, 255, 100)
6. Purple (255, 100, 255)
7. Cyan (100, 255, 255)
```

## Page 1: Buttons Page

### Layout
- **Title**: "Buttons Page"
  - Position: Left-aligned in button area, Y=60
  - Scrolls with button content
  - Font: Large font (32px default)
  - Color: White (255, 255, 255)

- **Button Area** (LEFT SIDE):
  - Width: BUTTON_PADDING + BUTTON_WIDTH + BUTTON_PADDING
  - Contains scrollable buttons
  - Clipped to this area only

- **API Data Area** (RIGHT SIDE):
  - X Position: BUTTON_PADDING + BUTTON_WIDTH + 20
  - Width: Remaining screen width minus right padding
  - Fixed position (does not scroll)
  - Y Position: 150

### Buttons Configuration (9 total)
| Index | Text | Color RGB | Action |
|-------|------|-----------|--------|
| 0 | "Blue" | (41, 128, 185) | Changes background to blue |
| 1 | "Random" | (142, 68, 173) | Changes background to random color |
| 2 | "Time\nYYYY-MMM-DD" | (142, 142, 142) | Shows current time/date |
| 3 | "Go to Page 1" | (231, 76, 60) | Transitions to page 0 |
| 4 | "Refresh User" | (39, 174, 96) | Fetches new API data |
| 5 | "Exit App" | (192, 57, 43) | Quits application |
| 6 | "Button 7" | (52, 152, 219) | Debug: Cycles text color |
| 7 | "Button 8" | (241, 196, 15) | Debug: Toggles debug overlay |
| 8 | "Button 9" | (230, 126, 34) | Debug: Toggles time display |

### Button Positioning
- First button Y: `BUTTON_PADDING + 90` (90px offset for title)
- Subsequent buttons: Previous Y + BUTTON_HEIGHT + BUTTON_PADDING
- Scrollable area calculation: `90 + BUTTON_PADDING + (button_count * (BUTTON_HEIGHT + BUTTON_PADDING))`

## Visual Rendering Details

### Background
- Default Color: Dark gray (33, 33, 33)
- Configurable via config.yaml
- Changes on button press (Blue/Random buttons)

### Button States and Effects
1. **BUTTON_NORMAL**: Base color as defined
2. **BUTTON_HOVER**: Color brightened by 20% (r*1.2, g*1.2, b*1.2, clamped to 255)
3. **BUTTON_PRESSED**: Color darkened by 20% (r*0.8, g*0.8, b*0.8)
4. **BUTTON_HELD**: Pulsing effect
   - Pulse cycle: 1 second (10 steps of 100ms each)
   - Brightness varies from 80% to 120% of base color

### Button Visual Details
- **Border**: 
  - Normal: 1px border in 70% of button color
  - Interactive states: Additional 2px white outer border
- **Text**: Always white (255, 255, 255), centered
- **Multi-line text**: 30px line spacing

### Fonts
- **Regular Font**: 18px (configurable)
- **Large Font**: 32px (for titles, configurable)
- **Small Font**: 14px (for API data, configurable)
- All fonts use embedded DejaVu Sans from memory

### API Data Display
- **Line Height**: 25px
- **Text Color**: White (255, 255, 255)
- **Left-aligned** with word wrapping at max_width
- **Fields displayed** (in order):
  1. Name: [full name]
  2. Age: [age]
  3. Nationality: [nationality]
  4. Location: [location]
  5. Email: [email]
  6. Phone: [phone]
  7. "Image URL available" (if picture_url exists)

### Page Indicators
- **Visibility**: Only during swipe and 2 seconds after
- **Fade Duration**: 400ms fade out
- **Position**: 20px from bottom of screen
- **Container**:
  - Pill/capsule shape with rounded ends
  - Black background with 60% opacity (0, 0, 0, 153)
  - Height: 20px
  - Padding: 10px horizontal
- **Dots**:
  - Radius: 4px
  - Spacing: 12px between centers
  - Active page: White (255, 255, 255)
  - Inactive pages: Gray (150, 150, 150)
  - Transition alpha blending during page change

## Gesture Support

### Touch/Mouse Input
- Both SDL touch events and mouse events are supported
- Mouse clicks/drags emulate touch behavior
- Coordinates normalized for touch events (0-1 range converted to pixels)

### Gesture Recognition Thresholds
- **Click Timeout**: 1000ms (CLICK_TIMEOUT_MS)
- **Drag Threshold**: 10px (DRAG_THRESHOLD_PX)
- **Hold Timeout**: 1000ms (HOLD_TIMEOUT_MS)

### Gesture Types
1. **GESTURE_POTENTIAL**: Initial state when touch begins
2. **GESTURE_CLICK**: Quick tap under thresholds
3. **GESTURE_DRAG_VERT**: Vertical movement > threshold (scrolling)
4. **GESTURE_DRAG_HORZ**: Horizontal movement > threshold (page swipe)
5. **GESTURE_HOLD**: Touch held > 1 second (not actively used)

### Page Swipe Behavior
- **Swipe Threshold**: 30% of screen width to complete transition
- **Elastic Resistance**: 30% movement when at first/last page boundary
- **Visual Feedback**: Current page slides out, new page slides in
- **Direction Detection**: Based on initial drag direction
- **Cancellation**: Returns to original page if threshold not met

### Vertical Scrolling (Page 1 only)
- **Scroll Direction**: Inverted (drag up to scroll down)
- **Scroll Limits**: 0 to max_scroll
- **Max Scroll**: Calculated as total_content_height - screen_height
- **No momentum/inertia**: Direct 1:1 movement

## Animation Timings

### Page Transitions
- **Transition Speed**: 0.12 per frame (50% faster than original 0.08)
- **Completion**: ~8-9 frames at 60fps (~133-150ms)
- **Direction**: Negative offset for forward, positive for backward

### Debug Overlay FPS
- **Update Interval**: 1000ms
- **Position**: Bottom 55px and 30px from bottom
- **Color**: White (255, 255, 255, 128) and Light Gray (200, 200, 200, 128)

## Dynamic Elements

### Time Display (Button 2, Page 1)
- **Format**: Two lines
  - Line 1: "HH:MM:SS" (24-hour format)
  - Line 2: "YYYY-MMM-DD" (short month name)
- **Update**: Every frame when show_time is true
- **Toggle**: Button 8 on Page 1

### API Data Updates
- **Initial Fetch**: Immediately on startup
- **Manual Refresh**: Button 4 on Page 1
- **States**:
  - Loading: "Loading user data..."
  - Success: Displays all user fields
  - Error: "Error: [message]"
  - No data: "No user data loaded"

### Debug Overlay
- **Toggle**: Button 7 on Page 1
- **Line 1**: "Page: X/Y | FPS: Z | Gesture: [STATE]"
- **Line 2**: "Button: X | Transition: [STATE] (offset) | Scroll: Y/Z"
- **Position**: Bottom-left, 10px padding

## Event System Integration

### Widget Event Mirroring
- Touch events mirrored to widget integration layer
- Button presses trigger widget event handlers
- Page changes synchronized with widget system
- State changes propagate through state store

### System Events
- **system.page_transition**: Fired on page changes
- **system.api_refresh**: Fired on API refresh requests

## Configuration Integration

All visual parameters can be overridden via config.yaml:
- Colors (background, UI elements)
- Font sizes (regular, large, small)
- Animation timings
- Layout parameters (padding, margins, thresholds)

This reference captures the exact user experience of the legacy UI system, ensuring pixel-perfect recreation in the widget-based implementation.