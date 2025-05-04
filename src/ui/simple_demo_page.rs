//! Simple demo page
//!
//! This is a simplified demo page using our component system.

use crate::event::Event;
use crate::ui::Page;
use crate::logging;
use crate::platform::{Color, GraphicsContext, Point, Rectangle};
use crate::ui::components::{
    button::Button, 
    layout::{Container, TitleBar}, 
    text::Text, 
    text::TextAlign, 
    ColoredRectangle, 
    Line
};
use crate::error::Result;
use std::any::Any;

/// A demo page showcasing various UI components
pub struct SimpleDemoPage {
    counter: u32,
    width: u32,
    height: u32,
    button_bounds: Rectangle,
    slider_bounds: Rectangle,
    slider_value: u32,
    left_arrow: [Point; 3],
    logger: &'static logging::ComponentLogger,
}

impl SimpleDemoPage {
    /// Create a new simple demo page
    pub fn new() -> Self {
        Self {
            counter: 0,
            width: 800,
            height: 480,
            button_bounds: Rectangle::new(325, 300, 150, 50),
            slider_bounds: Rectangle::new(250, 400, 300, 20),
            slider_value: 150, // Initial slider value (out of 300)
            left_arrow: [
                Point::new(100, 240), // Point
                Point::new(130, 210), // Top
                Point::new(130, 270), // Bottom
            ],
            logger: logging::get_logger("DemoPage"),
        }
    }
    
    /// Draw the slider with current value
    fn draw_slider(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        // Draw slider background
        ColoredRectangle::filled(
            self.slider_bounds.x,
            self.slider_bounds.y,
            self.slider_bounds.width,
            self.slider_bounds.height,
            Color::rgb(200, 200, 200)
        ).render(ctx)?;
        
        // Draw slider filled portion
        ColoredRectangle::filled(
            self.slider_bounds.x,
            self.slider_bounds.y,
            self.slider_value,
            self.slider_bounds.height,
            Color::rgb(48, 169, 255)
        ).render(ctx)?;
        
        Ok(())
    }
}

impl Page for SimpleDemoPage {
    fn init(&mut self) -> Result<()> {
        self.logger.info("Initializing DemoPage");
        Ok(())
    }
    
    fn render(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        self.logger.trace("Rendering DemoPage");
        
        // Clear with blue background
        ctx.clear(Color::rgb(180, 200, 255))?;
        
        // Draw title bar
        TitleBar::new(
            0, 0, self.width, 70, 
            "Demo Page", 
            Color::rgb(255, 255, 255), 
            Color::rgb(50, 50, 150)
        ).render(ctx)?;
        
        // Draw "World" text with a border
        ColoredRectangle::filled(300, 150, 200, 40, Color::rgb(255, 255, 255)).render(ctx)?;
        ColoredRectangle::outlined(300, 150, 200, 40, Color::rgb(0, 0, 0)).render(ctx)?;
        
        Text::new(400, 170, "World!", Color::rgb(0, 0, 0))
            .with_align(TextAlign::Center)
            .with_font_size(24)
            .render(ctx)?;
        
        // Draw counter display
        ColoredRectangle::filled(350, 200, 100, 50, Color::rgb(255, 255, 255)).render(ctx)?;
        
        // Draw counter value as horizontal lines
        let count_to_show = std::cmp::min(self.counter, 5);
        for i in 0..count_to_show {
            let y_pos = 210 + (i as i32 * 8);
            ColoredRectangle::filled(
                360, y_pos, 80, 5, Color::rgb(0, 0, 0)
            ).render(ctx)?;
        }
        
        // Draw interactive button
        let button_color = if self.counter % 2 == 0 {
            Color::rgb(48, 169, 255)
        } else {
            Color::rgb(48, 255, 169)
        };
        
        ColoredRectangle::filled(
            self.button_bounds.x,
            self.button_bounds.y,
            self.button_bounds.width,
            self.button_bounds.height,
            button_color
        ).render(ctx)?;
        
        ColoredRectangle::outlined(
            self.button_bounds.x,
            self.button_bounds.y,
            self.button_bounds.width,
            self.button_bounds.height,
            Color::rgb(0, 72, 255)
        ).render(ctx)?;
        
        Text::new(
            self.button_bounds.x + (self.button_bounds.width as i32 / 2),
            self.button_bounds.y + (self.button_bounds.height as i32 / 2), 
            "Click Me", 
            Color::rgb(255, 255, 255)
        )
        .with_align(TextAlign::Center)
        .render(ctx)?;
        
        // Draw slider
        self.draw_slider(ctx)?;
        
        // Draw navigation arrow to Hello page
        ctx.set_draw_color(Color::rgb(200, 200, 200))?;
        for i in 0..self.left_arrow.len() {
            let j = (i + 1) % self.left_arrow.len();
            ctx.draw_line(self.left_arrow[i], self.left_arrow[j])?;
        }
        
        self.logger.trace("DemoPage rendered successfully");
        Ok(())
    }
    
