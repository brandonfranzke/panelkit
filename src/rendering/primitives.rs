//! Rendering primitives for PanelKit
//!
//! This module defines the basic types used for rendering operations.
//! 
//! It also provides conversions between these types and the graphics module types
//! to ensure compatibility between the two systems.

use crate::platform::graphics;

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
    
    /// Convert from a graphics::Color
    pub fn from_graphics(color: graphics::Color) -> Self {
        Self {
            r: color.r,
            g: color.g,
            b: color.b,
            a: 255,
        }
    }
    
    /// Convert to a graphics::Color
    pub fn to_graphics(&self) -> graphics::Color {
        graphics::Color::rgb(self.r, self.g, self.b)
    }
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
    
    /// Convert from a graphics::Point
    pub fn from_graphics(point: graphics::Point) -> Self {
        Self {
            x: point.x,
            y: point.y,
        }
    }
    
    /// Convert to a graphics::Point
    pub fn to_graphics(&self) -> graphics::Point {
        graphics::Point::new(self.x, self.y)
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
    
    /// Convert from a graphics::Rectangle
    pub fn from_graphics(rect: graphics::Rectangle) -> Self {
        Self {
            x: rect.x,
            y: rect.y,
            width: rect.width,
            height: rect.height,
        }
    }
    
    /// Convert to a graphics::Rectangle
    pub fn to_graphics(&self) -> graphics::Rectangle {
        graphics::Rectangle::new(self.x, self.y, self.width, self.height)
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