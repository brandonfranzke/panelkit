#!/bin/bash

# Check for clean argument
if [ "$1" == "clean" ]; then
  echo "Cleaning build directory..."
  rm -rf build
  echo "Clean complete."
  exit 0
fi

# Create build directory if it doesn't exist
mkdir -p build

# Go to build directory
cd build

# Run CMake
cmake ..

# Build
make

# Check if we just want to build without running
if [ "$1" == "build" ]; then
  echo "Build complete."
  exit 0
fi

# Run if build was successful
if [ -f ./panelkit ]; then
  echo "========== MULTI-PAGE APPLICATION =========="
  echo ""
  echo "This implementation features a multi-page application with API integration:"
  echo ""
  echo "Key features:"
  echo "  • Two pages: Text page and Buttons page"
  echo "  • Swipe horizontally to navigate between pages"
  echo "  • iPhone-style page indicator dots at the bottom"
  echo "  • Vertical scrolling for content within each page"
  echo "  • Live API data display on page 2"
  echo "  • Gesture detection for clicks, drags and swipes"
  echo ""
  echo "Controls:"
  echo "  • LEFT/RIGHT: Change pages"
  echo "  • UP/DOWN: Scroll content"
  echo "  • D: Toggle debug overlay"
  echo "  • ESC: Quit"
  echo ""
  echo "========== STARTING APPLICATION =========="
  
  ./panelkit
else
  echo "Build failed, executable not found."
fi