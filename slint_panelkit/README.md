# PanelKit - Clean Architecture Prototype

A minimal Slint-based GUI application demonstrating clean architecture principles for embedded and desktop platforms.

## Architecture

### Module Structure
- `main.rs` - Entry point with minimal logic
- `config.rs` - Configuration and constants (no magic numbers)
- `platform.rs` - Platform-specific abstractions
- `state.rs` - Application state management
- `ui.rs` - UI creation and callback wiring
- `ui/main.slint` - Declarative UI using Slint's layout engine

### Key Improvements
- Uses `clap` for argument parsing
- Proper error handling with `anyhow`
- Platform abstractions for embedded vs desktop
- Clean state management
- Slint's layout engine (no manual calculations)
- Named constants for all configuration values

## Usage

### Development (macOS/Linux)
```bash
# Run with default dimensions (480x640)
make run

# Run with custom dimensions
make run WIDTH=640 HEIGHT=480

# Build release binary
make build
```

### Target Deployment (Raspberry Pi)
```bash
# Cross-compile for ARM64
make target

# Deploy to device (requires SSH config)
make deploy
```

### On Target Device
```bash
# Setup permissions and systemd service
make setup

# Start/stop the service
make start
make stop

# View logs
make logs
```

## Layout System
- Automatically adapts to portrait (2x4) or landscape (4x2) grid
- Uses Slint's built-in layouts (VerticalLayout, HorizontalLayout)
- ScrollView provides automatic scrolling when needed
- No manual positioning calculations

## Configuration Variables
All configuration is centralized in `config.rs`:
- Button grid dimensions
- Padding and spacing percentages
- Font size ratios
- Swipe threshold

## Platform Support
- **Desktop**: Uses winit backend for development
- **Embedded**: Uses LinuxKMS backend for direct framebuffer access
- Platform detection is automatic based on build features