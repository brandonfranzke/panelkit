// A minimal UI test that uses a simpler approach
// We skip most of the abstraction to directly test the core functionality

use anyhow::Result;
use sdl2::pixels::Color;
use sdl2::rect::Rect;
use sdl2::event::Event;
use sdl2::keyboard::Keycode;
use std::time::Duration;

enum PageType {
    Hello,
    World,
}

fn main() -> Result<()> {
    println!("Starting minimal UI test");
    
    // Initialize SDL
    let sdl_context = sdl2::init().map_err(|e| anyhow::anyhow!("SDL init error: {}", e))?;
    let video = sdl_context.video().map_err(|e| anyhow::anyhow!("SDL video error: {}", e))?;
    
    // Create window
    let window = video.window("PanelKit - Minimal UI", 800, 480)
        .position_centered()
        .build()
        .map_err(|e| anyhow::anyhow!("Window creation error: {}", e))?;
    
    // Create canvas
    let mut canvas = window.into_canvas()
        .accelerated()
        .build()
        .map_err(|e| anyhow::anyhow!("Canvas creation error: {}", e))?;
    
    // Set initial state
    let mut current_page = PageType::Hello;
    let mut hello_counter = 0;
    let mut world_counter = 0;
    
    // Show initialization marker
    canvas.set_draw_color(Color::RGB(100, 100, 100));
    canvas.clear();
    canvas.set_draw_color(Color::RGB(255, 0, 0));
    canvas.fill_rect(Rect::new(0, 0, 50, 50))
        .map_err(|e| anyhow::anyhow!("Drawing error: {}", e))?;
    canvas.present();
    
    // Short pause to demonstrate initialization is working
    std::thread::sleep(Duration::from_millis(500));
    
    // Main loop
    let mut event_pump = sdl_context.event_pump()
        .map_err(|e| anyhow::anyhow!("Event pump error: {}", e))?;
    
    'running: loop {
        // Handle events
        for event in event_pump.poll_iter() {
            match event {
                Event::Quit {..} | Event::KeyDown { keycode: Some(Keycode::Escape), .. } => {
                    println!("Quit event received");
                    break 'running;
                },
                Event::MouseButtonDown { x, y, .. } => {
                    // Handle page-specific clicks
                    match current_page {
                        PageType::Hello => {
                            // Counter click
                            if x >= 350 && x <= 450 && y >= 200 && y <= 250 {
                                hello_counter += 1;
                                println!("Hello counter: {}", hello_counter);
                            }
                            
                            // Navigation
                            if x >= 670 && x <= 700 && y >= 210 && y <= 270 {
                                current_page = PageType::World;
                                println!("Navigating to World page");
                            }
                        },
                        PageType::World => {
                            // Counter click
                            if x >= 350 && x <= 450 && y >= 200 && y <= 250 {
                                world_counter += 1;
                                println!("World counter: {}", world_counter);
                            }
                            
                            // Navigation
                            if x >= 100 && x <= 130 && y >= 210 && y <= 270 {
                                current_page = PageType::Hello;
                                println!("Navigating to Hello page");
                            }
                        }
                    }
                },
                _ => {}
            }
        }
        
        // Render current page
        match current_page {
            PageType::Hello => render_hello_page(&mut canvas, hello_counter)?,
            PageType::World => render_world_page(&mut canvas, world_counter)?,
        }
        
        // Present
        canvas.present();
        
        // Sleep to maintain reasonable framerate
        std::thread::sleep(Duration::from_millis(16));
    }
    
    println!("Test completed");
    Ok(())
}

