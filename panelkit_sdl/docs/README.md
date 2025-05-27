# PanelKit Documentation

## Overview

PanelKit is a touch-optimized UI application framework designed for embedded Linux devices. Built on SDL2 with minimal dependencies, it provides a widget-based UI system with gesture support, API integration, and flexible configuration.

## Documentation Structure

### Architecture & Design
- [System Architecture](ARCHITECTURE.md) - High-level system design and module organization
- [Widget System](WIDGETS.md) - UI component framework and widget hierarchy
- [Event System](EVENTS.md) - Event handling and state management
- [API Integration](API.md) - External API communication and data flow

### Component Guides
- [Configuration System](CONFIGURATION.md) - YAML-based configuration and runtime settings
- [Display Backend](DISPLAY.md) - Display abstraction supporting SDL and DRM backends
- [Input Handling](INPUT.md) - Touch, mouse, and gesture recognition
- [Logging System](LOGGING.md) - Structured logging with zlog integration
- [Error Handling](ERROR_HANDLING.md) - Comprehensive error recovery system

### Development & Operations
- [Building & Deployment](DEPLOYMENT.md) - Build process, cross-compilation, and deployment

### Living Documents
- [Tracking](tracking/) - TODOs, technical debt, and work-in-progress items
- [Planning](planning/) - Future plans, proposals, and architectural ideas

## Quick Start

1. **Build for development**: `make host`
2. **Cross-compile for target**: `make target`
3. **Deploy to device**: `make deploy`
4. **Run with custom config**: `./panelkit -c custom_config.yaml`

## Key Design Principles

1. **Minimal Dependencies**: Only SDL2, libcurl, and libyaml required
2. **Embedded-First**: Designed for resource-constrained devices
3. **Touch-Optimized**: Gesture support with unified input handling
4. **Configurable**: Runtime configuration without recompilation
5. **Maintainable**: Clean architecture with separation of concerns