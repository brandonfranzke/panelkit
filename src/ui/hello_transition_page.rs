//! Hello Transition Page implementation
//!
//! This module provides the "Hello" page using the transition page framework.

use crate::error::Result;
use crate::platform::{Color, GraphicsContext, Point, Rectangle, Renderable};
use crate::ui::transition_page::TransitionPage;
use crate::ui::Page;
use std::any::Any;

/// The Hello page using transition support
pub struct HelloTransitionPage {
    /// Base transition page
    pub base: TransitionPage,
}

impl HelloTransitionPage {
    /// Create a new Hello page
    pub fn new() -> Self {
        let mut base = TransitionPage::new("Hello");
        
        // Configure navigation
        base.set_next_page("world");
        
        // Set custom rendering function
        base.render_fn = Some(Box::new(|page, ctx| {
            // Clear with light background
            ctx.clear(Color::rgb(240, 240, 255))?; // Light blue tint
            
            // Draw centered "Hello" text
            ctx.set_draw_color(Color::rgb(0, 0, 0))?;
            
            // Draw the word "Hello"
            // Since we don't have text rendering, we'll draw a word box
            let text_width = 60; // "Hello" ~ 5 chars * ~12px
            let x = (page.width - text_width) / 2;
            let y = page.height / 2 - 15;
            
            // Draw text box
            ctx.fill_rect(Rectangle {
                x: x as i32,
                y: y as i32,
                width: text_width,
                height: 30,
            })?;
            
            // Draw right navigation indicator
            if page.has_next {
                ctx.set_draw_color(Color::rgb(0, 0, 200))?; // Blue
                
                // Arrow position: right side, middle of screen
                let arrow_x = page.width as i32 - 30;
                let arrow_y = (page.height / 2) as i32;
                
                // Draw triangle
                ctx.draw_line(
                    Point { x: arrow_x, y: arrow_y - 15 }, 
                    Point { x: arrow_x + 15, y: arrow_y }
                )?;
                ctx.draw_line(
                    Point { x: arrow_x + 15, y: arrow_y }, 
                    Point { x: arrow_x, y: arrow_y + 15 }
                )?;
                ctx.draw_line(
                    Point { x: arrow_x, y: arrow_y + 15 }, 
                    Point { x: arrow_x, y: arrow_y - 15 }
                )?;
                
                // Fill triangle
                // Simplified since we don't have a fill_triangle primitive
                for i in 0..15 {
                    ctx.draw_line(
                        Point { x: arrow_x + i, y: arrow_y - (15 - i) }, 
                        Point { x: arrow_x + i, y: arrow_y + (15 - i) }
                    )?;
                }
            }
            
            Ok(())
        }));
        
        Self { base }
    }
}

/// Implement the Page trait by delegating to the base page
impl Page for HelloTransitionPage {
    fn init(&mut self) -> Result<()> {
        self.base.init()
    }
    
    fn render(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        self.base.render(ctx)
    }
    
    fn handle_event(&mut self, event: &crate::event::Event) -> Result<Option<String>> {
        self.base.handle_event(event)
    }
    
    fn on_activate(&mut self) -> Result<()> {
        self.base.on_activate()
    }
    
    fn on_deactivate(&mut self) -> Result<()> {
        self.base.on_deactivate()
    }
    
    fn update_layout(&mut self, width: u32, height: u32) -> Result<()> {
        self.base.update_layout(width, height)
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}