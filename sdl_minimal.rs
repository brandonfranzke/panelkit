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
