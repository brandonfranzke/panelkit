# PanelKit SDL

A touch-optimized UI application built with SDL2 featuring a multi-page interface with gesture recognition and API integration.

## Features

- Multi-page interface with smooth swipe transitions
- iPhone-style page indicators
- Vertical scrolling for content
- API integration with randomuser.me
- Gesture detection (tap, drag, swipe)
- Fixed content areas with scrolling buttons
- Responsive UI elements

## Pages

- **Page 1**: Text display with customizable color
- **Page 2**: Interactive buttons and API data display

## Building and Running

This project uses CMake for building. A convenient build script is provided:

```bash
# Build and run the application
./build.sh

# Build only, without running
./build.sh build

# Clean build files
./build.sh clean
```

## Dependencies

- SDL2
- SDL2_ttf
- libcurl
- pthread
- Standard C libraries

## Controls

- **Swipe horizontally**: Navigate between pages
- **Swipe vertically**: Scroll content
- **Tap**: Activate buttons
- **D key**: Toggle debug overlay
- **ESC key**: Exit application