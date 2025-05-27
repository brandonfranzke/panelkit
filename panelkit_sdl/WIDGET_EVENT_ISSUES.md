# Widget Event System Issues

## Current Problems

### UPDATE: Issues Found and Fixed

1. **Child Widget Bounds Not Updated**: When page manager moved pages horizontally, child widget absolute positions weren't updated. Fixed by adding `widget_update_child_bounds()`.

2. **Text Widgets Using Absolute Instead of Relative Positions**: Text widgets inside buttons were positioned using absolute screen coordinates instead of relative-to-parent coordinates. This caused text to appear in wrong locations when parent moved.

## Remaining Problems

### FIXED Issues:

1. **Button Event Timing**: Widget manager was clearing PRESSED state before button could check it. Fixed by checking state before calling default handler.

2. **Event Propagation**: All widgets were receiving all events instead of just the clicked widget. This is by design but caused confusion.

3. **Button Event Data**: Each button now correctly publishes its own event data.

### Still Active Issues:

1. **Duplicate Event Handling**
**Symptom**: Some events appear to be handled twice

**Root Cause**: Need to investigate if:
- Event data is being shared between buttons
- Event data is being overwritten during setup
- Widget hit test is returning wrong widget

**Debug Output**:
```
[DEBUG] WIDGET HIT: id='page1_button3' type=1 at click pos (174,237), widget bounds=(10,160,193x140)
[WIDGET_INTEGRATION_DEBUG] Widget button click handler: page=1 button=0 text='Change Text Color'
```

### 2. Page Background Clicks
**Symptom**: Clicking on page background (not on any button) triggers events

**Root Cause**: 
- Page widget has event handler for scrolling
- Hit test returns page widget when no button is hit
- Need to distinguish between "scrollable" and "clickable" widgets

### 3. Text Color Changes on Wrong Events
**Symptom**: Text color changes when clicking certain areas, not related to the actual button action

**Root Cause**: Unknown - needs investigation

## Legacy Touch/Click Behaviors We're Affecting

### Original Behaviors to Preserve:
1. **Button Press Visual Feedback**: Buttons should show pressed state
2. **Swipe Gestures**: Horizontal swipes change pages
3. **Scroll Support**: Vertical scrolling on pages with overflow content
4. **Touch/Mouse Unification**: Single event flow for both input types
5. **Drag vs Click Detection**: Distinguish between drag gestures and clicks

### What We've Changed:
1. **Hit Testing**: Modified `widget_hit_test` to prefer interactive widgets (buttons) over containers
2. **Event Routing**: Changed from direct page widget to page_manager routing
3. **Button Event Data**: Each button has its own event data structure

## Fixes Needed

### Immediate:
1. Debug why all buttons share same event data
2. Fix page background click issue
3. Implement proper swipe gesture support

### Long-term:
1. Create clear separation between WIDGET_RENDER and legacy modes
2. Remove dependencies on global state in widget mode
3. Implement proper event bubbling/capturing