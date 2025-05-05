//! UI component library for PanelKit
//!
//! This module provides reusable UI components for building pages.

use crate::primitives::{RenderingContext, Rectangle, Color, Point};
use crate::ui::traits::{Positioned, Contains, Renderable, Component};
use anyhow::Result;

pub mod button;
pub mod text;
pub mod layout;

/// Base implementation of Component for simpler components
pub struct ComponentBase {
    bounds: Rectangle,
    enabled: bool,
    visible: bool,
}

impl ComponentBase {
    /// Create a new component base
    pub fn new(bounds: Rectangle) -> Self {
        Self {
            bounds,
            enabled: true,
            visible: true,
        }
    }
    
    /// Create a new component base with position and size
    pub fn with_bounds(x: i32, y: i32, width: u32, height: u32) -> Self {
        Self::new(Rectangle::new(x, y, width, height))
    }
}

impl Positioned for ComponentBase {
    fn bounds(&self) -> Rectangle {
        self.bounds
    }
}

impl Contains for ComponentBase {}

// Traits are now integrated into Component trait directly
// No need to re-export anything here

/// A basic colored rectangle component
pub struct ColoredRectangle {
    base: ComponentBase,
    color: Color,
    filled: bool,
}

impl ColoredRectangle {
    /// Create a new colored rectangle
    pub fn new(x: i32, y: i32, width: u32, height: u32, color: Color, filled: bool) -> Self {
        Self {
            base: ComponentBase::with_bounds(x, y, width, height),
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

impl Positioned for ColoredRectangle {
    fn bounds(&self) -> Rectangle {
        self.base.bounds()
    }
}

impl Contains for ColoredRectangle {}

impl Renderable for ColoredRectangle {
    fn render(&self, ctx: &mut dyn RenderingContext) -> Result<()> {
        if !self.base.visible {
            return Ok(());
        }
        
        if self.filled {
            ctx.fill_rect(self.bounds(), self.color)?;
        } else {
            ctx.draw_rect(self.bounds(), self.color)?;
        }
        
        Ok(())
    }
}

impl Component for ColoredRectangle {
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
    
    // Rectangle doesn't process any events, so use default implementation from Component
}

/// A line component 
pub struct Line {
    start: Point,
    end: Point,
    color: Color,
    enabled: bool,
    visible: bool,
}

impl Line {
    /// Create a new line
    pub fn new(start_x: i32, start_y: i32, end_x: i32, end_y: i32, color: Color) -> Self {
        Self {
            start: Point::new(start_x, start_y),
            end: Point::new(end_x, end_y),
            color,
            enabled: true,
            visible: true,
        }
    }
}

impl Positioned for Line {
    fn bounds(&self) -> Rectangle {
        let min_x = self.start.x.min(self.end.x);
        let min_y = self.start.y.min(self.end.y);
        let width = (self.start.x - self.end.x).abs() as u32;
        let height = (self.start.y - self.end.y).abs() as u32;
        
        Rectangle::new(min_x, min_y, width, height)
    }
}

impl Contains for Line {}

impl Renderable for Line {
    fn render(&self, ctx: &mut dyn RenderingContext) -> Result<()> {
        if !self.visible {
            return Ok(());
        }
        
        ctx.draw_line(self.start, self.end, self.color)?;
        Ok(())
    }
}

impl Component for Line {
    fn set_enabled(&mut self, enabled: bool) {
        self.enabled = enabled;
    }
    
    fn is_enabled(&self) -> bool {
        self.enabled
    }
    
    fn set_visible(&mut self, visible: bool) {
        self.visible = visible;
    }
    
    fn is_visible(&self) -> bool {
        self.visible
    }
    
    // Line doesn't process any events, so use default implementation from Component
}