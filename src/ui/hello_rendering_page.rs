//! Hello page implementation
//!
//! This module provides a "Hello" page for the application.

use crate::event::{Event, TouchEvent, EventType, CustomEvent, TouchAction, LegacyEvent};
use crate::ui::Page;
use crate::logging;
use crate::primitives::{RenderingContext, Point, Rectangle, Color, TextStyle, FontSize, TextAlignment};
use anyhow::Result; 
use std::any::Any;

/// Hello page implementation
pub struct HelloRenderingPage {
    width: u32,
    height: u32,
    title_area: Rectangle,
    content_area: Rectangle,
    button_area: Rectangle,
    right_arrow_points: [Point; 3],
    logger: &'static logging::ComponentLogger,
}

impl HelloRenderingPage {
    /// Create a Hello page
    pub fn new() -> Self {
        let width = 800;
        let height = 480;
        
        // Define layout areas (will be updated in update_layout)
        let title_area = Rectangle::new(0, 0, width, 70);
        let content_area = Rectangle::new(0, 70, width, height - 140);
        let button_area = Rectangle::new(width as i32 - 150, height as i32 - 70, 130, 50);
        
        // Right arrow for navigation
        let right_arrow_points = [
            Point::new(width as i32 - 80, height as i32 / 2),
            Point::new(width as i32 - 110, height as i32 / 2 - 30),
            Point::new(width as i32 - 110, height as i32 / 2 + 30),
        ];
        
        Self {
            width,
            height,
            title_area,
            content_area,
            button_area,
            right_arrow_points,
            logger: logging::ui_logger(),
        }
    }
    
    /// Draw title bar
    fn draw_title_bar(&self, ctx: &mut dyn RenderingContext) -> Result<()> {
        // Draw title background
        ctx.clear(Color::rgb(240, 240, 240))?;
        
        // Draw title area
        ctx.fill_rect(self.title_area, Color::rgb(30, 120, 60))?;
        
        // Draw title text
        let title_position = Point::new(
            self.title_area.x + (self.title_area.width as i32 / 2),
            self.title_area.y + (self.title_area.height as i32 / 2) - 10
        );
        
        let title_style = TextStyle {
            font_size: FontSize::Large,
            color: Color::rgb(255, 255, 255),
            alignment: TextAlignment::Center,
            bold: false,
            italic: false,
        };
                
        ctx.draw_text("Hello Page", title_position, title_style)?;
        
        Ok(())
    }
    
    /// Draw content area
    fn draw_content(&self, ctx: &mut dyn RenderingContext) -> Result<()> {
        // Draw hello text
        let content_position = Point::new(
            self.content_area.x + (self.content_area.width as i32 / 2),
            self.content_area.y + (self.content_area.height as i32 / 2) - 20
        );
        
        let content_style = TextStyle {
            font_size: FontSize::Large,
            color: Color::rgb(30, 30, 30),
            alignment: TextAlignment::Center,
            bold: false,
            italic: false,
        };
                
        ctx.draw_text("Hello", content_position, content_style)?;
        
        // Draw navigation arrow
        let arrow_color = Color::rgb(50, 100, 200);
        
        // Draw filled triangle for arrow
        for i in 0..3 {
            let j = (i + 1) % 3;
            let start = self.right_arrow_points[i];
            let end = self.right_arrow_points[j];
            ctx.draw_line(start, end, arrow_color)?;
        }
        
        // Draw "next" button
        ctx.draw_button(
            self.button_area,
            "Next →",
            Color::rgb(30, 120, 60),
            Color::rgb(255, 255, 255),
            Color::rgb(20, 80, 40)
        )?;
        
        Ok(())
    }
}

impl Page for HelloRenderingPage {
    fn init(&mut self) -> Result<()> {
        self.logger.info("Initializing HelloRenderingPage");
        
        // Update layout
        self.update_layout(self.width, self.height)?;
        
        Ok(())
    }
    
    fn render(&self, ctx: &mut dyn RenderingContext) -> Result<()> {
        self.logger.trace("Rendering HelloRenderingPage");
        
        // Draw title bar
        self.draw_title_bar(ctx)?;
        
        // Draw main content
        self.draw_content(ctx)?;
        
        self.logger.trace("HelloRenderingPage rendering complete");
        Ok(())
    }
    
    #[deprecated(
        since = "0.2.0",
        note = "MIGRATION REQUIRED: Use handle_new_event instead. Will be removed in version 0.3.0."
    )]
    fn handle_event(&mut self, event: &crate::event::LegacyEvent) -> Result<Option<String>> {
        // Forward to new_event handler by converting to the new event type
        // This avoids duplicating logic and ensures both handlers work the same
        let mut new_event = crate::event::convert_legacy_event(event);
        self.handle_new_event(&mut *new_event)
    }
    
    fn on_activate(&mut self) -> Result<()> {
        self.logger.info("HelloRenderingPage activated");
        Ok(())
    }
    
    fn on_deactivate(&mut self) -> Result<()> {
        self.logger.info("HelloRenderingPage deactivated");
        Ok(())
    }
    
    fn update_layout(&mut self, width: u32, height: u32) -> Result<()> {
        self.width = width;
        self.height = height;
        
        // Update layout areas
        self.title_area = Rectangle::new(0, 0, width, 70);
        self.content_area = Rectangle::new(0, 70, width, height - 140);
        self.button_area = Rectangle::new(width as i32 - 150, height as i32 - 70, 130, 50);
        
        // Update right arrow for navigation
        self.right_arrow_points = [
            Point::new(width as i32 - 80, height as i32 / 2),
            Point::new(width as i32 - 110, height as i32 / 2 - 30),
            Point::new(width as i32 - 110, height as i32 / 2 + 30),
        ];
        
        self.logger.debug(&format!("Updated layout to {}x{}", width, height));
        
        Ok(())
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
    
    fn handle_new_event(&mut self, event: &mut dyn NewEvent) -> Result<Option<String>> {
        match event.event_type() {
            EventType::Touch => {
                // Downcast to TouchEvent
                if let Some(touch_event) = event.as_any_mut().downcast_mut::<TouchEvent>() {
                    // Only handle TouchAction::Down events (equivalent to legacy Press)
                    if touch_event.action == TouchAction::Down {
                        let position = touch_event.position;
                        
                        // Check if the user clicked the next button
                        if self.button_area.contains(&position) {
                            self.logger.info("Next button clicked, navigating to World page");
                            touch_event.mark_handled();
                            return Ok(Some("world".to_string()));
                        }
                        
                        // Check if the user clicked the arrow
                        let arrow_bounds = Rectangle::new(
                            self.width as i32 - 130,
                            self.height as i32 / 2 - 40,
                            50,
                            80
                        );
                        
                        if arrow_bounds.contains(&position) {
                            self.logger.info("Navigation arrow clicked, navigating to World page");
                            touch_event.mark_handled();
                            return Ok(Some("world".to_string()));
                        }
                    }
                }
            },
            EventType::Custom => {
                // Downcast to CustomEvent
                if let Some(custom_event) = event.as_any_mut().downcast_mut::<CustomEvent>() {
                    if custom_event.name == "next_page" {
                        self.logger.info("Received next_page event, navigating to World page");
                        custom_event.mark_handled();
                        return Ok(Some("world".to_string()));
                    }
                }
            },
            _ => {}
        }
        
        Ok(None)
    }
}