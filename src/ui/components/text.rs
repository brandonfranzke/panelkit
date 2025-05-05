//! Text component for UI
//!
//! This module provides a simple text rendering component using basic shapes.

use crate::primitives::{RenderingContext, Color, Point, Rectangle, TextStyle, FontSize, TextAlignment};
use crate::ui::components::{ColoredRectangle, ComponentBase};
use crate::ui::traits::{Component, Positioned, Contains, Renderable};
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
    base: ComponentBase,
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
        // Create initial dimensions based on text
        let width = text.len() as u32 * 6; // Simple approximation
        let height = 16; // Default height
        
        Self {
            base: ComponentBase::with_bounds(x, y, width, height),
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
    
    /// Calculate the effective bounds based on alignment and dynamic size
    fn effective_bounds(&self) -> Rectangle {
        let text_width = self.calc_width();
        let text_height = self.calc_height();
        let bounds = self.base.bounds();
        
        // Calculate x position based on alignment
        let x = match self.align {
            TextAlign::Left => bounds.x,
            TextAlign::Center => bounds.x - (text_width as i32 / 2),
            TextAlign::Right => bounds.x - text_width as i32,
        };
        
        Rectangle::new(x, bounds.y, text_width, text_height)
    }
}

impl Positioned for Text {
    fn bounds(&self) -> Rectangle {
        self.effective_bounds()
    }
}

impl Contains for Text {}

impl Renderable for Text {
    fn render(&self, ctx: &mut dyn RenderingContext) -> Result<()> {
        if !self.is_visible() {
            return Ok(());
        }
        
        let bounds = self.bounds();
        
        // Draw background if specified
        if let Some(bg_color) = self.background_color {
            ColoredRectangle::filled(
                bounds.x, 
                bounds.y, 
                bounds.width, 
                bounds.height, 
                bg_color
            ).render(ctx)?;
        }
        
        // Use the rendering context's text drawing capability
        let position = Point::new(bounds.x, bounds.y);
        let text_alignment = match self.align {
            TextAlign::Left => TextAlignment::Left,
            TextAlign::Center => TextAlignment::Center,
            TextAlign::Right => TextAlignment::Right,
        };
        
        let style = TextStyle {
            font_size: match self.font_size {
                size if size <= 12 => FontSize::Small,
                size if size <= 16 => FontSize::Medium,
                _ => FontSize::Large,
            },
            color: self.color,
            alignment: text_alignment,
            bold: false,
            italic: false,
        };
        
        ctx.draw_text(&self.text, position, style)?;
        
        Ok(())
    }
}

impl Component for Text {
    fn set_enabled(&mut self, enabled: bool) {
        self.base.enabled = enabled;
    }
    
    fn is_enabled(&self) -> bool {
        self.base.enabled
    }
    
    fn set_visible(&mut self, visible: bool) {
        self.base.visible = visible;
    }
    
    fn is_visible(&self) -> bool {
        self.base.visible
    }
    
    // Text component has no special event handling, so we use the default implementation
}

