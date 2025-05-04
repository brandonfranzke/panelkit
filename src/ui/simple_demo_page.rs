//! Simple SDL2-based demo page
//!
//! This is a simplified demo page without LVGL dependencies.

use crate::event::Event;
use crate::ui::Page;
use anyhow::Result;
use std::sync::{Arc, Mutex};
use std::any::Any;

use sdl2::pixels::Color;
use sdl2::rect::Rect;
use sdl2::render::Canvas;
use sdl2::video::Window;

/// A simple demo page using SDL2 directly
pub struct SimpleDemoPage {
    canvas: Option<Arc<Mutex<Canvas<Window>>>>,
    counter: u32,
}

impl SimpleDemoPage {
    /// Create a new simple demo page
    pub fn new() -> Self {
        Self {
            canvas: None,
            counter: 0,
        }
    }
    
    /// Set the SDL2 canvas
    pub fn set_canvas(&mut self, canvas: Arc<Mutex<Canvas<Window>>>) {
        self.canvas = Some(canvas);
    }
}

impl Page for SimpleDemoPage {
    fn init(&mut self) -> Result<()> {
        log::info!("SimpleDemoPage initialized");
        Ok(())
    }
    
    fn render(&self) -> Result<()> {
        if let Some(canvas_arc) = &self.canvas {
            let mut canvas = canvas_arc.lock().unwrap();
            
            // Clear the canvas with a blue background
            canvas.set_draw_color(Color::RGB(100, 149, 237)); // Cornflower blue
            canvas.clear();
            
            // Draw title text area
            canvas.set_draw_color(Color::RGB(50, 50, 150));
            canvas.fill_rect(Rect::new(0, 0, 800, 70))
                .map_err(|e| anyhow::anyhow!("Error drawing title area: {}", e))?;
            
            // Draw a white rectangle for the counter
            canvas.set_draw_color(Color::RGB(255, 255, 255));
            canvas.fill_rect(Rect::new(350, 200, 100, 50))
                .map_err(|e| anyhow::anyhow!("Error drawing counter: {}", e))?;
            
            // Draw a button
            canvas.set_draw_color(Color::RGB(48, 169, 255));
            canvas.fill_rect(Rect::new(325, 300, 150, 50))
                .map_err(|e| anyhow::anyhow!("Error drawing button fill: {}", e))?;
            canvas.set_draw_color(Color::RGB(0, 72, 255));
            canvas.draw_rect(Rect::new(325, 300, 150, 50))
                .map_err(|e| anyhow::anyhow!("Error drawing button outline: {}", e))?;
            
            // Draw a slider
            canvas.set_draw_color(Color::RGB(200, 200, 200));
            canvas.fill_rect(Rect::new(250, 400, 300, 20))
                .map_err(|e| anyhow::anyhow!("Error drawing slider background: {}", e))?;
            canvas.set_draw_color(Color::RGB(48, 169, 255));
            canvas.fill_rect(Rect::new(250, 400, 150, 20))
                .map_err(|e| anyhow::anyhow!("Error drawing slider fill: {}", e))?;
                
            // Draw navigation arrows
            canvas.set_draw_color(Color::RGB(200, 200, 200));
            
            // Left arrow (to Hello page)
            let left_arrow_points = [
                (100, 240),  // Point
                (130, 210),  // Top
                (130, 270)   // Bottom
            ];
            
            for i in 0..left_arrow_points.len() {
                let j = (i + 1) % left_arrow_points.len();
                canvas.draw_line(
                    sdl2::rect::Point::new(left_arrow_points[i].0, left_arrow_points[i].1),
                    sdl2::rect::Point::new(left_arrow_points[j].0, left_arrow_points[j].1)
                ).map_err(|e| anyhow::anyhow!("Error drawing arrow: {}", e))?;
            }
            
            // Don't call present() here - the platform driver will do that
        }
        
        Ok(())
    }
    
    fn handle_event(&mut self, event: &Event) -> Result<Option<String>> {
        match event {
            Event::Touch { x, y, action } => {
                log::info!("SimpleDemoPage received touch event: {:?} at ({}, {})", action, x, y);
                
                // Check if button was clicked (simple hit test)
                if *x >= 325 && *x <= 475 && *y >= 300 && *y <= 350 {
                    self.counter += 1;
                    log::info!("Button clicked! Counter: {}", self.counter);
                }
                
                // Check if left arrow was clicked (for navigation)
                if *x >= 100 && *x <= 130 && *y >= 210 && *y <= 270 &&
                   matches!(action, crate::event::TouchAction::Press) {
                    log::info!("Left arrow clicked! Navigating to Hello page");
                    
                    // Return the page ID to navigate to
                    return Ok(Some("hello".to_string()));
                }
            }
            _ => {}
        }
        
        // No navigation requested
        Ok(None)
    }
    
    fn on_activate(&mut self) -> Result<()> {
        log::info!("SimpleDemoPage activated");
        Ok(())
    }
    
    fn on_deactivate(&mut self) -> Result<()> {
        log::info!("SimpleDemoPage deactivated");
        Ok(())
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}