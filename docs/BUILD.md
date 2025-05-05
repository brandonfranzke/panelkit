# PanelKit Build Guide

This document explains how to build, run, and deploy the PanelKit embedded UI system.

## Prerequisites

- Rust (install with `brew install rust` on macOS or use rustup.rs)
- SDL2 libraries (install with `brew install sdl2` on macOS)
- Docker (only needed for cross-compilation to Raspberry Pi)
- Make

## Build Approach

PanelKit uses a hybrid build approach:

1. **Local Development Builds**
   - Direct native compilation using local Rust toolchain
   - Fast iteration, simple debugging, no containerization overhead
   - Uses SDL2 for graphics rendering and input simulation

2. **Target Cross-Compilation**
   - Uses Docker container for consistent cross-compilation environment
   - Produces ARM binaries for deployment to Raspberry Pi
   - Clean isolation of build dependencies

## Development Workflow

### Checking Dependencies

Verify that all required dependencies are installed:

```bash
make check-deps
```

### Building for Development

Build the application for local development:

```bash
make host
```

This builds the application with host features enabled directly on your machine.

### Running the Application

Run the development build:

```bash
make run
```

This starts the application with a graphical UI for testing.

### Cross-Compiling for Raspberry Pi

Build for the Raspberry Pi embedded target:

```bash
make target
```

This builds the Docker container (if needed) and cross-compiles the application for ARM architecture. The binary is placed in the `build/` directory.

## Deployment

### Deploying to Target Device

Deploy to the target Raspberry Pi:

```bash
make deploy
```

This uses the default settings for target hostname (`raspberrypi.local`), username (`pi`), and directory (`/home/pi/panelkit`).

You can customize these settings:

```bash
make deploy TARGET_HOST=mypi.local TARGET_USER=admin TARGET_DIR=/opt/panelkit
```

### Setting Up Auto-Start Service

Create a systemd service for auto-starting on boot:

```bash
make create-service
make install-service
```

This creates a systemd service file and installs it on the target device.

## Project Structure

```
panelkit/
├── src/                # Rust source code
│   ├── ui/             # UI components and rendering
│   │   ├── mod.rs      # UI manager and Page trait
│   │   └── *_page.rs   # Page implementations
│   ├── platform/       # Platform abstraction
│   │   ├── mod.rs      # Driver interfaces
│   │   ├── sdl_driver.rs # SDL2 implementation
│   │   └── mock.rs     # Mock implementation
│   ├── event/          # Event system
│   ├── state/          # State management
│   ├── lib.rs          # Application implementation
│   └── main.rs         # Entry point
├── docs/               # Documentation
├── config/             # Configuration files
├── containers/         # Docker build environments
├── scripts/            # Helper scripts
├── Makefile            # Build orchestration
└── Cargo.toml          # Rust dependencies
```

## Advanced Usage

### Custom Dimensions

You can specify custom screen dimensions:

```bash
./build/panelkit-macos --dimensions 640x480
```

### Debug Logging

Enable detailed logging with:

```bash
RUST_LOG=debug ./build/panelkit-macos
```

### Clean Build

Remove all build artifacts:

```bash
make clean
```

## Runtime Platform Selection

PanelKit uses runtime detection instead of compile-time feature flags:

- **Single Binary**: The application can adapt to different environments at runtime
- **Auto-Detection**: Automatically selects appropriate backend based on environment
- **Environment Override**: Use `PANELKIT_EMBEDDED=1` or `PANELKIT_PLATFORM=host|embedded`
- **API Selection**: Programmatically select platform with `AppConfig::target_platform`

## Troubleshooting

### SDL2 Library Issues

If you encounter SDL2 library loading errors:

1. Verify SDL2 is properly installed:
   ```bash
   brew list sdl2
   ```

2. Check DYLD paths with:
   ```bash
   DYLD_PRINT_LIBRARIES=1 ./build/panelkit-macos
   ```

### Cross-Compilation Issues

Common cross-compilation problems:

1. **Missing Docker**: Ensure Docker is running
2. **ARM Toolchain**: The Docker container should include all needed tools
3. **Disk Space**: Docker builds require sufficient disk space

### Deployment Issues

If deployment fails:

1. Verify SSH connectivity:
   ```bash
   ssh [TARGET_USER]@[TARGET_HOST]
   ```
2. Check file permissions on target device
3. Verify network connectivity and firewall settings