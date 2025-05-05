//! Hello World example page
//!
//! This is a minimal example page that displays "Hello, World!" text.

use crate::event::Event;
use crate::ui::Page;
use crate::logging;
use crate::primitives::{RenderingContext, Color, Point, Rectangle};
use crate::ui::traits::Renderable;
use crate::ui::components::{layout::TitleBar, text::Text, text::TextAlign, ColoredRectangle};
use anyhow::Result;
use std::any::Any;

/// A simple Hello World page
pub struct HelloPage {
    counter: u32,
    width: u32,
    height: u32,
    counter_box: Rectangle,
    right_arrow: [Point; 3],
    logger: &'static logging::ComponentLogger,
}

impl HelloPage {
    /// Create a new HelloPage
    pub fn new() -> Self {
        Self { 
            counter: 0,
            width: 800,
            height: 480,
            counter_box: Rectangle::new(350, 200, 100, 50),
            right_arrow: [
                Point::new(700, 240), // Point
                Point::new(670, 210), // Top
                Point::new(670, 270), // Bottom
            ],
            logger: logging::hello_logger(),
        }
    }
}

impl Page for HelloPage {
    fn init(&mut self) -> Result<()> {
        self.logger.info("Initializing HelloPage");
        Ok(())
    }
    
    fn render(&self, ctx: &mut dyn RenderingContext) -> Result<()> {
        self.logger.trace("Rendering HelloPage");
        
        // Clear the screen with a green background
        ctx.clear(Color::rgb(50, 200, 100))?;
        
        // Draw title bar
        TitleBar::new(
            0, 0, self.width, 70, 
            "Hello Page", 
            Color::rgb(255, 255, 255), 
            Color::rgb(30, 120, 60)
        ).render(ctx)?;
        
        // Draw "Hello, World!" text with a border
        ColoredRectangle::filled(250, 100, 300, 60, Color::rgb(255, 255, 255)).render(ctx)?;
        ColoredRectangle::outlined(250, 100, 300, 60, Color::rgb(0, 0, 0)).render(ctx)?;
        
        Text::new(400, 130, "Hello, World!", Color::rgb(0, 0, 0))
            .with_align(TextAlign::Center)
            .with_font_size(24)
            .render(ctx)?;
        
        // Draw counter display
        ColoredRectangle::filled(
            self.counter_box.x, 
            self.counter_box.y, 
            self.counter_box.width, 
            self.counter_box.height, 
            Color::rgb(255, 255, 255)
        ).render(ctx)?;
        
        // Draw counter value
        let count_to_show = std::cmp::min(self.counter, 5);
        for i in 0..count_to_show {
            let x_pos = 360 + (i as i32 * 15);
            ColoredRectangle::filled(
                x_pos, 210, 10, 30, Color::rgb(0, 0, 0)
            ).render(ctx)?;
        }
        
        // Draw navigation arrow to Demo page
        let arrow_color = Color::rgb(200, 200, 200);
        for i in 0..self.right_arrow.len() {
            let j = (i + 1) % self.right_arrow.len();
            ctx.draw_line(self.right_arrow[i], self.right_arrow[j], arrow_color)?;
        }
        
        self.logger.trace("HelloPage rendered successfully");
        Ok(())
    }
    
    fn handle_event(&mut self, event: &Event) -> Result<Option<String>> {
        match event {
            Event::Touch { x, y, action } => {
                self.logger.debug(&format!("Received touch event: {:?} at ({}, {})", action, x, y));
                
                // Check if counter area was clicked
                if *x >= self.counter_box.x && 
                   *x <= self.counter_box.x + self.counter_box.width as i32 &&
                   *y >= self.counter_box.y && 
                   *y <= self.counter_box.y + self.counter_box.height as i32 {
                    self.counter += 1;
                    self.logger.info(&format!("Counter incremented to: {}", self.counter));
                }
                
                // Check if right arrow was clicked (for navigation)
                let arrow_bounds = Rectangle::new(670, 210, 30, 60);
                if *x >= arrow_bounds.x && 
                   *x <= arrow_bounds.x + arrow_bounds.width as i32 &&
                   *y >= arrow_bounds.y && 
                   *y <= arrow_bounds.y + arrow_bounds.height as i32 &&
                   matches!(action, crate::event::TouchAction::Press) {
                    self.logger.info("Right arrow clicked, navigating to Demo page");
                    return Ok(Some("demo".to_string()));
                }
            }
            _ => {}
        }
        
        Ok(None)
    }
    
    fn on_activate(&mut self) -> Result<()> {
        self.logger.info("HelloPage activated");
        Ok(())
    }
    
    fn on_deactivate(&mut self) -> Result<()> {
        self.logger.info("HelloPage deactivated");
        Ok(())
    }
    
    fn update_layout(&mut self, width: u32, height: u32) -> Result<()> {
        self.logger.debug(&format!("Updating layout to {}x{}", width, height));
        
        self.width = width;
        self.height = height;
        
        // Update responsive layout elements
        self.counter_box = Rectangle::new(
            (width as i32 - 100) / 2,  // Center horizontally
            200, 
            100, 
            50
        );
        
        // Right arrow (depends on screen width)
        self.right_arrow = [
            Point::new(width as i32 - 100, height as i32 / 2),    // Point
            Point::new(width as i32 - 130, height as i32 / 2 - 30), // Top
            Point::new(width as i32 - 130, height as i32 / 2 + 30), // Bottom
        ];
        
        Ok(())
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}