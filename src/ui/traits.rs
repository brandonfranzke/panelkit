//! Core UI trait definitions
//!
//! This module defines the core traits for UI components and behaviors.

use crate::primitives::{RenderingContext, Rectangle, Point};
use anyhow::Result;

/// Trait for objects that can be positioned on screen
pub trait Positioned {
    /// Get the bounding rectangle of this object
    fn bounds(&self) -> Rectangle;
    
    /// Get the position (top-left corner)
    fn position(&self) -> Point {
        let bounds = self.bounds();
        Point::new(bounds.x, bounds.y)
    }
    
    /// Get the dimensions (width and height)
    fn dimensions(&self) -> (u32, u32) {
        let bounds = self.bounds();
        (bounds.width, bounds.height)
    }
}

/// Trait for objects that can determine if they contain a point
pub trait Contains: Positioned {
    /// Check if the object contains the given point
    fn contains(&self, point: &Point) -> bool {
        self.bounds().contains(point)
    }
}

/// Trait for objects that can be rendered
pub trait Renderable {
    /// Render this object to the given rendering context
    fn render(&self, ctx: &mut dyn RenderingContext) -> Result<()>;
}

/// Core trait for UI components
/// 
/// UI Components are objects that:
/// 1. Have a position and dimensions (Positioned)
/// 2. Can determine if they contain a point (Contains)
/// 3. Can be rendered (Renderable)
/// 4. Can be interacted with
pub trait Component: Positioned + Contains + Renderable {
    /// Set whether the component is enabled (can receive input)
    fn set_enabled(&mut self, enabled: bool);
    
    /// Check if the component is enabled
    fn is_enabled(&self) -> bool;
    
    /// Set whether the component is visible
    fn set_visible(&mut self, visible: bool);
    
    /// Check if the component is visible
    fn is_visible(&self) -> bool;
}

/// Trait for components that can be focused
pub trait Focusable: Component {
    /// Set focus state
    fn set_focused(&mut self, focused: bool);
    
    /// Check if focused
    fn is_focused(&self) -> bool;
    
    /// Handle keyboard input when focused
    fn handle_key_event(&mut self, key_code: u32, pressed: bool) -> Result<bool>;
}

/// Trait for components that can handle touch input
pub trait Touchable: Component {
    /// Handle touch down event
    fn on_touch_down(&mut self, point: &Point) -> Result<bool>;
    
    /// Handle touch move event
    fn on_touch_move(&mut self, point: &Point) -> Result<bool>;
    
    /// Handle touch up event
    fn on_touch_up(&mut self, point: &Point) -> Result<bool>;
}