#!/bin/bash
cd "$(dirname "$0")"
echo "Running PanelKit with logging to panelkit.log..."
# Force disable fullscreen
DYLD_LIBRARY_PATH=/opt/homebrew/lib RUST_LOG=info ./build/panelkit-macos --no-fullscreen > panelkit.log 2>&1 || echo "Application crashed, see panelkit.log for details"