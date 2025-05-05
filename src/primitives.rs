//! Core rendering primitives for PanelKit
//!
//! This module defines the basic types used for all rendering operations.

use anyhow::Result;
use std::fmt::Debug;
use std::any::Any;

/// RGB color representation
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Color {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    pub a: u8, // Alpha channel (0 = transparent, 255 = opaque)
}

impl Color {
    /// Create a new RGB color with full opacity
    pub fn rgb(r: u8, g: u8, b: u8) -> Self {
        Self { r, g, b, a: 255 }
    }
    
    /// Create a new RGBA color with specified opacity
    pub fn rgba(r: u8, g: u8, b: u8, a: u8) -> Self {
        Self { r, g, b, a }
    }
    
    // Common colors
    pub fn black() -> Self { Self::rgb(0, 0, 0) }
    pub fn white() -> Self { Self::rgb(255, 255, 255) }
    pub fn red() -> Self { Self::rgb(255, 0, 0) }
    pub fn green() -> Self { Self::rgb(0, 255, 0) }
    pub fn blue() -> Self { Self::rgb(0, 0, 255) }
    pub fn transparent() -> Self { Self::rgba(0, 0, 0, 0) }
    
    // UI-specific colors
    pub fn ui_background() -> Self { Self::rgb(240, 240, 240) }
    pub fn ui_accent() -> Self { Self::rgb(0, 120, 215) }
    pub fn ui_text() -> Self { Self::rgb(10, 10, 10) }
}

/// 2D point representation
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Point {
    pub x: i32,
    pub y: i32,
}

impl Point {
    /// Create a new point
    pub fn new(x: i32, y: i32) -> Self {
        Self { x, y }
    }
    
    /// Create a point at the origin (0, 0)
    pub fn origin() -> Self {
        Self { x: 0, y: 0 }
    }
    
    /// Calculate the distance to another point
    pub fn distance(&self, other: &Point) -> f32 {
        let dx = (self.x - other.x) as f32;
        let dy = (self.y - other.y) as f32;
        (dx * dx + dy * dy).sqrt()
    }
}

/// Rectangle representation
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Rectangle {
    pub x: i32,
    pub y: i32,
    pub width: u32,
    pub height: u32,
}

impl Rectangle {
    /// Create a new rectangle
    pub fn new(x: i32, y: i32, width: u32, height: u32) -> Self {
        Self { x, y, width, height }
    }
    
    /// Create a rectangle at the origin (0, 0)
    pub fn with_size(width: u32, height: u32) -> Self {
        Self::new(0, 0, width, height)
    }
    
    /// Check if a point is inside the rectangle
    pub fn contains(&self, point: &Point) -> bool {
        point.x >= self.x 
            && point.x < self.x + self.width as i32
            && point.y >= self.y 
            && point.y < self.y + self.height as i32
    }
    
    /// Get the center point of the rectangle
    pub fn center(&self) -> Point {
        Point::new(
            self.x + (self.width as i32 / 2),
            self.y + (self.height as i32 / 2)
        )
    }
    
    /// Create a rectangle centered at a point
    pub fn centered_at(center: Point, width: u32, height: u32) -> Self {
        Self::new(
            center.x - (width as i32 / 2),
            center.y - (height as i32 / 2),
            width,
            height
        )
    }
    
    /// Get the top-left corner as a Point
    pub fn top_left(&self) -> Point {
        Point::new(self.x, self.y)
    }
    
    /// Get the bottom-right corner as a Point
    pub fn bottom_right(&self) -> Point {
        Point::new(
            self.x + self.width as i32,
            self.y + self.height as i32
        )
    }
}

/// Font size representation
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FontSize {
    Small,  // ~12pt
    Medium, // ~16pt
    Large,  // ~20pt
    ExtraLarge, // ~24pt
    Custom(u16),
}

impl FontSize {
    /// Convert font size to points
    pub fn to_points(&self) -> u16 {
        match self {
            FontSize::Small => 12,
            FontSize::Medium => 16,
            FontSize::Large => 20,
            FontSize::ExtraLarge => 24,
            FontSize::Custom(size) => *size,
        }
    }
}

/// Text alignment options
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TextAlignment {
    Left,
    Center,
    Right,
}

