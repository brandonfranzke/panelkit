//! Button component for UI
//!
//! This module provides a simple button component.

use crate::primitives::{RenderingContext, Color, Rectangle};
use crate::ui::components::{ComponentBase};
use crate::ui::traits::{Component, Positioned, Contains, Renderable};
use crate::event::TouchEvent;
use anyhow::Result;

/// A simple button component
pub struct Button {
    base: ComponentBase,
    fill_color: Color,
    border_color: Color,
    text: String,
    text_color: Color,
    pressed: bool,
    focused: bool,
    on_click: Option<Box<dyn Fn() -> Result<()>>>,
}

impl Button {
    /// Create a new button
    pub fn new(x: i32, y: i32, width: u32, height: u32, text: &str) -> Self {
        Self {
            base: ComponentBase::with_bounds(x, y, width, height),
            fill_color: Color::rgb(48, 169, 255),
            border_color: Color::rgb(0, 72, 255),
            text: text.to_string(),
            text_color: Color::rgb(255, 255, 255),
            pressed: false,
            focused: false,
            on_click: None,
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
    
    /// Set a click handler for the button
    pub fn with_on_click<F>(mut self, handler: F) -> Self 
    where 
        F: Fn() -> Result<()> + 'static 
    {
        self.on_click = Some(Box::new(handler));
        self
    }
    
    /// Execute the click handler if set
    fn handle_click(&self) -> Result<bool> {
        if let Some(handler) = &self.on_click {
            handler()?;
            Ok(true)
        } else {
            Ok(false)
        }
    }
}

impl Positioned for Button {
    fn bounds(&self) -> Rectangle {
        self.base.bounds()
    }
}

impl Contains for Button {}

impl Renderable for Button {
    fn render(&self, ctx: &mut dyn RenderingContext) -> Result<()> {
        if !self.base.visible {
            return Ok(());
        }
        
        // Adjust colors if pressed or focused
        let fill_color = if self.pressed {
            Color::rgba(
                (self.fill_color.r as u16 * 80 / 100) as u8,
                (self.fill_color.g as u16 * 80 / 100) as u8,
                (self.fill_color.b as u16 * 80 / 100) as u8,
                self.fill_color.a
            )
        } else if self.focused {
            Color::rgba(
                (self.fill_color.r as u16 * 90 / 100) as u8,
                (self.fill_color.g as u16 * 90 / 100) as u8,
                (self.fill_color.b as u16 * 90 / 100) as u8,
                self.fill_color.a
            )
        } else {
            self.fill_color
        };
        
        // Use the RenderingContext draw_button method
        ctx.draw_button(
            self.bounds(), 
            &self.text, 
            fill_color, 
            self.text_color, 
            self.border_color
        )?;
        
        Ok(())
    }
}

// Button basic component methods now in the impl Component block

// Implementation of event handling methods for Button
impl Component for Button {
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
    
    fn on_touch_down(&mut self, _event: &mut TouchEvent) -> Result<bool> {
        // No need to check enabled/visible - Component trait already does this
        self.pressed = true;
        Ok(true)
    }
    
    fn on_touch_move(&mut self, event: &mut TouchEvent) -> Result<bool> {
        if !self.pressed {
            return Ok(false);
        }
        
        // Update visual pressed state based on whether the touch is still within the button
        let inside = self.contains(&event.position);
        self.pressed = inside;
        Ok(inside)
    }
    
    fn on_touch_up(&mut self, event: &mut TouchEvent) -> Result<bool> {
        if !self.pressed {
            return Ok(false);
        }
        
        self.pressed = false;
        
        // Only trigger the click if the touch was released inside the button
        if self.contains(&event.position) {
            self.handle_click()
        } else {
            Ok(false)
        }
    }
}