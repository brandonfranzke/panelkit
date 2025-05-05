//! Button component for UI
//!
//! This module provides a simple button component.

use crate::primitives::{RenderingContext, Renderable, Color, Point, Rectangle};
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
    fn render(&self, ctx: &mut dyn RenderingContext) -> Result<()> {
        // Adjust colors if pressed
        let fill_color = if self.pressed {
            Color::rgba(
                (self.fill_color.r as u16 * 80 / 100) as u8,
                (self.fill_color.g as u16 * 80 / 100) as u8,
                (self.fill_color.b as u16 * 80 / 100) as u8,
                self.fill_color.a
            )
        } else {
            self.fill_color
        };
        
        // Use the new RenderingContext draw_button method
        ctx.draw_button(
            self.bounds, 
            &self.text, 
            fill_color, 
            self.text_color, 
            self.border_color
        )?;
        
        Ok(())
    }
}

impl UIComponent for Button {
    fn bounds(&self) -> Rectangle {
        self.bounds
    }
}