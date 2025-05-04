//! Button component for UI
//!
//! This module provides a simple button component.

use crate::platform::{GraphicsContext, Renderable};
use crate::platform::graphics::{Color, Point, Rectangle};
use crate::ui::components::{ColoredRectangle, UIComponent};
use anyhow::Result;

/// A simple button component
pub struct Button {
    bounds: Rectangle,
    fill_color: Color,
    border_color: Color,
    text: String,
    text_color: Color,
    pressed: bool,
}

impl Button {
    /// Create a new button
    pub fn new(x: i32, y: i32, width: u32, height: u32, text: &str) -> Self {
        Self {
            bounds: Rectangle::new(x, y, width, height),
            fill_color: Color::rgb(48, 169, 255),
            border_color: Color::rgb(0, 72, 255),
            text: text.to_string(),
            text_color: Color::rgb(255, 255, 255),
            pressed: false,
        }
    }
    
    /// Set the button's pressed state
    pub fn set_pressed(&mut self, pressed: bool) {
        self.pressed = pressed;
    }
    
    /// Set the button's colors
    pub fn with_colors(mut self, fill: Color, border: Color, text: Color) -> Self {
        self.fill_color = fill;
        self.border_color = border;
        self.text_color = text;
        self
    }
    
    /// Check if the button contains a point
    pub fn contains(&self, point: Point) -> bool {
        point.x >= self.bounds.x && 
        point.x < self.bounds.x + self.bounds.width as i32 &&
        point.y >= self.bounds.y && 
        point.y < self.bounds.y + self.bounds.height as i32
    }
}

impl Renderable for Button {
    fn render(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        // Adjust colors if pressed
        let fill_color = if self.pressed {
            Color::rgb(
                (self.fill_color.r as u16 * 80 / 100) as u8,
                (self.fill_color.g as u16 * 80 / 100) as u8,
                (self.fill_color.b as u16 * 80 / 100) as u8,
            )
        } else {
            self.fill_color
        };
        
        // Draw button background
        ColoredRectangle::filled(
            self.bounds.x, 
            self.bounds.y, 
            self.bounds.width, 
            self.bounds.height, 
            fill_color
        ).render(ctx)?;
        
        // Draw button border
        ColoredRectangle::outlined(
            self.bounds.x, 
            self.bounds.y, 
            self.bounds.width, 
            self.bounds.height, 
            self.border_color
        ).render(ctx)?;
        
        // For now, we don't have proper text rendering, so we'll just draw
        // a simplified representation of text using a small rectangle
        let text_width = std::cmp::min(self.text.len() as u32 * 8, self.bounds.width - 10);
        let text_height = 10;
        let text_x = self.bounds.x + (self.bounds.width as i32 - text_width as i32) / 2;
        let text_y = self.bounds.y + (self.bounds.height as i32 - text_height as i32) / 2;
        
        ColoredRectangle::filled(
            text_x, 
            text_y, 
            text_width, 
            text_height, 
            self.text_color
        ).render(ctx)?;
        
        Ok(())
    }
}

impl UIComponent for Button {
    fn bounds(&self) -> crate::platform::graphics::Rectangle {
        self.bounds
    }
}