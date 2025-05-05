# PanelKit Comprehensive Design Document

This document serves as a comprehensive reference for all design decisions, architecture, requirements, and implementation details for the PanelKit embedded UI system. In case of disconnection or when consulting with future AI assistants, this document should provide all necessary context.

## Project Overview

PanelKit is a compact, always-on graphical user interface application designed for embedded Linux environments with touch input. It is specifically built to run on Raspberry Pi CM4/CM5 with a Waveshare capacitive touch (2.8") screen as a self-contained appliance-like system with no visible OS or desktop environment.

### Core Requirements

1. **Run fullscreen with no window manager**
   - Direct framebuffer or lightweight rendering backend
   - No desktop, no title bars, no cursor, no shell
   - Boot directly into the application with no OS feedback

2. **Support a paged UI model**
   - Multiple discrete "pages" or screens 
   - Swipe or tap navigation between pages
   - Dynamic content (text labels, state indicators, toggles)

3. **Dynamic interaction and state tracking**
   - UI elements reflect and modify underlying state
   - Central state model accessible to UI and backend logic

4. **Modular and maintainable architecture**
   - Clean separation of concerns
   - Well-defined interfaces between components
   - Maintainable and extensible codebase

5. **Cross-development**
   - Compile for ARM embedded targets (cross-compile)
   - Native desktop version for host development
   - Containerized builds for clean host system

6. **Clean and reproducible build workflow**
   - Cargo-based build system
   - Centralized Makefile for orchestration
   - Docker containers for isolation

7. **Auto-start on boot**
   - systemd service for auto-launch
   - Disable unnecessary OS feedback

8. **Minimal external dependencies**
   - Lean runtime libraries
   - Well-supported in embedded Linux environments

### Hardware Target

- **Primary Hardware**: Raspberry Pi CM4/CM5
- **Display**: Waveshare capacitive touch (2.8")
- **Power Source**: Potentially battery powered (low power is important)
- **Future Expansion**: May include IR emitters and other hardware interfaces

## Technology Decisions

### Language Choice: Rust

**Decision**: Use Rust for the entire application codebase.

**Rationale**:
- Memory safety without garbage collection
- Strong typing and ownership model (ideal for UI events)
- Modern language features (pattern matching, traits)
- Clean abstractions for maintainable code
- Support for cross-compilation to ARM targets
- Strong performance characteristics

**Alternatives Considered**:
- C++: Rejected due to memory safety concerns and less modern language features, despite good performance
- C: Rejected due to lack of abstractions that would be useful for an event-driven UI application

### UI Framework: LVGL

**Decision**: Use LVGL (Light and Versatile Graphics Library) with SDL2 for development/simulation.

**Rationale**:
- Purpose-built for embedded systems
- Low resource requirements (good for battery operation)
- Rich built-in widget set with gesture support
- Direct framebuffer or DRM support for target hardware
- SDL2 support for desktop development
- No X/display manager requirements

**Key Implementation Strategy**:
- Use LVGL on top of SDL2 for desktop development
- Switch to direct framebuffer for target deployment
- Abstract the display and input drivers

### Build System: Containerized Cargo

**Decision**: Use Rust's Cargo build system within Docker containers.

**Rationale**:
- No need for local toolchain installation
- Consistent build environment
- Separate containers for native vs cross-compilation
- Volume mounts for source code and artifacts

**Implementation**:
- Debian-based containers
- Makefile as central workflow interface
- Docker containers that don't run the application, just build it

### Configuration: YAML

**Decision**: Use YAML for configuration files.

**Rationale**:
- More readable than JSON
- Good Rust library support
- Human-editable
- Configuration parsed at startup, not requiring runtime parsing

### State Management: Custom KV Store

**Decision**: Use a lightweight key-value store (redb) with in-memory caching.

**Rationale**:
- Allows persistence between reboots when needed
- Lightweight storage with good performance
- Schematized keys for future expansion

### Logging: spdlog-rs

**Decision**: Use spdlog-rs for logging with stdout/stderr output.

**Rationale**:
- Lightweight but comprehensive
- Multiple log levels
- Good Rust integration
- Output to stdout/stderr for systemd capture

## Architectural Design

### Core Components

1. **UI System**
   - **UI Manager**: Controls page navigation and widget hierarchy
   - **Page**: Full-screen interfaces with multiple widgets
   - **Widget**: Individual UI elements (buttons, labels, etc.)
   - Composition-based component design

2. **Event System**
   - **Trait-based events**: Strong type safety through Rust's trait system
   - **Event propagation phases**: Capturing, at-target, and bubbling phases
   - **EventBus**: Type-safe pub/sub message dispatcher
   - **EventHandler trait**: Components implement this for event processing
   - **Event cloning mechanism**: For multi-subscriber scenarios
   - **Box<dyn Event> support**: For use with trait objects

3. **State Management**
   - **State Manager**: Thread-safe state container
   - **Repository Pattern**: For configuration and persistence
   - **Observable state**: Notifies subscribers of changes

4. **Platform Abstraction**
   - **Display Driver**: Rendering to screen (SDL2 or framebuffer)
   - **Input Driver**: Touch and gesture processing
   - **Platform Factory**: Creates appropriate implementations
   - **Trait-based interfaces** for all hardware interactions

### Data Flow

1. Input events are captured by platform layer
2. Events are published to the event system using typed channels
3. UI components subscribe to relevant events
4. State changes are persisted as needed
5. UI is re-rendered based on state changes

### Threading Model

- Single UI thread for touch events and rendering
- Async tasks for network calls and background operations
- Message passing between components

### Power Management Strategy

1. **Event-Driven Updates**: Only render on state changes
2. **Variable Refresh Rates**: Slow UI refresh when inactive
3. **Component Sleep States**: Background components pause polling
4. **Display Backlight Control**: Dim/off on inactivity

### Network Handling

1. **Service Layer**: Abstract network dependencies behind traits
2. **Network Context**: Global connectivity state that components can query
3. **Fallback Modes**: Components define network/offline behaviors

### Error Handling Strategy

- Development: Fail-fast to catch issues early
- Production: Component-level isolation (invalidate component on error)
- Structured logging for remote diagnostics

## Implementation Details

### Project Structure

```
panelkit/
├── src/              # Rust source code
│   ├── ui/           # UI components and rendering
│   │   ├── mod.rs    # UI module definitions and Page trait
│   │   └── hello_page.rs # Example page implementation
│   ├── event/        # Event system
│   │   └── mod.rs    # Event types and broker
│   ├── state/        # State management
│   │   └── mod.rs    # StateManager implementation
│   ├── platform/     # Platform-specific code
│   │   ├── mod.rs    # Platform traits and factory
│   │   └── mock.rs   # Mock implementations for testing
│   ├── lib.rs        # Library interface
│   └── main.rs       # Application entry point
├── config/           # Configuration templates
├── docs/             # Documentation
├── containers/       # Dockerfiles for build environments
│   ├── Dockerfile.native # For Mac/local builds
│   └── Dockerfile.cross  # For ARM cross-compilation
├── Makefile          # Build orchestration
├── Cargo.toml        # Rust package definition
└── .dockerignore     # Docker build exclusions
```

### Deployment Strategy

- Application deployed via rsync/scp
- systemd service starts application on boot
- Error logs captured by systemd journal
- No special installation requirements other than the binary itself

### Command Line Options

- `--log-level`: Set logging verbosity
- `--dimensions`: Set display dimensions (default: auto)
- `--fullscreen`: Control fullscreen mode (default: true)
- `--state-path`: Path to state database (optional)

### Feature Flags

- `host`: Enable SDL2-based simulation on desktop
- `embedded`: Enable optimizations and features for target hardware

## Roadmap and Future Considerations

### Short-term Goals

1. Test containerized build system
2. Implement LVGL integration
3. Add real platform drivers (SDL2 and framebuffer)
4. Complete state persistence implementation

### Medium-term Goals

1. Create comprehensive UI widget library
2. Implement animations and transitions
3. Add network status monitoring
4. Create settings page and configuration UI

### Long-term Goals

1. Add power management optimizations
2. Design hardware abstraction for expandability
3. Add comprehensive testing
4. Implement CI/CD pipeline

## Development Guidelines

1. **Code Style**
   - Follow Rust idioms and conventions
   - Use meaningful variable and function names
   - Keep functions small and focused
   - Write comprehensive documentation

2. **Testing**
   - Unit test core functionality
   - Test on both simulator and target hardware
   - Test power management and resource usage

3. **Version Control**
   - Make incremental, focused commits
   - Use descriptive commit messages
   - Keep features in separate branches

4. **Documentation**
   - Document all public APIs
   - Keep design documents updated with changes
   - Include rationale for significant decisions

## Technical Constraints and Considerations

1. **Performance**
   - Target hardware has limited processing power
   - UI must remain responsive at all times
   - Battery efficiency is critical

2. **Display Limitations**
   - Small 2.8" screen size requires careful UI design
   - Touch input requires appropriate sizing of interactive elements
   - Limited resolution (typically 320x240 for 2.8" displays)

3. **Connectivity**
   - System should gracefully handle network dropouts
   - Essential functions should work offline
   - Network status should be clearly indicated to the user

4. **Direct Rendering**
   - No X11 or Wayland compositor
   - Direct framebuffer or DRM rendering
   - Custom input handling without X input drivers

## Design Patterns and Idioms

1. **Actor Pattern**
   - Components communicate via message passing
   - Helps maintain clean separation of concerns

2. **Repository Pattern**
   - For configuration and state persistence
   - Abstracts storage details from application logic

3. **Strategy Pattern**
   - For platform-specific implementations
   - Allows swapping implementations without changing core logic

4. **Observer Pattern**
   - For event notifications
   - Implemented via the pub/sub event system

5. **RAII (Resource Acquisition Is Initialization)**
   - Leverage Rust's ownership model for resource management
   - Ensures proper cleanup of resources

## Libraries and Dependencies

1. **Core**
   - `log`: Logging facade
   - `spdlog-rs`: Logging implementation
   - `clap`: Command-line argument parsing
   - `anyhow`: Error handling
   - `thiserror`: Error definitions

2. **Serialization**
   - `serde`: Serialization framework
   - `serde_yaml`: YAML parsing

3. **Concurrency**
   - `tokio`: Async runtime
   - `crossbeam-channel`: Multi-producer, multi-consumer channels

4. **UI/Graphics**
   - `lvgl`: UI framework
   - `sdl2`: Cross-platform graphics for simulation

5. **Storage**
   - `redb`: Embedded key-value store

## Specific Implementation Insights

### UI Rendering Process

1. UI Manager maintains page hierarchy
2. Current page is rendered on event or timer
3. Pages compose widgets into layouts
4. LVGL handles actual rendering
5. Platform driver flushes to display

### Event Propagation

1. Platform driver captures hardware events
2. Events are converted to typed event objects
3. Event broker dispatches to subscribers
4. Components process events and update state
5. State changes trigger UI updates

### State Persistence

1. State changes are stored in memory
2. Critical state is persisted to disk
3. State is loaded at startup
4. Components observe state changes via event system

### Gesture Recognition

1. Raw touch events from input driver
2. Gesture detector analyzes patterns
3. Detected gestures published as higher-level events
4. Pages and components respond to gestures

### Power Management

1. Activity monitoring tracks user interaction
2. Inactivity triggers progressive power saving
3. Display dimming/sleep controlled by platform driver
4. Background tasks pause during low-power states

This comprehensive document captures all key design decisions, architecture, and implementation details discussed during our planning phase. It should serve as a complete reference for continuing development of the PanelKit embedded UI system.