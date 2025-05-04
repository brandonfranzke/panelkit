extern crate sdl2;

use sdl2::pixels::Color;
use sdl2::rect::Rect;
use sdl2::event::Event;
use sdl2::keyboard::Keycode;
use std::time::Duration;

fn main() -> Result<(), String> {
    println!("Starting direct test with Hello page");
    
    // Initialize SDL2
    let sdl_context = sdl2::init()?;
    let video_subsystem = sdl_context.video()?;
    
    println!("Creating window...");
    let window = video_subsystem
        .window("Direct Hello Page Test", 800, 480)
        .position_centered()
        .build()
        .map_err(|e| e.to_string())?;
        
    println!("Creating canvas...");
    let mut canvas = window
        .into_canvas()
        .accelerated()
        .build()
        .map_err(|e| e.to_string())?;
    
    // Force red background on start
    println!("Drawing initial red screen...");
    canvas.set_draw_color(Color::RGB(255, 0, 0));
    canvas.clear();
    canvas.present();
    
    // Wait for 1 second to ensure it's visible
    std::thread::sleep(Duration::from_secs(1));
    
    // Draw a complex Hello screen
    println!("Drawing Hello page...");
    draw_hello_page(&mut canvas)?;
    canvas.present();
    
    println!("Hello page should now be visible");
    println!("Press Escape or close the window to exit");
    
    // Event loop
    println!("Entering event loop...");
    let mut event_pump = sdl_context.event_pump()?;
    'running: loop {
        for event in event_pump.poll_iter() {
            match event {
                Event::Quit {..} | Event::KeyDown { keycode: Some(Keycode::Escape), .. } => {
                    println!("Quit event received");
                    break 'running;
                },
                _ => {}
            }
        }
        
        std::thread::sleep(Duration::from_millis(100));
    }
    
    println!("Test complete");
    Ok(())
}

fn draw_hello_page(canvas: &mut sdl2::render::Canvas<sdl2::video::Window>) -> Result<(), String> {
    // Green background
    canvas.set_draw_color(Color::RGB(50, 200, 100)); // Bright green
    canvas.clear();
    
    // Title bar
    canvas.set_draw_color(Color::RGB(30, 120, 60)); // Darker green
    canvas.fill_rect(Rect::new(0, 0, 800, 70))?;
    
    // Text box
    canvas.set_draw_color(Color::RGB(255, 255, 255)); // White
    canvas.fill_rect(Rect::new(250, 100, 300, 60))?;
    
    // Draw "HELLO" text with black rectangles
    canvas.set_draw_color(Color::RGB(0, 0, 0)); // Black
    
    // Letter H
    canvas.fill_rect(Rect::new(280, 115, 10, 30))?;
    canvas.fill_rect(Rect::new(310, 115, 10, 30))?;
    canvas.fill_rect(Rect::new(290, 130, 20, 5))?;
    
    // Letter E
    canvas.fill_rect(Rect::new(330, 115, 10, 30))?;
    canvas.fill_rect(Rect::new(340, 115, 20, 5))?;
    canvas.fill_rect(Rect::new(340, 130, 15, 5))?;
    canvas.fill_rect(Rect::new(340, 140, 20, 5))?;
    
    // Letter L
    canvas.fill_rect(Rect::new(370, 115, 10, 30))?;
    canvas.fill_rect(Rect::new(380, 140, 20, 5))?;
    
    // Letter L
    canvas.fill_rect(Rect::new(410, 115, 10, 30))?;
    canvas.fill_rect(Rect::new(420, 140, 20, 5))?;
    
    // Letter O
    canvas.fill_rect(Rect::new(450, 115, 10, 30))?;
    canvas.fill_rect(Rect::new(480, 115, 10, 30))?;
    canvas.fill_rect(Rect::new(460, 115, 20, 5))?;
    canvas.fill_rect(Rect::new(460, 140, 20, 5))?;
    
    Ok(())
}