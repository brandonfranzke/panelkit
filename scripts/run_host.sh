#!/bin/bash
# Helper script to run PanelKit on macOS with proper settings

# Set environment variables
export DYLD_LIBRARY_PATH=/opt/homebrew/lib
export RUST_LOG=debug
export RUST_BACKTRACE=1
export SDL_RENDER_DRIVER=software

# Parse arguments
WIDTH=800
HEIGHT=480
LOG_LEVEL=debug

while [[ $# -gt 0 ]]; do
  case $1 in
    --width)
      WIDTH="$2"
      shift 2
      ;;
    --height)
      HEIGHT="$2"
      shift 2
      ;;
    --log-level)
      LOG_LEVEL="$2"
      shift 2
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done

# Build if needed
if [ ! -f ./build/panelkit-macos ]; then
  echo "Building panelkit for macOS..."
  make host
fi

echo "Running panelkit with dimensions ${WIDTH}x${HEIGHT}, log level: ${LOG_LEVEL}"
echo "==== Starting PanelKit ===="
RUST_BACKTRACE=1 RUST_LOG=${LOG_LEVEL} ./build/panelkit-macos -d "${WIDTH}x${HEIGHT}"
EXIT_CODE=$?
if [ $EXIT_CODE -ne 0 ]; then
  echo "==== PanelKit exited with error code $EXIT_CODE ===="
  echo "Check logs above for error details"
else
  echo "==== PanelKit exited normally ===="
fi