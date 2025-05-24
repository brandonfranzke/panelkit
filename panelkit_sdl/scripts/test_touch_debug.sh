#!/bin/bash
# Debug script to test touch input with detailed logging

echo "=== Touch Input Debug Script ==="
echo

# Check if we're on the target device
if [ ! -d "/dev/input" ]; then
    echo "Error: /dev/input not found. This script must run on the target device."
    exit 1
fi

# List all input devices
echo "Available input devices:"
for dev in /dev/input/event*; do
    if [ -r "$dev" ]; then
        name=$(cat /sys/class/input/$(basename $dev)/device/name 2>/dev/null || echo "Unknown")
        echo "  $dev: $name"
    fi
done
echo

# Check for touch devices specifically
echo "Checking for touch devices:"
for dev in /dev/input/event*; do
    if [ -r "$dev" ]; then
        # Use evtest to check capabilities (if available)
        if command -v evtest >/dev/null 2>&1; then
            caps=$(timeout 0.1s evtest --query $dev EV_ABS ABS_MT_POSITION_X 2>/dev/null)
            if [ $? -eq 0 ]; then
                name=$(cat /sys/class/input/$(basename $dev)/device/name 2>/dev/null || echo "Unknown")
                echo "  Found touch device: $dev ($name)"
            fi
        fi
    fi
done
echo

# Set environment for debug logging
export PANELKIT_LOG_LEVEL=DEBUG
export SDL_VIDEODRIVER=offscreen

# Run panelkit with debug logging
echo "Starting PanelKit with debug logging..."
echo "Touch the screen to test input. Press Ctrl+C to stop."
echo "----------------------------------------"

# Run with SDL+DRM backend to force evdev input
cd /tmp/panelkit
./panelkit --display-backend sdl_drm 2>&1 | grep -E "(Input|Touch|evdev|finger)"