//! World Transition Page implementation
//!
//! This module provides the "World" page using the transition page framework.

use crate::error::Result;
use crate::platform::{Color, GraphicsContext, Point, Rectangle, Renderable};
use crate::ui::transition_page::TransitionPage;
use crate::ui::Page;
use std::any::Any;

/// The World page using transition support
pub struct WorldTransitionPage {
    /// Base transition page
    pub base: TransitionPage,
}

impl WorldTransitionPage {
    /// Create a new World page
    pub fn new() -> Self {
        let mut base = TransitionPage::new("World");
        
        // Configure navigation
        base.set_prev_page("hello");
        
        // Set custom rendering function
        base.render_fn = Some(Box::new(|page, ctx| {
            // Clear with light background
            ctx.clear(Color::rgb(240, 255, 240))?; // Light green tint
            
            // Draw centered "World" text
            ctx.set_draw_color(Color::rgb(0, 0, 0))?;
            
            // Draw the word "World"
            // Since we don't have text rendering, we'll draw a word box
            let text_width = 60; // "World" ~ 5 chars * ~12px
            let x = (page.width - text_width) / 2;
            let y = page.height / 2 - 15;
            
            // Draw text box
            ctx.fill_rect(Rectangle {
                x: x as i32,
                y: y as i32,
                width: text_width,
                height: 30,
            })?;
            
            // Draw left navigation indicator
            if page.has_prev {
                ctx.set_draw_color(Color::rgb(0, 0, 200))?; // Blue
                
                // Arrow position: left side, middle of screen
                let arrow_x = 30;
                let arrow_y = (page.height / 2) as i32;
                
                // Draw triangle
                ctx.draw_line(
                    Point { x: arrow_x, y: arrow_y }, 
                    Point { x: arrow_x - 15, y: arrow_y - 15 }
                )?;
                ctx.draw_line(
                    Point { x: arrow_x - 15, y: arrow_y - 15 }, 
                    Point { x: arrow_x - 15, y: arrow_y + 15 }
                )?;
                ctx.draw_line(
                    Point { x: arrow_x - 15, y: arrow_y + 15 }, 
                    Point { x: arrow_x, y: arrow_y }
                )?;
                
                // Fill triangle
                // Simplified since we don't have a fill_triangle primitive
                for i in 0..15 {
                    ctx.draw_line(
                        Point { x: arrow_x - i, y: arrow_y - i }, 
                        Point { x: arrow_x - i, y: arrow_y + i }
                    )?;
                }
            }
            
            Ok(())
        }));
        
        Self { base }
    }
}

/// Implement the Page trait by delegating to the base page
impl Page for WorldTransitionPage {
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