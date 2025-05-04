//! Graphics abstraction for PanelKit
//!
//! This module provides a platform-agnostic graphics API that isolates
//! the UI system from specific rendering implementations.

use anyhow::Result;
use std::fmt::{Debug, Display};

/// Represents a color in RGB format
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Color {
    pub r: u8,
    pub g: u8,
    pub b: u8,
}

impl Color {
    /// Create a new RGB color
    pub fn rgb(r: u8, g: u8, b: u8) -> Self {
        Self { r, g, b }
    }
    
    /// Common colors
    pub fn black() -> Self { Self::rgb(0, 0, 0) }
    pub fn white() -> Self { Self::rgb(255, 255, 255) }
    pub fn red() -> Self { Self::rgb(255, 0, 0) }
    pub fn green() -> Self { Self::rgb(0, 255, 0) }
    pub fn blue() -> Self { Self::rgb(0, 0, 255) }
}

/// Represents a point in 2D space
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Point {
    pub x: i32,
    pub y: i32,
}

impl Point {
    /// Create a new point
    pub fn new(x: i32, y: i32) -> Self {
        Self { x, y }
    }
}

/// Represents a rectangle in 2D space
#[derive(Debug, Clone, Copy, PartialEq)]
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
}

/// Graphics context that provides drawing primitives
///
/// This trait defines the core drawing operations that any
/// graphics implementation must provide.
pub trait GraphicsContext: Send {
    /// Clear the entire drawing surface with a color
    fn clear(&mut self, color: Color) -> Result<()>;
    
    /// Set the current drawing color
    fn set_draw_color(&mut self, color: Color) -> Result<()>;
    
    /// Draw a filled rectangle
    fn fill_rect(&mut self, rect: Rectangle) -> Result<()>;
    
    /// Draw a rectangle outline
    fn draw_rect(&mut self, rect: Rectangle) -> Result<()>;
    
    /// Draw a line between two points
    fn draw_line(&mut self, start: Point, end: Point) -> Result<()>;
    
    /// Get the dimensions of the drawing surface
    fn dimensions(&self) -> (u32, u32);
}

/// A rendering primitive that can be drawn to a graphics context
pub trait Renderable {
    /// Render this primitive to the given graphics context
    fn render(&self, ctx: &mut dyn GraphicsContext) -> Result<()>;
}