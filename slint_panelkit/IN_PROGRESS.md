# In-Progress Issues

This document captures the current active issues being addressed in the PanelKit project.

## UI Layout Issues

### Button Clipping in ScrollView

**Observed behavior**: The bottom-most buttons in the ScrollView are being clipped and not fully visible to the user. When scrolling to the bottom of the page, the last buttons are cut off.

### Orientation Display Issue

**Observed behavior**: Despite setting the orientation to "portrait" in the configuration and service file, the UI appears to be rendering in landscape orientation.

### Gray Box Background Artifact

**Observed behavior**: A small gray background box appears behind the buttons instead of showing the specified page background color.

## Cross-Compilation Environment

### Docker Build Process Reliability

**Observed behavior**: The cross-compilation process fails with an error when trying to locate the compiled binary inside the Docker container.
"Could not find the file /project/target/aarch64-unknown-linux-gnu/release/slint_panelkit in container"

## Build System Optimization

### Docker Volume Caching Inconsistency

**Observed behavior**: The build system sometimes reports "No changes detected, skipping build" when changes have been made.

## ScrollView Content Sizing

### Fixed Content Height Limitations

**Observed behavior**: When adding more content to the ScrollView, manual adjustments to the content height are required to prevent clipping.

**Steps to reproduce**:
1. Add additional buttons to the VerticalLayout inside ScrollView
2. Run the application without adjusting the content-height parameter
3. Observe that new buttons may be clipped or inaccessible
