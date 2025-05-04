//! Text component for UI
//!
//! This module provides a simple text rendering component using basic shapes.

use crate::platform::{GraphicsContext, Renderable};
use crate::platform::graphics::Color;
use crate::ui::components::{ColoredRectangle, UIComponent};
use anyhow::Result;

/// Text alignment options
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum TextAlign {
    Left,
    Center,
    Right,
}

/// A simple text component that renders text using primitive shapes
pub struct Text {
    x: i32,
    y: i32,
    text: String,
    color: Color,
    background_color: Option<Color>,
    font_size: u32,
    align: TextAlign,
    width: Option<u32>,
}

impl Text {
    /// Create a new text component
    pub fn new(x: i32, y: i32, text: &str, color: Color) -> Self {
        Self {
            x,
            y,
            text: text.to_string(),
            color,
            background_color: None,
            font_size: 12,
            align: TextAlign::Left,
            width: None,
        }
    }
    
    /// Set the background color
    pub fn with_background(mut self, color: Color) -> Self {
        self.background_color = Some(color);
        self
    }
    
    /// Set the font size
    pub fn with_font_size(mut self, size: u32) -> Self {
        self.font_size = size;
        self
    }
    
    /// Set the text alignment
    pub fn with_align(mut self, align: TextAlign) -> Self {
        self.align = align;
        self
    }
    
    /// Set a fixed width for the text
    pub fn with_width(mut self, width: u32) -> Self {
        self.width = Some(width);
        self
    }
    
    /// Calculate the approximate width of the text
    fn calc_width(&self) -> u32 {
        // This is an approximation - a real implementation would measure
        // the actual text width based on the font
        if let Some(width) = self.width {
            width
        } else {
            self.text.len() as u32 * (self.font_size / 2)
        }
    }
    
    /// Calculate the height of the text
    fn calc_height(&self) -> u32 {
        self.font_size + 4
    }
}

impl Renderable for Text {
    fn render(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        let text_width = self.calc_width();
        let text_height = self.calc_height();
        
        // Calculate x position based on alignment
        let x = match self.align {
            TextAlign::Left => self.x,
            TextAlign::Center => self.x - (text_width as i32 / 2),
            TextAlign::Right => self.x - text_width as i32,
        };
        
        // Draw background if specified
        if let Some(bg_color) = self.background_color {
            ColoredRectangle::filled(
                x, 
                self.y, 
                text_width, 
                text_height, 
                bg_color
            ).render(ctx)?;
        }
        
        // For now, since we don't have real font rendering, we'll draw the text
        // as a series of simple shapes based on the characters
        let mut char_x = x;
        for c in self.text.chars() {
            // Simple approximation of character shapes
            draw_character(ctx, char_x, self.y, c, self.color, self.font_size)?;
            char_x += (self.font_size / 2) as i32;
        }
        
        Ok(())
    }
}

impl UIComponent for Text {
    fn bounds(&self) -> crate::platform::graphics::Rectangle {
        let text_width = self.calc_width();
        let text_height = self.calc_height();
        
        // Calculate x position based on alignment
        let x = match self.align {
            TextAlign::Left => self.x,
            TextAlign::Center => self.x - (text_width as i32 / 2),
            TextAlign::Right => self.x - text_width as i32,
        };
        
        crate::platform::graphics::Rectangle::new(x, self.y, text_width, text_height)
    }
}

/// Draw a character using primitive shapes
fn draw_character(
    ctx: &mut dyn GraphicsContext, 
    x: i32, 
    y: i32, 
    c: char, 
    color: Color,
    size: u32
) -> Result<()> {
    // Simplistic character rendering using rectangles
    // In a real implementation, this would use font rendering
    
    let stroke_width = std::cmp::max(1, size / 12);
    let char_width = size / 2;
    let char_height = size;
    
    // For now, we'll just render a simplified version
    match c {
        'A'..='Z' | 'a'..='z' => {
            // Draw a simple rectangle for the letter
            ColoredRectangle::filled(
                x, 
                y, 
                char_width, 
                char_height, 
                color
            ).render(ctx)?;
        },
        '0'..='9' => {
            // Draw a different shape for numbers
            ColoredRectangle::outlined(
                x, 
                y, 
                char_width, 
                char_height, 
                color
            ).render(ctx)?;
            
            // Fill with a lighter version of the color
            ColoredRectangle::filled(
                x + stroke_width as i32, 
                y + stroke_width as i32, 
                char_width - 2 * stroke_width, 
                char_height - 2 * stroke_width, 
                color
            ).render(ctx)?;
        },
        ' ' => {
            // Space character - do nothing
        },
        _ => {
            // All other characters get a simple mark
            ColoredRectangle::filled(
                x, 
                y + (char_height / 2) as i32, 
                char_width, 
                stroke_width, 
                color
            ).render(ctx)?;
        }
    }
    
    Ok(())
}