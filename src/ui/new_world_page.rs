//! World page implementation using the new rendering abstraction
//!
//! This module provides a "World" page that uses the rendering abstraction layer.

use crate::event::Event;
use crate::ui::Page;
use crate::platform::{GraphicsContext, Point, Rectangle, Color, TextStyle, FontSize, TextAlignment};
use crate::logging;
use crate::rendering::RenderingBackend;
use crate::error::Result;
use std::any::Any;

/// World page using the new rendering abstraction
pub struct NewWorldPage {
    width: u32,
    height: u32,
    title_area: Rectangle,
    content_area: Rectangle,
    button_area: Rectangle,
    left_arrow_points: [Point; 3],
    logger: &'static logging::ComponentLogger,
}

impl NewWorldPage {
    /// Create a new World page
    pub fn new() -> Self {
        let width = 800;
        let height = 480;
        
        // Define layout areas (will be updated in update_layout)
        let title_area = Rectangle::new(0, 0, width, 70);
        let content_area = Rectangle::new(0, 70, width, height - 140);
        let button_area = Rectangle::new(20, height as i32 - 70, 130, 50);
        
        // Left arrow for navigation
        let left_arrow_points = [
            Point::new(80, height as i32 / 2),
            Point::new(110, height as i32 / 2 - 30),
            Point::new(110, height as i32 / 2 + 30),
        ];
        
        Self {
            width,
            height,
            title_area,
            content_area,
            button_area,
            left_arrow_points,
            logger: logging::ui_logger(),
        }
    }
    
    /// Draw title bar
    fn draw_title_bar(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        // Draw title background
        ctx.clear(Color::ui_background())?;
        
        // Draw title bar background
        let mut driver_ctx = match ctx.as_any_mut().downcast_mut::<crate::platform::rendering_driver::RenderingGraphicsContext>() {
            Some(ctx) => ctx,
            None => {
                self.logger.warn("Failed to downcast to RenderingGraphicsContext - falling back to legacy rendering");
                
                // Fall back to legacy rendering if we can't get the new context
                return self.draw_title_bar_legacy(ctx);
            }
        };
        
        // Get the backend from the context
        if let Some(backend) = driver_ctx.backend.as_any_mut().downcast_mut::<crate::rendering::sdl_backend::SDLBackend>() {
            // Draw title area
            backend.fill_rect(self.title_area, Color::rgb(30, 90, 120))?;
            
            // Draw title text
            let title_position = Point::new(
                self.title_area.x + (self.title_area.width as i32 / 2),
                self.title_area.y + (self.title_area.height as i32 / 2) - 10
            );
            
            let title_style = TextStyle::new(Color::white())
                .with_size(FontSize::Large)
                .with_alignment(TextAlignment::Center);
                
            backend.draw_text("World Page", title_position, title_style)?;
        }
        
        Ok(())
    }
    
    /// Draw title bar using legacy rendering (fallback)
    fn draw_title_bar_legacy(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        // This is a simplified version for compatibility with legacy GraphicsContext
        ctx.fill_rect(self.title_area)?;
        Ok(())
    }
    
    /// Draw content area
    fn draw_content(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        let mut driver_ctx = match ctx.as_any_mut().downcast_mut::<crate::platform::rendering_driver::RenderingGraphicsContext>() {
            Some(ctx) => ctx,
            None => {
                self.logger.warn("Failed to downcast to RenderingGraphicsContext - falling back to legacy rendering");
                
                // Fall back to legacy rendering if we can't get the new context
                return self.draw_content_legacy(ctx);
            }
        };
        
        // Get the backend from the context
        if let Some(backend) = driver_ctx.backend.as_any_mut().downcast_mut::<crate::rendering::sdl_backend::SDLBackend>() {
            // Draw world text
            let content_position = Point::new(
                self.content_area.x + (self.content_area.width as i32 / 2),
                self.content_area.y + (self.content_area.height as i32 / 2) - 20
            );
            
            let content_style = TextStyle::new(Color::rgb(30, 30, 30))
                .with_size(FontSize::ExtraLarge)
                .with_alignment(TextAlignment::Center)
                .with_bold(true);
                
            backend.draw_text("World", content_position, content_style)?;
            
            // Draw navigation arrow
            let arrow_color = Color::rgb(50, 100, 200);
            
            // Draw filled triangle for arrow
            for i in 0..3 {
                let j = (i + 1) % 3;
                backend.draw_line(self.left_arrow_points[i], self.left_arrow_points[j], arrow_color)?;
            }
            
            // Draw "back" button
            backend.draw_button(
                self.button_area,
                "← Back",
                Color::rgb(30, 90, 120),
                Color::white(),
                Color::rgb(20, 60, 80)
            )?;
        }
        
        Ok(())
    }
    
    /// Draw content using legacy rendering (fallback)
    fn draw_content_legacy(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        // This is a simplified version for compatibility with legacy GraphicsContext
        Ok(())
    }
}

impl Page for NewWorldPage {
    fn init(&mut self) -> Result<()> {
        self.logger.info("Initializing NewWorldPage");
        
        // Update layout
        self.update_layout(self.width, self.height)?;
        
        Ok(())
    }
    
    fn render(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        self.logger.trace("Rendering NewWorldPage");
        
        // Draw title bar
        self.draw_title_bar(ctx)?;
        
        // Draw main content
        self.draw_content(ctx)?;
        
        self.logger.trace("NewWorldPage rendering complete");
        Ok(())
    }
    
    fn handle_event(&mut self, event: &Event) -> Result<Option<String>> {
        match event {
            Event::Touch { x, y, action } => {
                // Check if the user clicked the back button
                if self.button_area.contains(&Point::new(*x, *y)) {
                    self.logger.info("Back button clicked, navigating to Hello page");
                    return Ok(Some("hello".to_string()));
                }
                
                // Check if the user clicked the arrow
                let arrow_bounds = Rectangle::new(
                    80 - 25,
                    self.height as i32 / 2 - 40,
                    50,
                    80
                );
                
                if arrow_bounds.contains(&Point::new(*x, *y)) {
                    self.logger.info("Navigation arrow clicked, navigating to Hello page");
                    return Ok(Some("hello".to_string()));
                }
            },
            Event::Custom { event_type, .. } => {
                if event_type == "prev_page" {
                    self.logger.info("Received prev_page event, navigating to Hello page");
                    return Ok(Some("hello".to_string()));
                }
            },
            _ => {}
        }
        
        Ok(None)
    }
    
    fn on_activate(&mut self) -> Result<()> {
        self.logger.info("NewWorldPage activated");
        Ok(())
    }
    
    fn on_deactivate(&mut self) -> Result<()> {
        self.logger.info("NewWorldPage deactivated");
        Ok(())
    }
    
    fn update_layout(&mut self, width: u32, height: u32) -> Result<()> {
        self.width = width;
        self.height = height;
        
        // Update layout areas
        self.title_area = Rectangle::new(0, 0, width, 70);
        self.content_area = Rectangle::new(0, 70, width, height - 140);
        self.button_area = Rectangle::new(20, height as i32 - 70, 130, 50);
        
        // Update left arrow for navigation
        self.left_arrow_points = [
            Point::new(80, height as i32 / 2),
            Point::new(110, height as i32 / 2 - 30),
            Point::new(110, height as i32 / 2 + 30),
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