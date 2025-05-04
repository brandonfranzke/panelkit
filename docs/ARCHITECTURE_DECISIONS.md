# PanelKit Architecture Decisions

This document outlines the key architectural decisions that have shaped PanelKit's design. Understanding these decisions provides insight into the technical foundations of the project.

## Initial Architecture (2024-05-03)

The initial architecture was established with these key components:

1. **UI System**
   - UI Manager with Page-based navigation
   - Trait-based component design
   - Simple events and rendering

2. **Event System**
   - Pub/sub pattern
   - Typed events

3. **State Management**
   - In-memory state with optional persistence
   - Observable state changes

4. **Platform Abstraction**
   - Display and Input driver interfaces
   - Factory pattern for driver creation

## LVGL Integration Attempt (2024-05-03)

### Decision
Initially, we planned to use LVGL as the UI framework with SDL2 as the backend for host development mode.

### Implementation
- Added LVGL dependencies to Cargo.toml
- Created initial bindings for LVGL in platform drivers
- Attempted to set up proper configuration paths

### Challenges Encountered
- Compilation issues with LVGL Rust bindings
- Configuration path problems
- Difficulty matching C library with Rust bindings

### Resolution
After multiple attempts to resolve LVGL compilation issues, we decided to pivot to a simpler approach:

1. Remove LVGL dependencies temporarily
2. Implement a direct SDL2-based UI for the initial proof-of-concept
3. Postpone LVGL integration to a future phase
4. Focus on getting core architecture working first

## Simplified SDL2 Implementation (2024-05-03)

### Decision
Create a simplified UI implementation using SDL2 directly, without LVGL.

### Implementation
- Removed LVGL dependencies
- Created `simple_demo_page.rs` with direct SDL2 rendering
- Updated the `SDLDriver` to provide a canvas to UI components
- Implemented basic touch event handling

### Key Changes
1. Modified `UIManager` to work with the simplified demo page
2. Updated platform abstraction to handle SDL2 directly
3. Added canvas sharing between the driver and UI components

## Trait Enhancement for Downcasting (2024-05-03)

### Decision
To allow UI components to access concrete driver implementations for specialized functionality (like SDL2's canvas), we needed to add support for downcasting.

### Implementation
1. Extended platform traits with `as_any()` and `as_any_mut()` methods
2. Added trait bounds to ensure proper type support
3. Implemented methods for all driver types
4. Created safe downcasting patterns in UI Manager

### Key Changes
- Updated `DisplayDriver` and `InputDriver` traits
- Added implementations for `MockDisplayDriver`, `MockInputDriver`, `CombinedMockDriver`, and `SDLDriver`
- Implemented downcasting in the UI Manager for canvas access

## Driver Trait Refinement (2024-05-03)

### Decision
The initial design used trait composition (`DisplayDriver + InputDriver`), which caused compilation issues with trait objects. We refined this to use a more explicit approach.

### Challenge
Rust's limitations with trait composition in trait objects (error E0225: only auto traits can be used as additional traits in a trait object) required a design change.

### Implementation
1. Created a standalone `Driver` trait that incorporates methods from both display and input functionality
2. Implemented this trait for `SDLDriver` and `CombinedMockDriver`
3. Updated usage throughout the codebase to work with the new trait
4. Resolved method disambiguation issues

### Key Changes
- Replaced trait composition with a unified `Driver` trait
- Added explicit method implementations
- Used trait methods like `init()`, `init_input()`, `poll_events()`, etc.
- Updated application initialization code

## Cross-Platform Development and Deployment (2024-05-03)

### Decision
For cross-platform development (specifically macOS development machine targeting Raspberry Pi), we decided to focus on a clean transfer approach rather than native macOS builds.

### Implementation
1. Created a transfer script (`scripts/transfer.sh`) to simplify deploying to target devices
2. Updated Makefile with a `transfer` target
3. Added command-line options for target host, user, and directory
4. Streamlined the development-to-deployment workflow

### Key Changes
- Added `transfer` support to Makefile
- Created configurable transfer script
- Ensured proper execution permissions on target device
- Removed any MacOS-specific build targets for simplicity

## Error Handling Improvements (2024-05-03)

### Decision
Improve error handling in SDL2 rendering to properly propagate errors through the application.

### Implementation
1. Enhanced error handling in SDL2 canvas operations
2. Used `anyhow` for error context and chaining
3. Added proper error conversion from SDL2 errors

### Key Changes
- Updated SDL2 rendering code with specific error handling
- Added context to error messages
- Ensured errors would be properly propagated and logged

## UI Component Design (2024-05-03)

### Decision
Create a simple but effective UI component model that can be rendered with SDL2.

### Implementation
1. Created a basic demo page with:
   - Title area
   - Counter display
   - Interactive button
   - Slider component
2. Implemented touch event handling for interactive elements
3. Structured UI components for easy extension

### Key Changes
- Added `SimpleDemoPage` implementation
- Created basic UI elements and layout
- Implemented touch event handling and state updates

## Build System Refinement (2024-05-03)

### Decision
Refine the build system to ensure it works properly across platforms without unnecessary complexity.

### Implementation
1. Updated Dockerfiles for cleaner builds
2. Refined Makefile targets for more explicit build steps
3. Ensured Docker containers have the right dependencies
4. Created a simple, reproducible build process

### Key Changes
- Streamlined build container definitions
- Reduced unnecessary make targets
- Improved build flags and environment variables
- Added consistent error handling in build scripts

## macOS Native Build Support (2024-05-03)

### Decision
To enable cross-platform development without requiring X11/XQuartz on macOS, we implemented a Docker-based build system that produces macOS-compatible binaries.

### Rationale
Our design goals include maintaining a clean, containerized build process while minimizing host dependencies. The macOS-native approach allows us to:
- Build in Docker containers for consistency
- Run natively on macOS without X11/XQuartz dependencies
- Maintain a consistent development environment
- Provide full UI functionality on macOS for development and testing

### Implementation
1. Updated the Makefile with macOS-specific build targets:
   - `build-mac`: Compiles in Docker and outputs a macOS-compatible binary
   - `run-mac`: Runs the binary natively on macOS with SDL2
2. Added SDL2 dependency management with Homebrew
3. Updated platform factory to choose the appropriate driver
4. Refined SDL driver to work optimally on macOS

### Key Changes
1. Created macOS-specific build targets in the Makefile
2. Updated the SDL2 driver with macOS-specific configurations
3. Simplified platform selection logic
4. Updated documentation to explain the macOS approach
5. Removed display server dependencies for development builds

## Future Directions

Based on the implementation experience and design evolution, these are the key next steps:

1. Complete the SDL2-based proof-of-concept
2. Revisit LVGL integration with improved understanding of binding requirements
3. Expand the UI component library beyond the initial demo
4. Implement proper state persistence
5. Add more comprehensive platform drivers for target hardware
6. Create proper testing infrastructure
7. Implement actual framebuffer driver for Raspberry Pi target

## Lesson Learned

1. **Start Simple**: Begin with direct implementations before adding abstractions
2. **Platform Independence**: Keep platform-specific code isolated and well-abstracted
3. **Progressive Enhancement**: Get a basic version working before adding advanced features
4. **Clear Documentation**: Keep design decisions documented for future reference
5. **Flexible Architecture**: Design for change and evolving requirements