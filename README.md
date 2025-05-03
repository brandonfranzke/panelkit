# PanelKit

A compact, always-on graphical user interface for embedded Linux environments with touch input.

## Overview

PanelKit is designed as a self-contained appliance-like UI system for embedded devices, specifically targeting Raspberry Pi CM4/CM5 with capacitive touchscreens. It operates directly on the framebuffer without a window manager or desktop environment.

### Features

- Fullscreen operation without window manager
- Paged UI model with touch navigation
- Component-based architecture with trait abstractions
- Event-driven design with pub/sub pattern
- Platform abstraction for display and input handling
- Direct SDL2 rendering for development/simulation
- Cross-platform development for macOS and Linux
- Cross-compilation for ARM target hardware
- Clean, modular codebase written in Rust

### Current Status

- Basic architecture and abstractions implemented
- Working SDL2-based demo UI with touch event handling
- Cross-platform support for macOS without X11/XQuartz
- Trait-based platform abstraction with downcasting
- Containerized build environment for consistent development
- Cross-compilation for Raspberry Pi targets
- Deployment scripts and systemd service integration

## Development

### Requirements

- Docker (for containerized builds)
- SDL2 libraries (installed automatically for macOS if needed)
- SSH access to target device (for deployment)

No local Rust toolchain or build tools are required as all compilation occurs within containers. The macOS build target will automatically install SDL2 dependencies via Homebrew if needed.

### Architecture

PanelKit follows a layered architecture with clean separation of concerns:

- **UI Components**: Pages and widgets for user interface elements
- **Event System**: Pub/sub pattern for communication between components
- **State Management**: Centralized state with optional persistence
- **Platform Abstraction**: Cross-platform display and input drivers
- **Build System**: Containerized cross-compilation for multiple targets

### Building

The project uses a containerized build environment:

```bash
# Build the Docker containers
make build-containers

# Build for local testing in Docker
make local

# Cross-compile for Raspberry Pi
make target

# Build for macOS (compile in Docker, run natively on macOS)
make build-mac
make run-mac

# Deploy to default target device
make deploy

# Transfer to custom target
./scripts/transfer.sh --host=192.168.1.100 --user=pi
```

See [Build Guide](docs/BUILD.md) for more detailed instructions.

### Project Structure

```
panelkit/
├── src/              # Rust source code
│   ├── ui/           # UI components and rendering
│   │   ├── simple_demo_page.rs # SDL2-based demo page
│   │   └── hello_page.rs  # Simple text-based page
│   ├── state/        # State management
│   ├── platform/     # Platform-specific code
│   │   ├── sdl_driver.rs     # SDL2 driver implementation (macOS, Linux)
│   │   └── mock.rs           # Mock drivers for testing
│   ├── event/        # Event system
│   ├── lib.rs        # Library interface
│   └── main.rs       # Application entry point
├── config/           # Configuration templates
├── docs/             # Documentation
│   ├── ARCHITECTURE.md  # System architecture
│   ├── BUILD.md      # Build instructions
│   ├── DESIGN_REVISIONS.md # Evolution of design decisions
│   ├── COMPREHENSIVE_DESIGN.md # Full design details
│   └── NEXT_STEPS.md # Development roadmap
├── scripts/          # Helper scripts
│   └── transfer.sh   # Binary transfer utility
├── containers/       # Dockerfiles for build environments
├── Makefile          # Main build orchestration
└── Cargo.toml        # Rust package definition
```

### Documentation

- [Architecture](docs/ARCHITECTURE.md) - Core architecture and components
- [Build Guide](docs/BUILD.md) - Detailed build and deployment instructions
- [Comprehensive Design](docs/COMPREHENSIVE_DESIGN.md) - Detailed design decisions
- [Design Revisions](docs/DESIGN_REVISIONS.md) - Evolution of the design decisions
- [Next Steps](docs/NEXT_STEPS.md) - Development roadmap
- [macOS Cross-Compilation](docs/MAC_CROSS_COMPILATION.md) - Details on macOS build challenges

## License

[MIT License](LICENSE)