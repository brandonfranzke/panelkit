//! UI component library for PanelKit
//!
//! This module provides reusable UI components for building pages.

use crate::platform::{Color, GraphicsContext, Point, Rectangle, Renderable};
use anyhow::Result;

pub mod button;
pub mod text;
pub mod layout;

/// Represents a basic UI element that can be rendered to a graphics context
pub trait UIComponent: Renderable {
    /// Get the bounding rectangle of this component
    fn bounds(&self) -> Rectangle;
    
    /// Check if a point is inside this component
    fn contains(&self, point: Point) -> bool {
        let bounds = self.bounds();
        point.x >= bounds.x && 
        point.x < bounds.x + bounds.width as i32 &&
        point.y >= bounds.y && 
        point.y < bounds.y + bounds.height as i32
    }
}

/// A basic colored rectangle component
pub struct ColoredRectangle {
    bounds: Rectangle,
    color: Color,
    filled: bool,
}

impl ColoredRectangle {
    /// Create a new colored rectangle
    pub fn new(x: i32, y: i32, width: u32, height: u32, color: Color, filled: bool) -> Self {
        Self {
            bounds: Rectangle::new(x, y, width, height),
            color,
            filled,
        }
    }
    
    /// Create a filled rectangle
    pub fn filled(x: i32, y: i32, width: u32, height: u32, color: Color) -> Self {
        Self::new(x, y, width, height, color, true)
    }
    
    /// Create an outlined rectangle
    pub fn outlined(x: i32, y: i32, width: u32, height: u32, color: Color) -> Self {
        Self::new(x, y, width, height, color, false)
    }
}

impl Renderable for ColoredRectangle {
    fn render(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        ctx.set_draw_color(self.color)?;
        
        if self.filled {
            ctx.fill_rect(self.bounds)?;
        } else {
            ctx.draw_rect(self.bounds)?;
        }
        
        Ok(())
    }
}

impl UIComponent for ColoredRectangle {
    fn bounds(&self) -> Rectangle {
        self.bounds
    }
}

/// A line component 
pub struct Line {
    start: Point,
    end: Point,
    color: Color,
}

impl Line {
    /// Create a new line
    pub fn new(start_x: i32, start_y: i32, end_x: i32, end_y: i32, color: Color) -> Self {
        Self {
            start: Point::new(start_x, start_y),
            end: Point::new(end_x, end_y),
            color,
        }
    }
}

impl Renderable for Line {
    fn render(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        ctx.set_draw_color(self.color)?;
        ctx.draw_line(self.start, self.end)?;
        Ok(())
    }
}

// Calculate approximate bounding box for a line
impl UIComponent for Line {
    fn bounds(&self) -> Rectangle {
        let min_x = self.start.x.min(self.end.x);
        let min_y = self.start.y.min(self.end.y);
        let width = (self.start.x - self.end.x).abs() as u32;
        let height = (self.start.y - self.end.y).abs() as u32;
        
        Rectangle::new(min_x, min_y, width, height)
    }
}