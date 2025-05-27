# Building and Deployment

## Overview

PanelKit supports multiple build configurations:
- **Host development**: Native builds for macOS/Linux development
- **Target cross-compilation**: ARM64 builds for embedded devices
- **Docker-based builds**: Reproducible cross-compilation environment

## Build System

### Prerequisites

#### Host Development
- SDL2 and SDL2_ttf
- libcurl
- libyaml
- zlog (optional)
- CMake 3.10+

#### Target Cross-Compilation
- Docker (for build environment)
- ARM64 toolchain (included in Docker image)

### Build Commands

```bash
# Development build (native)
make host

# Run development build
make run

# Cross-compile for ARM64 target
make target

# Deploy to target device
make deploy TARGET_HOST=panelkit

# Clean build artifacts
make clean
```

### Build Options

```bash
# Custom display configuration
make run DISPLAY_WIDTH=800 DISPLAY_HEIGHT=600
make run PORTRAIT=1  # 480x640 portrait mode

# Force specific backend
make run DISPLAY_BACKEND=sdl

# Deploy with custom user/directory
make deploy TARGET_HOST=192.168.1.100 TARGET_USER=pi
```

## Cross-Compilation

### Docker Build Environment

The project includes a Dockerfile for consistent cross-compilation:

```dockerfile
FROM debian:bullseye
# Installs ARM64 toolchain
# Configures cross-compilation environment
# Builds static libraries for target
```

### Build Process

1. **Prepare Environment**: Docker container with toolchain
2. **Configure CMake**: Target-specific settings
3. **Build Dependencies**: Static linking for portability
4. **Compile Application**: Optimized for ARM64
5. **Package Binary**: Single executable output

### Target Configuration

```cmake
# CMake toolchain configuration
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
```

## Deployment Process

### Quick Deploy

```bash
# From development machine
make deploy TARGET_HOST=panelkit TARGET_USER=root

# This will:
# 1. Cross-compile for ARM64
# 2. Copy files to target:/tmp/panelkit
# 3. Show next steps for target setup
```

### Target Setup

On the target device:

```bash
cd /tmp/panelkit

# One-time system setup
make setup  # Configure permissions, create directories

# Install and start
make install  # Install binary and systemd service
make start    # Start the service
```

### File Locations

After installation:
- Binary: `/usr/local/bin/panelkit`
- Service: `/etc/systemd/system/panelkit.service`
- Config: `/etc/panelkit/config.yaml`
- Logs: `/var/log/panelkit/`

## Systemd Service

### Service Configuration

```ini
[Unit]
Description=PanelKit Touch UI
After=graphical.target

[Service]
Type=simple
ExecStart=/usr/local/bin/panelkit
Restart=always
RestartSec=3
User=root

# Logging
StandardOutput=append:/var/log/panelkit/panelkit.log
StandardError=append:/var/log/panelkit/panelkit.log

# Environment
Environment="SDL_VIDEODRIVER=kmsdrm"
Environment="SDL_VIDEO_KMSDRM_LEGACY_SUPPORT=0"

[Install]
WantedBy=graphical.target
```

### Service Management

```bash
# Control service
sudo systemctl start panelkit
sudo systemctl stop panelkit
sudo systemctl restart panelkit

# View status
sudo systemctl status panelkit

# Enable auto-start
sudo systemctl enable panelkit

# View logs
sudo journalctl -u panelkit -f
tail -f /var/log/panelkit/panelkit.log
```

## Configuration Deployment

### Configuration Files

1. **System config**: `/etc/panelkit/config.yaml`
2. **User override**: `~/.config/panelkit/config.yaml`
3. **Local override**: `./config.yaml`

### Deploy Custom Config

```bash
# Copy config to target
scp custom_config.yaml target:/etc/panelkit/config.yaml

# Restart service to apply
ssh target "sudo systemctl restart panelkit"
```

## Troubleshooting Deployment

### Common Issues

#### Permission Denied
```bash
# Add user to required groups
sudo usermod -a -G input,video $USER

# Fix framebuffer permissions
sudo chmod 666 /dev/fb0
```

#### Display Not Found
```bash
# Check available displays
ls /dev/dri/card*
ls /dev/fb*

# Set environment variables
export SDL_VIDEODRIVER=kmsdrm
export SDL_VIDEO_KMSDRM_LEGACY_SUPPORT=1
```

#### Service Won't Start
```bash
# Check binary
file /usr/local/bin/panelkit
ldd /usr/local/bin/panelkit

# Check logs
tail -100 /var/log/panelkit/panelkit.log

# Run manually for debugging
sudo /usr/local/bin/panelkit -v
```

### Debug Deployment

```bash
# Enable verbose logging
/usr/local/bin/panelkit -v -c /etc/panelkit/config.yaml

# Check dependencies
ldd /usr/local/bin/panelkit

# Monitor system resources
top -p $(pgrep panelkit)
```

## Production Considerations

### Security
- Run as non-root user when possible
- Restrict config file permissions
- Use systemd security features (PrivateTmp, ProtectSystem)

### Performance
- Enable compiler optimizations (`-O2`)
- Use hardware acceleration when available
- Monitor memory usage

### Reliability
- Configure systemd restart policy
- Implement log rotation
- Monitor service health
- Set resource limits

### Updates
```bash
# Deploy update
make deploy TARGET_HOST=panelkit

# On target
cd /tmp/panelkit
make stop
make install
make start
```