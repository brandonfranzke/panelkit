#!/bin/bash
cd "$(dirname "$0")"
echo "Building SDL2 test program..."
RUSTFLAGS="-C link-args=-Wl,-rpath,/opt/homebrew/lib -L/opt/homebrew/lib" LIBRARY_PATH="/opt/homebrew/lib" cargo build --bin sdl_test
echo "Running SDL2 test program..."
DYLD_LIBRARY_PATH=/opt/homebrew/lib target/debug/sdl_test