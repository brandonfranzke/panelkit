# PanelKit Build Guide

This document explains how to build, run, and deploy the PanelKit application.

## Prerequisites

- Docker (for containerized builds)
- Make
- SSH client (for deployment)

## Build Environments

PanelKit uses Docker containers for all builds to ensure consistency and avoid local toolchain dependencies:

1. **Native Build Container (`Dockerfile.native`)**
   - For building and running the simulator on Linux
   - Contains Rust, SDL2, and other required libraries
   - Used for local development and testing

2. **Cross-Compilation Container (`Dockerfile.cross`)**
   - For cross-compiling to ARM targets (Raspberry Pi)
   - Contains ARM toolchain and required libraries
   - Used for building binaries for target deployment

## Common Build Commands

### Setting Up

Before building for the first time, create the Docker containers:

```bash
make build-containers
```

This creates the Docker images needed for building the application.

### Local Development Build

To build for local development and testing:

```bash
make local
```

This builds the application with simulator features enabled inside the Docker container.

### Running the Application

You have two primary options for running the application:

#### Headless Mode

The headless mode allows you to build and run the application:

```bash
make build-headless  # Build the headless version
make run-headless    # Run the headless version
```

This mode:
- Builds entirely in Docker containers
- Uses a console-based display driver
- Simulates touch events automatically 
- All UI operations are logged to the console

This is the recommended approach for development.

#### Target Mode (For Actual Deployment)

For deploying to the target hardware:

```bash
make target         # Cross-compile for ARM
make deploy         # Deploy to default target device
```

This builds and deploys the full application to the Raspberry Pi target device.

### Cross-Compiling for Target

To build for the Raspberry Pi target:

```bash
make target
```

This cross-compiles the application for ARM architecture and places the binary in the `build/` directory.

## Deployment

### Deploying to Default Target

To deploy to the default target device (defined by TARGET_HOST, TARGET_USER, and TARGET_DIR variables):

```bash
make deploy
```

This builds and copies the binary to the target device using SSH.

### Deploying to Custom Target

For more flexibility in deployment, use the transfer script:

```bash
# Build for target first
make target

# Transfer to a specific device
./scripts/transfer.sh --host=192.168.1.100 --user=pi --dir=/home/pi/apps
```

Options:
- `--host` - Hostname or IP address of the target device
- `--user` - Username on the target device
- `--dir` - Directory on the target device to install to

### Creating Service for Auto-Start

To create a systemd service for auto-starting on boot:

```bash
make create-service
make install-service
```

This creates a systemd service file and installs it on the target device.

## Feature Flags

PanelKit uses feature flags to control compilation:

- `simulator` - Enables SDL2-based simulation for desktop development
- `target` - Enables optimizations and features for target hardware

## Build Directory Structure

The build system creates the following structure:

```
build/
├── panelkit       # Native Linux binary (for simulation)
├── panelkit-arm   # ARM binary (for Raspberry Pi)
└── panelkit.service # Generated systemd service file
```

## Troubleshooting

### Headless Mode Issues

If you have problems with the headless mode:

1. Check for RUST_LOG environment variable:
   ```bash
   export RUST_LOG=debug
   ```
   This ensures you see all debug output from the application.

2. Make sure you're using the correct build:
   ```bash
   make build-headless  # Creates the headless build
   ```

3. Verify the binary has execute permissions:
   ```bash
   chmod +x build/panelkit-headless
   ```

4. Check for these specific errors:
   - `error: no matches for kind 'bin'`: This means the build failed in Docker
   - `permission denied`: The binary doesn't have execute permissions
   - Silent failure: Try running with RUST_LOG=trace for more verbose output

### Cross-Compilation Issues

If cross-compilation fails:

1. Ensure the Docker containers were built correctly
2. Check that the ARM toolchain is properly installed in the Docker container
3. Verify that any libraries used have ARM versions available

### Deployment Issues

If deployment fails:

1. Ensure SSH access to the target device works
2. Check that the user has write permissions to the target directory
3. Verify that the ARM binary was built successfully
4. Check the SSH keys and authentication method

## Build Configuration

You can customize the build by setting environment variables:

- `TARGET_HOST` - Hostname or IP address of target device (default: raspberrypi.local)
- `TARGET_USER` - Username on target device (default: pi)
- `TARGET_DIR` - Directory on target device (default: /home/$TARGET_USER/panelkit)

Example:

```bash
TARGET_HOST=mypi.local TARGET_USER=admin make deploy
```

## Architecture-Specific Notes

### Development Machine (x86_64)

- Builds occur in Docker containers
- UI runs through X11 forwarding
- Uses SDL2 for display and input

### Target Device (ARM)

- Binary is cross-compiled for ARM architecture
- Requires framebuffer driver on target
- Requires touch input driver on target
- Autostart through systemd