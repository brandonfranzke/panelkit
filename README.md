# PanelKit

A compact, always-on graphical user interface framework for embedded Linux environments with touch input.

## Overview

PanelKit is designed as a self-contained appliance-like UI system for embedded devices, specifically targeting Raspberry Pi CM4/CM5 with capacitive touchscreens. It operates directly on the framebuffer without requiring a window manager or desktop environment.

### Features

- **Cross-Platform Development**: Develop on macOS/Linux, deploy to embedded targets
- **Unified Interface**: Single, coherent API across development and production
- **Event-Driven Architecture**: Robust pub/sub system for component communication
- **Page-Based Navigation**: Structured UI organization with navigation
- **Platform Abstraction**: Clean separation between UI logic and hardware specifics
- **Safe Typing**: Type-safe downcasting for platform-specific features
- **Comprehensive Error Handling**: Context-rich errors with recovery paths
- **Modular Design**: Clean component separation with well-defined interfaces
- **Containerized Builds**: Docker-based cross-compilation for target hardware

### Current Status

PanelKit is in active development with:

- Core application architecture and runtime implemented
- Unified platform driver interface with multiple implementations
- Rendering abstraction layer with pluggable backends (SDL2, framebuffer)
- UI manager with page navigation and safe component access
- Event system with typed events and pub/sub messaging
- State management framework with serialization support
- Build system for both host development and embedded deployment
- Comprehensive error handling with context and recovery

## Getting Started

### Requirements

- Rust (install with `brew install rust`)
- SDL2 libraries (install with `brew install sdl2`)
- Docker (only for cross-compilation to Raspberry Pi)

Verify all dependencies are installed with:

```bash
make check-deps
```

### Quick Start

Build and run the application on your development machine:

```bash
# Build for local development
make host

# Run the application
make run

# Run the application with rendering abstraction layer enabled
./scripts/run_with_rendering.sh
```

Deploy to a Raspberry Pi:

```bash
# Cross-compile for ARM
make target

# Deploy to the target device
make deploy TARGET_HOST=raspberrypi.local TARGET_USER=pi

# Run on the target with rendering abstraction enabled
ssh pi@raspberrypi.local "cd /home/pi/panelkit && PANELKIT_USE_RENDERING=1 RUST_LOG=debug ./panelkit"
```

### Architecture

PanelKit follows a layered architecture with clean separation of concerns:

```
┌───────────────┐     ┌───────────────┐     ┌───────────────┐
│  Application  │     │  UI Manager   │     │  Pages        │
│  - Core loop  │◄───►│  - Navigation │◄───►│  - Content    │
│  - Lifecycle  │     │  - Layout     │     │  - Interaction│
└───────┬───────┘     └───────┬───────┘     └───────────────┘
        │                     │
        ▼                     ▼
┌───────────────┐     ┌───────────────┐     ┌───────────────┐
│ Event System  │     │Platform Driver │     │ State Manager │
│ - Pub/sub     │◄───►│ - Display     │◄───►│ - Data store  │
│ - Dispatching │     │ - Input       │     │ - Persistence │
└───────────────┘     └───────┬───────┘     └───────────────┘
                              │
                              ▼
                     ┌───────────────────┐
                     │Rendering Backend  │
                     │ - SDL2 (Host)     │
                     │ - Framebuffer     │
                     │   (Embedded)      │
                     └───────────────────┘
```

The architecture includes a rendering abstraction layer that provides a clean interface for different rendering backends, allowing the same UI code to run on both host (SDL2) and embedded (framebuffer) targets.

For more details, see the [Architecture Documentation](docs/ARCHITECTURE.md).

## Development

### Building

The project provides targets for both development and deployment:

```bash
# Local development build
make host

# Run the development build
make run

# Cross-compile for Raspberry Pi
make target

# Deploy to target device
make deploy

# Set up as a systemd service
make install-service

# Clean build artifacts
make clean
```

For detailed build options, run `make help`.

### Creating Custom Pages

Extend the `Page` trait to create custom UI screens:

```rust
use panelkit::ui::Page;

struct MyCustomPage {
    // Page state
}

impl Page for MyCustomPage {
    fn init(&mut self) -> anyhow::Result<()> {
        // Initialize page
        Ok(())
    }
    
    fn render(&self) -> anyhow::Result<()> {
        // Render page content
        Ok(())
    }
    
    // ... other trait methods
}
```

See the [API Documentation](docs/API.md) for more details.

### Project Structure

```
panelkit/
├── src/              # Rust source code
│   ├── ui/           # UI components and rendering
│   ├── platform/     # Platform abstraction
│   ├── event/        # Event system
│   ├── state/        # State management
│   ├── lib.rs        # Core application code
│   └── main.rs       # Entry point
├── docs/             # Documentation
├── config/           # Configuration templates
├── containers/       # Docker build environment
├── scripts/          # Helper scripts
├── Makefile          # Build orchestration
└── Cargo.toml        # Rust dependencies
```

## Documentation

- [Architecture](docs/ARCHITECTURE.md) - System design and components
- [Rendering Architecture](docs/RENDERING_ARCHITECTURE.md) - Rendering abstraction design
- [Build Guide](docs/BUILD.md) - Detailed build and deployment instructions
- [API Reference](docs/API.md) - Developer interface documentation
- [Next Steps](docs/NEXT_STEPS.md) - Development roadmap

## License

[MIT License](LICENSE)