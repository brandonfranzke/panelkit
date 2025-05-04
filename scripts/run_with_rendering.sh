#!/bin/bash
# Run PanelKit with the new rendering abstraction

# Export the environment variable
export PANELKIT_USE_RENDERING=1

# Run the application with debug logging
if [ -f "build/panelkit-macos" ]; then
    DYLD_LIBRARY_PATH=/opt/homebrew/lib RUST_LOG=debug ./build/panelkit-macos
else
    echo "Error: build/panelkit-macos not found. Please run 'make host' first."
    exit 1
fi