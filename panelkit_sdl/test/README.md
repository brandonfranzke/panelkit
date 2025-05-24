# PanelKit Test Suite

This directory contains test utilities organized by category to help debug and verify different aspects of the PanelKit system.

## Directory Structure

```
test/
├── core/           # Core functionality tests (logging, etc.)
├── input/          # Input system tests (touch, gestures)
├── display/        # Display system tests (backends, rendering)
├── integration/    # End-to-end integration tests
└── Makefile        # Build system for all tests
```

## Available Tests

### Core Tests
- **`core/test_logger`** - Test the zlog-based logging system

### Input Tests  
- **`input/test_touch_raw`** - Test raw touch input using Linux input events (target device only)

### Display Tests
- *(Coming soon)*

### Integration Tests
- *(Coming soon)*

## Building Tests

```bash
# Show available targets
make help

# Build all tests
make all

# Build tests by category
make core
make input
make display
make integration

# Build individual tests
make core/test_logger
make input/test_touch_raw
```

## Running Tests

### Core Tests
```bash
# Test logging system
./core/test_logger ../config/zlog.conf
```

### Input Tests
```bash
# Test raw touch input (auto-detect touch device)
sudo ./input/test_touch_raw

# Test specific input device
sudo ./input/test_touch_raw /dev/input/event0

# List available input devices
ls -la /dev/input/event*
```

## Touch Input Debugging

The `test_touch_raw` utility is particularly useful for debugging touch issues:

1. **Driver Issues**: If no events appear, the touch driver may not be loaded
2. **Device Path Issues**: Try different `/dev/input/eventX` devices
3. **Permission Issues**: Touch devices usually require root access
4. **Protocol Issues**: Events will show if the device uses standard Linux input protocol

### Expected Touch Events

For a working touch screen, you should see:
- `ABS_X` / `ABS_Y` - Single-touch coordinates
- `ABS_MT_POSITION_X` / `ABS_MT_POSITION_Y` - Multi-touch coordinates  
- `BTN_TOUCH` - Touch press/release
- `ABS_MT_TRACKING_ID` - Touch tracking (finger identification)
- `SYN_REPORT` - End of event packet

### Troubleshooting

1. **No touch device found**:
   ```bash
   # Check for input devices
   ls -la /dev/input/
   
   # Check kernel messages for touch driver
   dmesg | grep -i touch
   ```

2. **Permission denied**:
   ```bash
   # Run as root
   sudo ./input/test_touch_raw
   
   # Or add user to input group
   sudo usermod -a -G input $USER
   ```

3. **No events appearing**:
   - Verify the device path is correct
   - Check if another process is using the touch device
   - Verify touch screen is properly connected and configured

## Adding New Tests

1. Create test file in appropriate category directory
2. Add build rule to Makefile
3. Update this README with usage instructions
4. Follow existing naming convention: `test_<functionality>`