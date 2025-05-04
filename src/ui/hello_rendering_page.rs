//! Hello page implementation using the rendering abstraction
//!
//! This module provides a "Hello" page that uses the rendering abstraction layer.

use crate::event::Event;
use crate::ui::Page;
use crate::platform::GraphicsContext;
use crate::platform::graphics::{Point, Rectangle, Color};
use crate::logging;
use crate::rendering::RenderingBackend;
use anyhow::Result; 
use std::any::Any;

/// Hello page using the rendering abstraction
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
    fn draw_title_bar(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        // Draw title background
        ctx.clear(Color::rgb(240, 240, 240))?;
        
        // Try to use rendering backend if available
        if let Some(driver_ctx) = ctx.as_any_mut().downcast_mut::<crate::platform::rendering_driver::RenderingGraphicsContext>() {
            // Get the backend from the context
            if let Some(backend) = driver_ctx.backend.as_any_mut().downcast_mut::<crate::rendering::sdl_backend::SDLBackend>() {
                // Create rendering primitives
                use crate::rendering::primitives;
                
                // Draw title area
                let rendering_rect = primitives::Rectangle::from_graphics(self.title_area);
                let rendering_color = primitives::Color::rgb(30, 120, 60);
                backend.fill_rect(rendering_rect, rendering_color)?;
                
                // Draw title text
                let title_position = primitives::Point::new(
                    self.title_area.x + (self.title_area.width as i32 / 2),
                    self.title_area.y + (self.title_area.height as i32 / 2) - 10
                );
                
                let title_style = primitives::TextStyle::new(primitives::Color::rgb(255, 255, 255))
                    .with_size(primitives::FontSize::Large)
                    .with_alignment(primitives::TextAlignment::Center);
                    
                backend.draw_text("Hello Page", title_position, title_style)?;
                
                return Ok(());
            }
        }
        
        // Fall back to standard GraphicsContext if rendering backend isn't available
        self.draw_title_bar_fallback(ctx)
    }
    
    /// Draw title bar using standard rendering (fallback)
    fn draw_title_bar_fallback(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        // This is a simplified version for compatibility with standard GraphicsContext
        ctx.fill_rect(self.title_area)?;
        Ok(())
    }
    
    /// Draw content area
    fn draw_content(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        // Try to use rendering backend if available
        if let Some(driver_ctx) = ctx.as_any_mut().downcast_mut::<crate::platform::rendering_driver::RenderingGraphicsContext>() {
            // Get the backend from the context
            if let Some(backend) = driver_ctx.backend.as_any_mut().downcast_mut::<crate::rendering::sdl_backend::SDLBackend>() {
                // Create rendering primitives
                use crate::rendering::primitives;
                
                // Draw hello text
                let content_position = primitives::Point::new(
                    self.content_area.x + (self.content_area.width as i32 / 2),
                    self.content_area.y + (self.content_area.height as i32 / 2) - 20
                );
                
                let content_style = primitives::TextStyle::new(primitives::Color::rgb(30, 30, 30))
                    .with_size(primitives::FontSize::ExtraLarge)
                    .with_alignment(primitives::TextAlignment::Center)
                    .with_bold(true);
                    
                backend.draw_text("Hello", content_position, content_style)?;
                
                // Draw navigation arrow
                let arrow_color = primitives::Color::rgb(50, 100, 200);
                
                // Draw filled triangle for arrow
                for i in 0..3 {
                    let j = (i + 1) % 3;
                    let start = primitives::Point::from_graphics(self.right_arrow_points[i]);
                    let end = primitives::Point::from_graphics(self.right_arrow_points[j]);
                    backend.draw_line(start, end, arrow_color)?;
                }
                
                // Draw "next" button
                let button_rect = primitives::Rectangle::from_graphics(self.button_area);
                backend.draw_button(
                    button_rect,
                    "Next →",
                    primitives::Color::rgb(30, 120, 60),
                    primitives::Color::rgb(255, 255, 255),
                    primitives::Color::rgb(20, 80, 40)
                )?;
                
                return Ok(());
            }
        }
        
        // Fall back to standard GraphicsContext if rendering backend isn't available
        self.draw_content_fallback(ctx)
    }
    
    /// Draw content using standard rendering (fallback)
    fn draw_content_fallback(&self, _ctx: &mut dyn GraphicsContext) -> Result<()> {
        // This is a simplified version for compatibility with standard GraphicsContext
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
    
    fn render(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        self.logger.trace("Rendering HelloRenderingPage");
        
        // Draw title bar
        self.draw_title_bar(ctx)?;
        
        // Draw main content
        self.draw_content(ctx)?;
        
        self.logger.trace("HelloRenderingPage rendering complete");
        Ok(())
    }
    
    fn handle_event(&mut self, event: &Event) -> Result<Option<String>> {
        match event {
            Event::Touch { x, y, action: _ } => {
                // Check if the user clicked the next button
                if self.button_area.contains(&Point::new(*x, *y)) {
                    self.logger.info("Next button clicked, navigating to World page");
                    return Ok(Some("world".to_string()));
                }
                
                // Check if the user clicked the arrow
                let arrow_bounds = Rectangle::new(
                    self.width as i32 - 130,
                    self.height as i32 / 2 - 40,
                    50,
                    80
                );
                
                if arrow_bounds.contains(&Point::new(*x, *y)) {
                    self.logger.info("Navigation arrow clicked, navigating to World page");
                    return Ok(Some("world".to_string()));
                }
            },
            Event::Custom { event_type, .. } => {
                if event_type == "next_page" {
                    self.logger.info("Received next_page event, navigating to World page");
                    return Ok(Some("world".to_string()));
                }
            },
            _ => {}
        }
        
        Ok(None)
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
}