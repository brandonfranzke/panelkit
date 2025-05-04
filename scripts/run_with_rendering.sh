#!/bin/bash
# Run PanelKit with the rendering abstraction

# Note: PANELKIT_USE_RENDERING environment variable is no longer needed
# as the rendering abstraction is now the only implementation

# Run the application with debug logging
if [ -f "build/panelkit-macos" ]; then
    DYLD_LIBRARY_PATH=/opt/homebrew/lib RUST_LOG=debug ./build/panelkit-macos
else
    echo "Error: build/panelkit-macos not found. Please run 'make host' first."
    exit 1
fi