fn render_hello_page(canvas: &mut sdl2::render::Canvas<sdl2::video::Window>, counter: u32) -> Result<()> {
    // Clear with green background
    canvas.set_draw_color(Color::RGB(50, 200, 100));
    canvas.clear();
    
    // Draw title bar
    canvas.set_draw_color(Color::RGB(30, 120, 60));
    canvas.fill_rect(Rect::new(0, 0, 800, 70))
        .map_err(|e| anyhow::anyhow!("Drawing error: {}", e))?;
    
    // Draw "Hello" text box
    canvas.set_draw_color(Color::RGB(255, 255, 255));
    canvas.fill_rect(Rect::new(250, 100, 300, 60))
        .map_err(|e| anyhow::anyhow!("Drawing error: {}", e))?;
    
    // Draw basic "Hello" text using rectangles
    canvas.set_draw_color(Color::RGB(0, 0, 0));
    // Letter H
    canvas.fill_rect(Rect::new(280, 115, 10, 30)).ok();
    canvas.fill_rect(Rect::new(290, 125, 20, 10)).ok();
    canvas.fill_rect(Rect::new(310, 115, 10, 30)).ok();
    
    // Others letters...
    
    // Draw counter
    canvas.set_draw_color(Color::RGB(255, 255, 255));
    canvas.fill_rect(Rect::new(350, 200, 100, 50))
        .map_err(|e| anyhow::anyhow!("Drawing error: {}", e))?;
    
    // Show counter value
    canvas.set_draw_color(Color::RGB(0, 0, 0));
    for i in 0..std::cmp::min(counter, 5) {
        let x_pos = 360 + (i as i32 * 15);
        canvas.fill_rect(Rect::new(x_pos, 210, 10, 30)).ok();
    }
    
    // Draw right arrow
    canvas.set_draw_color(Color::RGB(200, 200, 200));
    let points = [
        sdl2::rect::Point::new(700, 240),
        sdl2::rect::Point::new(670, 210),
        sdl2::rect::Point::new(670, 270),
    ];
    
    for i in 0..points.len() {
        let j = (i + 1) % points.len();
        canvas.draw_line(points[i], points[j])
            .map_err(|e| anyhow::anyhow!("Drawing error: {}", e))?;
    }
    
    Ok(())
}

fn render_world_page(canvas: &mut sdl2::render::Canvas<sdl2::video::Window>, counter: u32) -> Result<()> {
    // Clear with blue background
    canvas.set_draw_color(Color::RGB(100, 149, 237));
    canvas.clear();
    
    // Draw title bar
    canvas.set_draw_color(Color::RGB(50, 50, 150));
    canvas.fill_rect(Rect::new(0, 0, 800, 70))
        .map_err(|e| anyhow::anyhow!("Drawing error: {}", e))?;
    
    // Draw "World" text box
    canvas.set_draw_color(Color::RGB(255, 255, 255));
    canvas.fill_rect(Rect::new(300, 150, 200, 40))
        .map_err(|e| anyhow::anyhow!("Drawing error: {}", e))?;
    
    // Draw simple "World" text
    canvas.set_draw_color(Color::RGB(0, 0, 0));
    // Just draw some rectangles to represent text
    canvas.fill_rect(Rect::new(320, 160, 8, 25)).ok(); // W
    canvas.fill_rect(Rect::new(350, 160, 8, 25)).ok(); // O
    // Other letters...
    
    // Draw counter
    canvas.set_draw_color(Color::RGB(255, 255, 255));
    canvas.fill_rect(Rect::new(350, 200, 100, 50))
        .map_err(|e| anyhow::anyhow!("Drawing error: {}", e))?;
    
    // Show counter value
    canvas.set_draw_color(Color::RGB(0, 0, 0));
    for i in 0..std::cmp::min(counter, 5) {
        let y_pos = 210 + (i as i32 * 8);
        canvas.fill_rect(Rect::new(360, y_pos, 80, 5)).ok();
    }
    
    // Draw left arrow
    canvas.set_draw_color(Color::RGB(200, 200, 200));
    let points = [
        sdl2::rect::Point::new(100, 240),
        sdl2::rect::Point::new(130, 210),
        sdl2::rect::Point::new(130, 270),
    ];
    
    for i in 0..points.len() {
        let j = (i + 1) % points.len();
        canvas.draw_line(points[i], points[j])
            .map_err(|e| anyhow::anyhow!("Drawing error: {}", e))?;
    }
    
    Ok(())
}