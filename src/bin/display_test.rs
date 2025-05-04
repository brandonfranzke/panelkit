extern crate sdl2;

use sdl2::pixels::Color;
use sdl2::event::Event;
use sdl2::keyboard::Keycode;
use std::time::Duration;

fn main() -> Result<(), String> {
    println!("Starting SDL2 display test");
    
    // Initialize SDL2
    let sdl_context = sdl2::init()?;
    let video_subsystem = sdl_context.video()?;
    
    // Create window
    let window = video_subsystem
        .window("SDL2 Display Test", 800, 480)
        .position_centered()
        .build()
        .unwrap();
    
    // Create canvas
    let mut canvas = window.into_canvas().build().unwrap();
    
    // Draw bright red background
    canvas.set_draw_color(Color::RGB(255, 0, 0));
    canvas.clear();
    canvas.present();
    
    println!("Initial red screen should be visible");
    
    // Wait 2 seconds
    std::thread::sleep(Duration::from_secs(2));
    
    // Draw green background
    canvas.set_draw_color(Color::RGB(0, 255, 0));
    canvas.clear();
    canvas.present();
    
    println!("Green screen should now be visible");
    
    // Wait 2 seconds
    std::thread::sleep(Duration::from_secs(2));
    
    // Draw blue background
    canvas.set_draw_color(Color::RGB(0, 0, 255));
    canvas.clear();
    canvas.present();
    
    println!("Blue screen should now be visible");
    println!("Press Escape or close the window to exit");
    
    // Event loop - wait for quit
    let mut event_pump = sdl_context.event_pump().unwrap();
    'running: loop {
        for event in event_pump.poll_iter() {
            match event {
                Event::Quit {..} | 
                Event::KeyDown { keycode: Some(Keycode::Escape), .. } => {
                    break 'running
                },
                _ => {}
            }
        }
        
        // Sleep to keep CPU usage reasonable
        std::thread::sleep(Duration::from_millis(100));
    }
    
    println!("Display test complete");
    
    Ok(())
}