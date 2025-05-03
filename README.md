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
- Cross-compilation for target hardware
- Clean, modular codebase written in Rust

### Current Status

- Basic architecture and abstractions implemented
- Working SDL2-based demo UI
- Touch event handling
- Containerized build with cross-compilation support
- Deployment scripts for target devices

## Development

### Requirements

- Docker (for containerized builds)
- SSH access to target device (for deployment)

No local build tools are required as all compilation occurs within containers.

### Architecture

PanelKit follows a layered architecture:

- UI Components (Pages, Widgets)
- Event System (Pub/Sub)
- State Manager
- Platform Services (Display, Input, Network)

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
│   │   ├── sdl_driver.rs     # SDL2 driver implementation
│   │   ├── headless_driver.rs # Console-based driver (no display server needed)
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
- [Comprehensive Design](docs/COMPREHENSIVE_DESIGN.md) - Detailed design decisions
- [Design Revisions](docs/DESIGN_REVISIONS.md) - Evolution of the design
- [Build Guide](docs/BUILD.md) - Build and deployment instructions
- [Next Steps](docs/NEXT_STEPS.md) - Development roadmap

## License

[MIT License](LICENSE)