# Deprecated LVGL Integration

These files have been moved here from the main codebase as they represent a historical integration attempt with the LVGL (Light and Versatile Graphics Library) that has been superseded by PanelKit's own rendering abstraction.

## Historical Context

Initially, LVGL was considered as the primary rendering backend for PanelKit. However, due to integration challenges and dependency issues, the project pivoted to a custom rendering abstraction that provides:

1. Better control over the rendering pipeline
2. Simpler dependency management
3. Consistent cross-platform behavior
4. Type-safe Rust-native implementation

## File Overview

- `lvgl_driver.rs` - Platform driver implementation using LVGL
- `lvgl_page.rs` - Base page implementation for LVGL pages
- `lvgl_pages.rs` - Primary LVGL page implementations
- `hello_lvgl_page.rs` - "Hello" page implementation using LVGL
- `world_lvgl_page.rs` - "World" page implementation using LVGL

## Status

The LVGL integration has been officially deprecated in PanelKit. All environments now use the rendering abstraction exclusively. The `use_lvgl` feature flag has been disabled in Cargo.toml.

These files are kept here for historical reference but are no longer maintained or used in the codebase.