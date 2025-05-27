# PanelKit SDL

A lightweight, touch-optimized UI framework for embedded Linux devices, built on SDL2 with minimal dependencies.

## Features

- **Touch-First Design**: Native touch support with gesture recognition
- **Minimal Dependencies**: Only SDL2, libcurl, and libyaml required
- **Cross-Platform Development**: Develop on macOS/Linux, deploy to ARM64
- **Widget-Based UI**: Extensible component system
- **Event-Driven Architecture**: Decoupled communication between components
- **Configuration-Driven**: YAML-based runtime configuration
- **Production-Ready**: Systemd integration, comprehensive logging

## Quick Start

### Development Build

```bash
# Clone repository
git clone <repository-url>
cd panelkit_sdl

# Build for host development
make host

# Run with default configuration
make run

# Run with custom display size
make run DISPLAY_WIDTH=800 DISPLAY_HEIGHT=600
```

### Target Deployment

```bash
# Cross-compile for ARM64
make target

# Deploy to device
make deploy TARGET_HOST=panelkit

# On target device
cd /tmp/panelkit
make setup
make install
make start
```

## Documentation

See the [docs/](docs/) directory for detailed documentation:

- [System Architecture](docs/ARCHITECTURE.md)
- [Widget System](docs/WIDGETS.md)
- [Configuration Guide](docs/CONFIGURATION.md)
- [Building & Deployment](docs/DEPLOYMENT.md)

## Project Structure

```
panelkit_sdl/
├── src/
│   ├── app.c              # Main application
│   ├── ui/                # Widget system
│   ├── api/               # HTTP API client
│   ├── config/            # Configuration management
│   ├── display/           # Display backends
│   ├── input/             # Input handlers
│   ├── events/            # Event system
│   └── state/             # State management
├── config/
│   └── config.yaml        # Default configuration
├── deploy/                # Deployment scripts
├── test/                  # Test files
└── docs/                  # Documentation
```

## Configuration

PanelKit uses YAML configuration files with sensible defaults:

```yaml
display:
  width: 640
  height: 480
  fullscreen: false
  backend: auto  # auto, sdl, or sdl_drm

ui:
  colors:
    background: "#212121"
    primary: "#3498db"
  fonts:
    regular_size: 18
    large_size: 32

api:
  base_url: "https://randomuser.me/api/"
  timeout_ms: 5000
```

## Requirements

### Development
- SDL2 and SDL2_ttf
- libcurl
- libyaml
- CMake 3.10+
- C compiler (GCC/Clang)

### Target Device
- Linux with framebuffer or DRM support
- ARM64 architecture (or x86_64 for testing)
- Minimal system libraries

## License

[License information here]

## Contributing

[Contributing guidelines here]