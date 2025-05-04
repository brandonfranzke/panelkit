#!/bin/bash
cd "$(dirname "$0")"
echo "Running PanelKit directly..."
DYLD_LIBRARY_PATH=/opt/homebrew/lib RUST_LOG=info ./build/panelkit-macos