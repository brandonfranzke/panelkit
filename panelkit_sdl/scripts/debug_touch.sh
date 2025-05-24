#!/bin/bash
# Comprehensive touch debugging script for PanelKit

echo "=== PanelKit Touch Debug Helper ==="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check if we're on the target device
if [ ! -d "/dev/input" ]; then
    echo -e "${RED}Error: /dev/input not found. This script must run on the target device.${NC}"
    exit 1
fi

# Function to check permissions
check_permissions() {
    echo -e "${BLUE}=== Checking Permissions ===${NC}"
    
    # Check user groups
    echo -n "User '$USER' is in groups: "
    groups $USER
    
    # Check input group
    if groups $USER | grep -q input; then
        echo -e "${GREEN}✓ User is in 'input' group${NC}"
    else
        echo -e "${RED}✗ User is NOT in 'input' group${NC}"
        echo "  Run: sudo usermod -a -G input $USER"
        echo "  Then logout and login again"
    fi
    
    # Check video group
    if groups $USER | grep -q video; then
        echo -e "${GREEN}✓ User is in 'video' group${NC}"
    else
        echo -e "${YELLOW}⚠ User is NOT in 'video' group${NC}"
        echo "  Run: sudo usermod -a -G video $USER"
    fi
    
    # Check device permissions
    echo
    echo "Input device permissions:"
    for dev in /dev/input/event*; do
        if [ -e "$dev" ]; then
            perms=$(ls -l $dev | awk '{print $1, $3, $4}')
            if [ -r "$dev" ]; then
                echo -e "${GREEN}✓ $dev: $perms${NC}"
            else
                echo -e "${RED}✗ $dev: $perms (not readable)${NC}"
            fi
        fi
    done
    echo
}

# Function to find touch devices
find_touch_devices() {
    echo -e "${BLUE}=== Scanning for Touch Devices ===${NC}"
    
    touch_found=false
    
    for dev in /dev/input/event*; do
        if [ -r "$dev" ]; then
            # Get device name
            name=$(cat /sys/class/input/$(basename $dev)/device/name 2>/dev/null || echo "Unknown")
            
            # Check if it has touch capabilities using evtest
            if command -v evtest >/dev/null 2>&1; then
                # Check for ABS_MT_POSITION_X (multi-touch)
                if timeout 0.1s evtest --query $dev EV_ABS ABS_MT_POSITION_X >/dev/null 2>&1; then
                    echo -e "${GREEN}✓ Multi-touch device found: $dev - $name${NC}"
                    touch_found=true
                    TOUCH_DEVICE=$dev
                # Check for ABS_X (single touch)
                elif timeout 0.1s evtest --query $dev EV_ABS ABS_X >/dev/null 2>&1 && \
                     timeout 0.1s evtest --query $dev EV_ABS ABS_Y >/dev/null 2>&1; then
                    echo -e "${YELLOW}⚠ Single-touch device found: $dev - $name${NC}"
                    touch_found=true
                    TOUCH_DEVICE=$dev
                fi
            else
                # Fallback: check by name
                if echo "$name" | grep -iE "(touch|goodix|ft5x06|edt-ft5x06)" >/dev/null; then
                    echo -e "${YELLOW}⚠ Possible touch device (by name): $dev - $name${NC}"
                    touch_found=true
                    TOUCH_DEVICE=$dev
                fi
            fi
        fi
    done
    
    if ! $touch_found; then
        echo -e "${RED}✗ No touch devices found${NC}"
    fi
    echo
}

# Function to test raw touch input
test_raw_touch() {
    if [ -z "$TOUCH_DEVICE" ]; then
        echo -e "${RED}No touch device to test${NC}"
        return
    fi
    
    echo -e "${BLUE}=== Testing Raw Touch Input ===${NC}"
    echo "Testing device: $TOUCH_DEVICE"
    echo "Touch the screen for 5 seconds..."
    echo
    
    # Use evtest if available
    if command -v evtest >/dev/null 2>&1; then
        timeout 5s evtest $TOUCH_DEVICE 2>&1 | grep -E "(ABS_MT_|ABS_X|ABS_Y|BTN_TOUCH)" | head -20
    else
        # Fallback to hexdump
        echo "Reading raw events (hex dump):"
        timeout 5s hexdump -C $TOUCH_DEVICE | head -20
    fi
    echo
}

# Function to run PanelKit with debug logging
run_panelkit_debug() {
    echo -e "${BLUE}=== Running PanelKit with Debug Logging ===${NC}"
    
    # Check if panelkit exists
    if [ ! -f "/tmp/panelkit/panelkit" ]; then
        echo -e "${RED}PanelKit not found at /tmp/panelkit/panelkit${NC}"
        echo "Deploy it first with: make deploy"
        return
    fi
    
    cd /tmp/panelkit
    
    # Set debug environment
    export PANELKIT_LOG_LEVEL=DEBUG
    export SDL_VIDEODRIVER=offscreen  # Force offscreen to test evdev
    
    echo "Starting PanelKit with SDL+DRM backend..."
    echo "Touch the screen to test. Press Ctrl+C to stop."
    echo -e "${YELLOW}Filtering for input-related messages:${NC}"
    echo
    
    # Run with filtered output
    ./panelkit --display-backend sdl_drm 2>&1 | grep -E "(Input|Touch|evdev|finger|Build|SDL|Device|capabilities)" --color=always
}

# Function to show summary
show_summary() {
    echo
    echo -e "${BLUE}=== Summary ===${NC}"
    
    if [ -n "$TOUCH_DEVICE" ]; then
        echo -e "Touch device: ${GREEN}$TOUCH_DEVICE${NC}"
        
        # Get device info
        name=$(cat /sys/class/input/$(basename $TOUCH_DEVICE)/device/name 2>/dev/null || echo "Unknown")
        echo "Device name: $name"
        
        # Check if device is accessible
        if [ -r "$TOUCH_DEVICE" ]; then
            echo -e "Device access: ${GREEN}✓ Readable${NC}"
        else
            echo -e "Device access: ${RED}✗ Not readable${NC}"
        fi
    else
        echo -e "Touch device: ${RED}Not found${NC}"
    fi
    
    echo
    echo -e "${BLUE}Debug Tips:${NC}"
    echo "1. If touch device not found:"
    echo "   - Check dmesg for touch driver messages: dmesg | grep -i touch"
    echo "   - List all input devices: cat /proc/bus/input/devices"
    echo
    echo "2. If permissions denied:"
    echo "   - Add user to input group: sudo usermod -a -G input $USER"
    echo "   - Logout and login again"
    echo
    echo "3. If touch events not working in PanelKit:"
    echo "   - Check the filtered log output above"
    echo "   - Look for 'evdev' initialization messages"
    echo "   - Check for 'Touch DOWN/UP' messages when touching"
}

# Main menu
while true; do
    echo -e "${BLUE}=== Menu ===${NC}"
    echo "1) Check permissions"
    echo "2) Find touch devices"  
    echo "3) Test raw touch input"
    echo "4) Run PanelKit with debug"
    echo "5) Show summary"
    echo "6) Run all tests"
    echo "q) Quit"
    echo
    read -p "Choose option: " choice
    
    case $choice in
        1) check_permissions ;;
        2) find_touch_devices ;;
        3) test_raw_touch ;;
        4) run_panelkit_debug ;;
        5) show_summary ;;
        6) 
            check_permissions
            find_touch_devices
            test_raw_touch
            show_summary
            ;;
        q|Q) exit 0 ;;
        *) echo -e "${RED}Invalid option${NC}" ;;
    esac
    
    echo
    read -p "Press Enter to continue..."
    clear
done