# PanelKit Deployment Guide

## Overview

This directory contains files and tools for deploying PanelKit to target devices. The deployment process is designed to be simple: copy files to the target, then use the target's Makefile to setup and install.

## Quick Deployment

### From Development Machine

Use the automated deployment script with long options:

```bash
# Basic deployment
./scripts/deploy.sh --host panelkit

# With custom user
./scripts/deploy.sh --host 192.168.1.100 --user pi

# With custom target directory
./scripts/deploy.sh --host panelkit --user pi --target-dir /opt/panelkit

# Via Makefile
make deploy TARGET_HOST=panelkit TARGET_USER=pi
```

### Target Device Setup

After deployment, on the target device:

```bash
cd /tmp/panelkit  # or your chosen target directory

# Setup system permissions and directories
make setup

# Install binary and systemd service
make install

# Start the service
make start

# View logs
make logs

# Check service status
make status
```

## File Structure

After deployment, the target directory contains:

```
/tmp/panelkit/
├── panelkit              # ARM64 binary
├── panelkit.service      # Systemd service file
├── Makefile             # Target management commands
└── README.md            # This file
```

## Target Makefile Commands

The target Makefile provides these commands:

- **`make setup`** - Configure system permissions and create directories
- **`make install`** - Install binary and systemd service
- **`make start`** - Start the panelkit service
- **`make stop`** - Stop the panelkit service  
- **`make restart`** - Restart the panelkit service
- **`make status`** - Show service status
- **`make logs`** - Follow service logs (file-based)
- **`make clean`** - Remove service and files

## Service Configuration

The systemd service (`panelkit.service`) is configured for:

- **User**: root (for framebuffer access)
- **Auto-restart**: Service restarts on failure
- **Logging**: File-based logs to `/var/log/panelkit/panelkit.log`
- **Environment**: SDL2 framebuffer configuration
- **Dependencies**: Waits for graphical session target

## Manual Installation (Alternative)

If you prefer manual installation:

### 1. Setup Permissions
```bash
sudo usermod -a -G input,video $USER
sudo chmod a+rw /dev/dri/card0 /dev/fb0 || true
sudo mkdir -p /var/log/panelkit
```

### 2. Install Service
```bash
sudo cp panelkit.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable panelkit.service
```

### 3. Start Service
```bash
sudo systemctl start panelkit.service
```

## Troubleshooting

### Check Service Status
```bash
sudo systemctl status panelkit.service
```

### View Logs
```bash
# Follow logs in real-time
tail -f /var/log/panelkit/panelkit.log

# View recent logs
tail -n 100 /var/log/panelkit/panelkit.log

# Search logs
grep "error" /var/log/panelkit/panelkit.log
```

### Common Issues

**Framebuffer access issues:**
```bash
sudo usermod -a -G video $USER
sudo chmod 666 /dev/fb0
```

**Service won't start:**
- Check binary exists and is executable: `ls -la /tmp/panelkit/panelkit`
- Check service file syntax: `systemctl cat panelkit.service`
- Check logs: `tail /var/log/panelkit/panelkit.log`

**Permission denied:**
- Ensure user is in video group: `groups $USER`
- Check framebuffer permissions: `ls -la /dev/fb0`

## Remote Debugging

For debugging from development machine:

```bash
# Copy log file for analysis
scp target:/var/log/panelkit/panelkit.log .

# Monitor logs remotely
ssh target "tail -f /var/log/panelkit/panelkit.log"

# Check service status remotely
ssh target "systemctl status panelkit.service"
```

## Configuration

Default configuration in target Makefile:
- **APP**: panelkit
- **TARGET_DIR**: /tmp/panelkit  
- **LOG_DIR**: /var/log/panelkit
- **SERVICE_DIR**: /etc/systemd/system

These can be customized by editing the target Makefile if needed.