# PanelKit

A compact, always-on graphical user interface for embedded Linux environments with touch input.

## Overview

PanelKit is designed as a self-contained appliance-like UI system for embedded devices, specifically targeting Raspberry Pi CM4/CM5 with capacitive touchscreens. It operates directly on the framebuffer without a window manager or desktop environment.

### Features

- Fullscreen operation without window manager
- Paged UI model with gesture navigation
- Component-based architecture
- Event-driven design with pub/sub pattern
- Low power operation
- Configurable via YAML
- Clean, modular codebase written in Rust

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
# Build for local testing (macOS)
make local

# Cross-compile for Raspberry Pi
make target

# Run local simulator
make run

# Deploy to device
make deploy
```

### Project Structure

```
panelkit/
├── src/              # Rust source code
│   ├── ui/           # UI components and rendering
│   ├── state/        # State management
│   ├── platform/     # Platform-specific code
│   ├── event/        # Event system
│   └── main.rs       # Application entry point
├── config/           # Configuration templates
├── docs/             # Documentation
├── containers/       # Dockerfiles for build environments
├── Makefile          # Main build orchestration
└── Cargo.toml        # Rust package definition
```

## License

[MIT License](LICENSE)