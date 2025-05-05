# PanelKit

A compact, always-on graphical user interface framework for embedded Linux environments with touch input.

## Overview

PanelKit is designed as a self-contained appliance-like UI system for embedded devices, specifically targeting Raspberry Pi CM4/CM5 with capacitive touchscreens. It operates directly on the framebuffer without requiring a window manager or desktop environment.

### Features

- **Cross-Platform Development**: Develop on macOS/Linux, deploy to embedded targets
- **Runtime Platform Detection**: Automatically detects and adapts to the current platform
- **Unified Rendering Interface**: Single, coherent API across development and production
- **Advanced Event System**: Type-safe, trait-based events with proper propagation phases
- **Page-Based Navigation**: Structured UI organization with navigation
- **Platform Abstraction**: Clean separation between UI logic and hardware specifics
- **Safe Typing**: Type-safe downcasting for platform-specific features
- **Comprehensive Error Handling**: Context-rich errors with recovery paths
- **Modular Design**: Clean component separation with well-defined interfaces
- **Containerized Builds**: Docker-based cross-compilation for target hardware

### Current Status

PanelKit is in active development with:

- Core application architecture and runtime implemented
- Runtime polymorphism for platform and backend selection
- Unified platform driver interface with multiple implementations
- Consolidated rendering primitives and abstraction layer
- Pluggable rendering backends (SDL2, framebuffer) with automatic detection
- UI manager with page navigation and safe component access
- Comprehensive event system with trait-based events, propagation phases, and thread safety
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
# Build for development
make build

# Run the application (auto-detects platform)
make run
```

Deploy to a Raspberry Pi:

```bash
# Cross-compile for ARM
make cross-compile

# Deploy to the target device
make deploy TARGET_HOST=raspberrypi.local TARGET_USER=pi

# Run on the target (auto-detects platform)
ssh pi@raspberrypi.local "cd /home/pi/panelkit && RUST_LOG=debug ./panelkit"
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
│ - Type-safe   │◄───►│ - Display     │◄───►│ - Data store  │
│ - Propagation │     │ - Input       │     │ - Persistence │
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

The architecture uses runtime polymorphism with a unified rendering abstraction layer that provides a clean interface for different backends. The system automatically detects the appropriate platform and rendering backend at runtime, allowing the same binary to adapt to both host (SDL2) and embedded (framebuffer) environments.

For more details, see the [Architecture Documentation](docs/ARCHITECTURE.md).

## Development

### Building

The project provides build targets for development and deployment:

```bash
# Development build (with runtime platform detection)
make build

# Run the application (auto-detects platform)
make run

# Cross-compile for Raspberry Pi
make cross-compile

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
use panelkit::primitives::{RenderingContext, Color, Point, Rectangle, TextStyle};
use anyhow::Result;

struct MyCustomPage {
    // Page state
    width: u32,
    height: u32,
}

impl Page for MyCustomPage {
    fn init(&mut self) -> Result<()> {
        // Initialize page
        Ok(())
    }
    
    fn render(&self, ctx: &mut dyn RenderingContext) -> Result<()> {
        // Clear background
        ctx.clear(Color::rgb(240, 240, 240))?;
        
        // Draw content with unified rendering abstraction
        ctx.fill_rect(
            Rectangle::new(10, 10, 100, 50),
            Color::rgb(0, 120, 215)
        )?;
        
        ctx.draw_text(
            "Hello World",
            Point::new(60, 35),
            TextStyle {
                font_size: panelkit::primitives::FontSize::Medium,
                color: Color::rgb(255, 255, 255),
                alignment: panelkit::primitives::TextAlignment::Center,
                bold: false,
                italic: false,
            }
        )?;
        
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