/// Text style for rendering
#[derive(Debug, Clone)]
pub struct TextStyle {
    pub color: Color,
    pub font_size: FontSize,
    pub alignment: TextAlignment,
    pub bold: bool,
    pub italic: bool,
}

impl TextStyle {
    /// Create a new text style with default parameters
    pub fn new(color: Color) -> Self {
        Self {
            color,
            font_size: FontSize::Medium,
            alignment: TextAlignment::Left,
            bold: false,
            italic: false,
        }
    }
    
    /// Set the font size
    pub fn with_size(mut self, size: FontSize) -> Self {
        self.font_size = size;
        self
    }
    
    /// Set the text alignment
    pub fn with_alignment(mut self, alignment: TextAlignment) -> Self {
        self.alignment = alignment;
        self
    }
    
    /// Set bold styling
    pub fn with_bold(mut self, bold: bool) -> Self {
        self.bold = bold;
        self
    }
    
    /// Set italic styling
    pub fn with_italic(mut self, italic: bool) -> Self {
        self.italic = italic;
        self
    }
    
    /// Create a default text style
    pub fn default() -> Self {
        Self::new(Color::ui_text())
    }
}

/// Rendering context that provides drawing primitives
///
/// This trait defines the core drawing operations that any
/// rendering implementation must provide.
pub trait RenderingContext {
    /// Initialize the rendering context with the given dimensions
    fn init(&mut self, width: u32, height: u32) -> Result<()>;
    
    /// Present the rendered content to the screen
    fn present(&mut self) -> Result<()>;
    
    /// Get the current screen dimensions
    fn dimensions(&self) -> (u32, u32);
    
    /// Clean up any resources used by the rendering context
    fn cleanup(&mut self);
    
    /// Clear the screen with a specific color
    fn clear(&mut self, color: Color) -> Result<()>;
    
    /// Draw a filled rectangle
    fn fill_rect(&mut self, rect: Rectangle, color: Color) -> Result<()>;
    
    /// Draw a rectangle outline
    fn draw_rect(&mut self, rect: Rectangle, color: Color) -> Result<()>;
    
    /// Draw a line between two points
    fn draw_line(&mut self, start: Point, end: Point, color: Color) -> Result<()>;
    
    /// Draw text at a specific position
    fn draw_text(&mut self, text: &str, position: Point, style: TextStyle) -> Result<()>;
    
    /// Draw a button with text
    fn draw_button(&mut self, rect: Rectangle, text: &str, bg_color: Color, text_color: Color, border_color: Color) -> Result<()>;
    
    /// Create a new surface/canvas (for offscreen rendering)
    fn create_surface(&mut self, width: u32, height: u32) -> Result<Box<dyn Surface>>;
    
    /// Cast to Any for downcasting to specific backend type if needed
    fn as_any(&self) -> &dyn Any;
    
    /// Cast to Any for downcasting to specific backend type if needed
    fn as_any_mut(&mut self) -> &mut dyn Any;
}

/// Surface for offscreen rendering
pub trait Surface {
    /// Clear the surface with a specific color
    fn clear(&mut self, color: Color) -> Result<()>;
    
    /// Get the dimensions of the surface
    fn dimensions(&self) -> (u32, u32);
    
    /// Draw a filled rectangle on the surface
    fn fill_rect(&mut self, rect: Rectangle, color: Color) -> Result<()>;
    
    /// Draw a rectangle outline on the surface
    fn draw_rect(&mut self, rect: Rectangle, color: Color) -> Result<()>;
    
    /// Draw a line between two points on the surface
    fn draw_line(&mut self, start: Point, end: Point, color: Color) -> Result<()>;
    
    /// Draw text at a specific position on the surface
    fn draw_text(&mut self, text: &str, position: Point, style: TextStyle) -> Result<()>;
    
    /// Cast to Any for downcasting to specific surface type if needed
    fn as_any(&self) -> &dyn Any;
    
    /// Cast to Any for downcasting to specific surface type if needed
    fn as_any_mut(&mut self) -> &mut dyn Any;
}

/// A rendering primitive that can be drawn to a rendering context
pub trait Renderable {
    /// Render this primitive to the given rendering context
    fn render(&self, ctx: &mut dyn RenderingContext) -> Result<()>;
}