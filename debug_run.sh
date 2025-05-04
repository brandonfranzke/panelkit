#!/bin/bash
cd "$(dirname "$0")"
echo "Running minimal SDL test with logging..."

# Create a new temporary directory for our test
mkdir -p /tmp/sdl_test
cd /tmp/sdl_test

# Create a new cargo project
cargo init --bin

# Add SDL2 dependency to Cargo.toml
cat > Cargo.toml << 'EOF'
[package]
name = "sdl_test"
version = "0.1.0"
edition = "2021"

[dependencies]
sdl2 = "0.35"
EOF

# Create a simple SDL test
cat > src/main.rs << 'EOF'
extern crate sdl2;

use sdl2::pixels::Color;
use sdl2::event::Event;
use sdl2::keyboard::Keycode;
use std::time::Duration;

pub fn main() {
    println!("Starting SDL2 minimal test");
    
    let sdl_context = sdl2::init().unwrap();
    println!("SDL2 initialized");
    
    let video_subsystem = sdl_context.video().unwrap();
    println!("Video subsystem initialized");

    let window = video_subsystem
        .window("SDL2 Minimal Test", 800, 600)
        .position_centered()
        .build()
        .unwrap();
    println!("Window created");

    let mut canvas = window.into_canvas().build().unwrap();
    println!("Canvas created");

    // Set red background
    canvas.set_draw_color(Color::RGB(255, 0, 0));
    canvas.clear();
    canvas.present();
    println!("Red background set");
    
    // Just wait for 5 seconds and then exit
    std::thread::sleep(Duration::from_secs(5));
    println!("Test completed");
}
EOF

# Build and run
echo "Building SDL minimal test..."
RUSTFLAGS="-C link-args=-Wl,-rpath,/opt/homebrew/lib -L/opt/homebrew/lib" LIBRARY_PATH="/opt/homebrew/lib" cargo build

# Run the test
echo "Running SDL minimal test..."
DYLD_LIBRARY_PATH=/opt/homebrew/lib ./target/debug/sdl_test