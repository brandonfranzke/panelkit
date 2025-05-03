# macOS Cross-Compilation

This document outlines the challenges and potential solutions for cross-compiling the PanelKit application for macOS from a Docker environment.

## Challenges

Unlike cross-compilation for Windows using MinGW (as shown in the sample Dockerfile), cross-compilation for macOS presents several unique challenges:

1. **Apple SDK Requirement**: Building for macOS requires the macOS SDK, which is typically only available on macOS systems.

2. **Signing Requirements**: macOS applications often require code signing to run properly.

3. **Framework Dependencies**: SDL2 on macOS uses the framework model, which differs from Linux shared libraries.

4. **Architecture Differences**: The target macOS system may be x86_64 or ARM64 (Apple Silicon), requiring different compilation settings.

## Potential Solutions

### 1. OSXCross

[OSXCross](https://github.com/tpoechtrager/osxcross) is a tool that allows cross-compilation for macOS on Linux. However, it requires the macOS SDK, which must be obtained from a macOS system due to licensing restrictions.

### 2. Docker for Mac with Volume Mounts

A simpler approach might be to use Docker for Mac with volume mounts to compile in a container while allowing the binary to be executed natively:

```bash
# Build in Docker with volume mount to macOS file system
docker run --rm -v $(pwd):/src -w /src rust:latest \
    cargo build --features simulator
    
# Then run the binary natively on macOS
./target/debug/panelkit
```

However, this approach requires Rust and SDL2 dependencies to be correctly set up on both the Docker container and the macOS host.

### 3. Use Cargo on macOS Directly

The simplest solution for development purposes would be to install Rust and required dependencies on the macOS system:

```bash
# Install Rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# Install SDL2 dependencies via Homebrew
brew install sdl2 sdl2_ttf sdl2_image

# Build and run
cargo build --features simulator
./target/debug/panelkit
```

## Recommended Approach for macOS Development

For macOS development with the current constraints:

1. Install Rust and SDL2 libraries directly on the macOS system.
2. Use the standard Cargo build system with the simulator feature.
3. For production/release, consider setting up a proper CI/CD pipeline with macOS runners.

## Future Work

- Investigate OSXCross setup with legally obtained macOS SDK
- Create a cross-platform build matrix for CI/CD
- Consider universal binary support (x86_64 + ARM64)