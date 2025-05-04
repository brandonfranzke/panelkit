//! Hello World example page
//!
//! This is a minimal example page that displays "Hello, World!" text.

use crate::event::Event;
use crate::ui::Page;
use anyhow::Result;
use std::any::Any;
use std::sync::{Arc, Mutex};
use sdl2::pixels::Color;
use sdl2::rect::Rect;
use sdl2::render::Canvas;
use sdl2::video::Window;

/// A simple Hello World page
pub struct HelloPage {
    counter: u32,
    canvas: Option<Arc<Mutex<Canvas<Window>>>>,
}

impl HelloPage {
    /// Create a new HelloPage
    pub fn new() -> Self {
        Self { 
            counter: 0,
            canvas: None,
        }
    }
    
    /// Set the SDL2 canvas
    pub fn set_canvas(&mut self, canvas: Arc<Mutex<Canvas<Window>>>) {
        self.canvas = Some(canvas);
    }
}

impl Page for HelloPage {
    fn init(&mut self) -> Result<()> {
        log::info!("HelloPage initialized");
        Ok(())
    }
    
    fn render(&self) -> Result<()> {
        log::info!("HelloPage: Starting render");
        if let Some(canvas_arc) = &self.canvas {
            log::info!("HelloPage: Canvas found, locking");
            let mut canvas = canvas_arc.lock().unwrap();
            log::info!("HelloPage: Canvas locked successfully");
            
            // Clear the canvas with a green background
            log::info!("HelloPage: Setting green background");
            canvas.set_draw_color(Color::RGB(50, 200, 100));
            canvas.clear();
            log::info!("HelloPage: Background cleared");
            
            // Draw title text area
            canvas.set_draw_color(Color::RGB(30, 120, 60));
            canvas.fill_rect(Rect::new(0, 0, 800, 70))
                .map_err(|e| anyhow::anyhow!("Error drawing title area: {}", e))?;
            
            // Draw a white rectangle for the counter display
            canvas.set_draw_color(Color::RGB(255, 255, 255));
            canvas.fill_rect(Rect::new(350, 200, 100, 50))
                .map_err(|e| anyhow::anyhow!("Error drawing counter display: {}", e))?;
            
            // Draw "Hello, World!" box
            canvas.set_draw_color(Color::RGB(255, 255, 255));
            canvas.fill_rect(Rect::new(250, 100, 300, 60))
                .map_err(|e| anyhow::anyhow!("Error drawing text box: {}", e))?;
            canvas.set_draw_color(Color::RGB(0, 0, 0));
            canvas.draw_rect(Rect::new(250, 100, 300, 60))
                .map_err(|e| anyhow::anyhow!("Error drawing text box outline: {}", e))?;
            
            // Draw navigation arrows
            canvas.set_draw_color(Color::RGB(200, 200, 200));
            
            // Right arrow (to Demo page)
            let right_arrow_points = [
                (700, 240),  // Point
                (670, 210),  // Top
                (670, 270)   // Bottom
            ];
            
            for i in 0..right_arrow_points.len() {
                let j = (i + 1) % right_arrow_points.len();
                canvas.draw_line(
                    sdl2::rect::Point::new(right_arrow_points[i].0, right_arrow_points[i].1),
                    sdl2::rect::Point::new(right_arrow_points[j].0, right_arrow_points[j].1)
                ).map_err(|e| anyhow::anyhow!("Error drawing arrow: {}", e))?;
            }
            
            // Don't call present() here - the platform driver will do that
        } else {
            log::info!("HelloPage rendered with counter: {} (no canvas available)", self.counter);
        }
        
        Ok(())
    }
    
    fn handle_event(&mut self, event: &Event) -> Result<Option<String>> {
        match event {
            Event::Touch { x, y, action } => {
                log::info!("HelloPage received touch event: {:?} at ({}, {})", action, x, y);
                
                // Check if counter area was clicked
                if *x >= 350 && *x <= 450 && *y >= 200 && *y <= 250 {
                    self.counter += 1;
                    log::info!("Counter clicked! New value: {}", self.counter);
                }
                
                // Check if right arrow was clicked (for navigation)
                if *x >= 670 && *x <= 700 && *y >= 210 && *y <= 270 &&
                   matches!(action, crate::event::TouchAction::Press) {
                    log::info!("Right arrow clicked! Navigating to Demo page");
                    
                    // Return the page ID to navigate to
                    return Ok(Some("demo".to_string()));
                }
            }
            _ => {}
        }
        
        // No navigation requested
        Ok(None)
    }
    
    fn on_activate(&mut self) -> Result<()> {
        log::info!("HelloPage activated");
        Ok(())
    }
    
    fn on_deactivate(&mut self) -> Result<()> {
        log::info!("HelloPage deactivated");
        Ok(())
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}