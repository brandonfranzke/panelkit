//! Transition-capable page implementation
//!
//! This module provides a base implementation for pages that support
//! smooth transitions between them, with keyboard and touch navigation.

use crate::error::Result;
use crate::event::{Event, TouchAction};
use crate::platform::{Color, GraphicsContext, Point, Rectangle, Renderable};
use crate::ui::Page;
use std::any::Any;

/// Direction for page transitions
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TransitionDirection {
    /// Transition to the left
    Left,
    /// Transition to the right
    Right,
    /// No transition
    None,
}

/// Base struct for pages with transition support
pub struct TransitionPage {
    /// Page title
    pub title: String,
    
    /// Background color
    pub bg_color: Color,
    
    /// Text color
    pub text_color: Color,
    
    /// Dimensions
    pub width: u32,
    pub height: u32,
    
    /// Whether this page can transition to the next page
    pub has_next: bool,
    
    /// Whether this page can transition to the previous page
    pub has_prev: bool,
    
    /// ID of next page (if has_next is true)
    pub next_page_id: Option<String>,
    
    /// ID of previous page (if has_prev is true)
    pub prev_page_id: Option<String>,
    
    /// Whether this page is active
    pub is_active: bool,
    
    /// Custom render function
    pub render_fn: Option<Box<dyn Fn(&TransitionPage, &mut dyn GraphicsContext) -> Result<()>>>,
}

impl TransitionPage {
    /// Create a new transition page
    pub fn new(title: &str) -> Self {
        Self {
            title: title.to_string(),
            bg_color: Color::rgb(230, 230, 230), // Light gray background
            text_color: Color::rgb(0, 0, 0),     // Black text
            width: 320,
            height: 240,
            has_next: false,
            has_prev: false,
            next_page_id: None,
            prev_page_id: None,
            is_active: false,
            render_fn: None,
        }
    }
    
    /// Set the page that comes next in the sequence
    pub fn set_next_page(&mut self, page_id: &str) {
        self.has_next = true;
        self.next_page_id = Some(page_id.to_string());
    }
    
    /// Set the page that comes before in the sequence
    pub fn set_prev_page(&mut self, page_id: &str) {
        self.has_prev = true;
        self.prev_page_id = Some(page_id.to_string());
    }
    
    /// Handle transition events
    pub fn handle_transition(&self, direction: TransitionDirection) -> Option<String> {
        match direction {
            TransitionDirection::Right => {
                if self.has_next {
                    self.next_page_id.clone()
                } else {
                    None
                }
            },
            TransitionDirection::Left => {
                if self.has_prev {
                    self.prev_page_id.clone()
                } else {
                    None
                }
            },
            TransitionDirection::None => None,
        }
    }
    
    /// Draw the page content
    pub fn draw_content(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        // Clear background
        ctx.clear(self.bg_color)?;
        
        // If a custom render function is provided, use it
        if let Some(render) = &self.render_fn {
            return render(self, ctx);
        }
        
        // Draw title text (centered at top)
        self.draw_centered_text(ctx, &self.title, self.height / 4)?;
        
        // Draw navigation indicators if needed
        self.draw_navigation_indicators(ctx)?;
        
        Ok(())
    }
    
    /// Draw navigation indicators
    pub fn draw_navigation_indicators(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        // Draw right arrow if has_next is true
        if self.has_next {
            self.draw_right_arrow(ctx)?;
        }
        
        // Draw left arrow if has_prev is true
        if self.has_prev {
            self.draw_left_arrow(ctx)?;
        }
        
        Ok(())
    }
    
    /// Draw centered text
    pub fn draw_centered_text(&self, ctx: &mut dyn GraphicsContext, text: &str, y: u32) -> Result<()> {
        // In a real implementation with actual font rendering, we would measure text
        // For now, we'll just estimate: 8 pixels per character
        let text_width = text.len() as u32 * 8;
        let x = (self.width - text_width) / 2;
        
        // For simplicity, we'll draw a rectangle to represent text
        // In a real implementation, we would render actual text
        ctx.set_draw_color(self.text_color)?;
        ctx.draw_rect(Rectangle {
            x: x as i32,
            y: y as i32,
            width: text_width,
            height: 20,
        })?;
        
        // Draw a smaller filled rectangle to represent text
        ctx.fill_rect(Rectangle {
            x: (x + 2) as i32,
            y: (y + 2) as i32,
            width: text_width - 4,
            height: 16,
        })?;
        
        Ok(())
    }
    
    /// Draw right arrow
    pub fn draw_right_arrow(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        ctx.set_draw_color(Color::rgb(0, 0, 200))?; // Blue
        
        // Arrow position: right side, middle of screen
        let x = self.width as i32 - 30;
        let y = (self.height / 2) as i32;
        
        // Draw triangle
        ctx.draw_line(Point { x, y: y - 15 }, Point { x: x + 15, y })?;
        ctx.draw_line(Point { x: x + 15, y }, Point { x, y: y + 15 })?;
        ctx.draw_line(Point { x, y: y + 15 }, Point { x, y: y - 15 })?;
        
        Ok(())
    }
    
    /// Draw left arrow
    pub fn draw_left_arrow(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        ctx.set_draw_color(Color::rgb(0, 0, 200))?; // Blue
        
        // Arrow position: left side, middle of screen
        let x = 30;
        let y = (self.height / 2) as i32;
        
        // Draw triangle
        ctx.draw_line(Point { x, y }, Point { x: x - 15, y: y - 15 })?;
        ctx.draw_line(Point { x: x - 15, y: y - 15 }, Point { x: x - 15, y: y + 15 })?;
        ctx.draw_line(Point { x: x - 15, y: y + 15 }, Point { x, y })?;
        
        Ok(())
    }
}

/// Implementation of the Page trait for transition pages
impl Page for TransitionPage {
    fn init(&mut self) -> Result<()> {
        // Basic initialization
        Ok(())
    }
    
    fn render(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        // Draw page content
        self.draw_content(ctx)
    }
    
    fn handle_event(&mut self, event: &Event) -> Result<Option<String>> {
        // Handle transition events
        match event {
            Event::Custom { event_type, .. } => {
                match event_type.as_str() {
                    "next_page" => {
                        return Ok(self.handle_transition(TransitionDirection::Right));
                    },
                    "prev_page" => {
                        return Ok(self.handle_transition(TransitionDirection::Left));
                    },
                    _ => {}
                }
            },
            Event::Touch { x, y, action } => {
                if *action == TouchAction::Release {
                    // Check if touch is in the right side of the screen (right 1/3)
                    if *x > ((self.width as i32) * 2 / 3) {
                        return Ok(self.handle_transition(TransitionDirection::Right));
                    }
                    // Check if touch is in the left side of the screen (left 1/3)
                    else if *x < ((self.width as i32) / 3) {
                        return Ok(self.handle_transition(TransitionDirection::Left));
                    }
                }
            },
            _ => {}
        }
        
        Ok(None)
    }
    
    fn on_activate(&mut self) -> Result<()> {
        self.is_active = true;
        Ok(())
    }
    
    fn on_deactivate(&mut self) -> Result<()> {
        self.is_active = false;
        Ok(())
    }
    
    fn update_layout(&mut self, width: u32, height: u32) -> Result<()> {
        self.width = width;
        self.height = height;
        Ok(())
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}