    fn handle_event(&mut self, event: &Event) -> Result<Option<String>> {
        match event {
            Event::Touch { x, y, action } => {
                self.logger.debug(&format!("Received touch event: {:?} at ({}, {})", action, x, y));
                
                // Check if button was clicked
                if *x >= self.button_bounds.x && 
                   *x <= self.button_bounds.x + self.button_bounds.width as i32 &&
                   *y >= self.button_bounds.y && 
                   *y <= self.button_bounds.y + self.button_bounds.height as i32 &&
                   matches!(action, crate::event::TouchAction::Press) {
                    self.counter += 1;
                    self.logger.info(&format!("Button clicked! Counter: {}", self.counter));
                }
                
                // Check if slider was clicked/moved
                if *x >= self.slider_bounds.x && 
                   *x <= self.slider_bounds.x + self.slider_bounds.width as i32 &&
                   *y >= self.slider_bounds.y && 
                   *y <= self.slider_bounds.y + self.slider_bounds.height as i32 {
                    // Update slider value based on x position
                    let new_value = (*x - self.slider_bounds.x) as u32;
                    if new_value <= self.slider_bounds.width {
                        self.slider_value = new_value;
                        self.logger.debug(&format!("Slider value: {}/{}", 
                            self.slider_value, self.slider_bounds.width));
                    }
                }
                
                // Check if left arrow was clicked (for navigation)
                let arrow_bounds = Rectangle::new(100, 210, 30, 60);
                if *x >= arrow_bounds.x && 
                   *x <= arrow_bounds.x + arrow_bounds.width as i32 &&
                   *y >= arrow_bounds.y && 
                   *y <= arrow_bounds.y + arrow_bounds.height as i32 &&
                   matches!(action, crate::event::TouchAction::Press) {
                    self.logger.info("Left arrow clicked, navigating to Hello page");
                    return Ok(Some("hello".to_string()));
                }
            }
            _ => {}
        }
        
        Ok(None)
    }
    
    fn on_activate(&mut self) -> Result<()> {
        self.logger.info("DemoPage activated");
        Ok(())
    }
    
    fn on_deactivate(&mut self) -> Result<()> {
        self.logger.info("DemoPage deactivated");
        Ok(())
    }
    
    fn update_layout(&mut self, width: u32, height: u32) -> Result<()> {
        self.logger.debug(&format!("Updating layout to {}x{}", width, height));
        
        self.width = width;
        self.height = height;
        
        // Update responsive layout elements
        let center_x = width as i32 / 2;
        
        // Center the button horizontally
        self.button_bounds = Rectangle::new(
            center_x - 75,
            300, 
            150, 
            50
        );
        
        // Center the slider horizontally
        self.slider_bounds = Rectangle::new(
            center_x - 150,
            400, 
            300, 
            20
        );
        
        // Left arrow (depends on screen dimensions)
        self.left_arrow = [
            Point::new(100, height as i32 / 2),            // Point
            Point::new(130, height as i32 / 2 - 30),       // Top
            Point::new(130, height as i32 / 2 + 30),       // Bottom
        ];
        
        // Also adjust the slider value proportionally if the width changed
        if width != 800 {
            let ratio = self.slider_value as f32 / 300.0;
            self.slider_value = (ratio * self.slider_bounds.width as f32) as u32;
        }
        
        Ok(())
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}