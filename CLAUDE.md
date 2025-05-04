# PanelKit Project Memory

## Project Overview

PanelKit is a Rust-based embedded UI application designed to run on Raspberry Pi devices with touch screens. It provides a page-based UI system with platform abstraction for both simulation (using SDL2) and target hardware.

## Recent Changes

- Refactored unsafe downcasting in UI manager to use safe type-based approach
- Unified platform driver interface to replace separate display/input interfaces
- Replaced feature flag conditionals with runtime polymorphism
- Improved error handling with proper context
- Streamlined build system (Makefile) and consolidated Docker containers
- Added comprehensive API documentation (docs/API.md)
- Updated all project documentation to reflect architectural improvements

## Key Files

- `src/ui/mod.rs`: UI manager and Page trait implementation
- `src/platform/mod.rs`: Platform abstraction layer with unified PlatformDriver trait
- `src/platform/sdl_driver.rs`: SDL2-based driver for simulation
- `src/platform/mock.rs`: Mock implementation for testing
- `src/lib.rs`: Main library implementation
- `src/main.rs`: Application entry point
- `Makefile`: Build system commands
- `containers/Dockerfile`: Consolidated build environment

## Architecture

- **UI System**: Page-based navigation with type-safe downcasting
- **Platform Abstraction**: Unified PlatformDriver interface with GraphicsContext
- **Event System**: Simple pub/sub pattern with typed events
- **State Management**: In-memory with persistence options

## Build Commands

- `make check-deps`: Check for required dependencies
- `make build`: Build for current platform
- `make run`: Run on current platform
- `make clean`: Clean build artifacts

## Testing

- Basic testing through simulator mode
- Consider adding unit tests in future revisions

## Next Steps

- Implement proper framebuffer driver for Raspberry Pi
- Expand UI component library
- Add state persistence
- Improve touch event handling and gestures
