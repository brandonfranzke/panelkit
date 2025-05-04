//! World page implementation using the rendering abstraction
//!
//! This module provides a "World" page that uses the rendering abstraction layer.

use crate::event::Event;
use crate::ui::Page;
use crate::platform::GraphicsContext;
use crate::platform::graphics::{Point, Rectangle, Color};
use crate::logging;
use crate::rendering::RenderingBackend;
use crate::ui::compat;
use anyhow::Result;
use std::any::Any;

/// World page using the rendering abstraction
pub struct WorldRenderingPage {
    width: u32,
    height: u32,
    title_area: Rectangle,
    content_area: Rectangle,
    button_area: Rectangle,
    left_arrow_points: [Point; 3],
    logger: &'static logging::ComponentLogger,
}

impl WorldRenderingPage {
    /// Create a World page
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
        let driver_ctx = match ctx.as_any_mut().downcast_mut::<crate::platform::rendering_driver::RenderingGraphicsContext>() {
            Some(ctx) => ctx,
            None => {
                self.logger.warn("Failed to downcast to RenderingGraphicsContext - falling back to standard rendering");
                
                // Fall back to standard rendering if we can't get the rendering context
                return self.draw_title_bar_fallback(ctx);
            }
        };
        
        // Get the backend from the context
        if let Some(backend) = driver_ctx.backend.as_any_mut().downcast_mut::<crate::rendering::sdl_backend::SDLBackend>() {
            // Create rendering primitives
            use crate::rendering::primitives;
            
            // Draw title area
            let rendering_rect = primitives::Rectangle::from_graphics(self.title_area);
            let rendering_color = primitives::Color::rgb(30, 90, 120);
            backend.fill_rect(rendering_rect, rendering_color)?;
            
            // Draw title text
            let title_position = Point::new(
                self.title_area.x + (self.title_area.width as i32 / 2),
                self.title_area.y + (self.title_area.height as i32 / 2) - 10
            );
            let rendering_position = primitives::Point::from_graphics(title_position);
            
            // Create a TextStyle using rendering primitives version
            let rendering_style = primitives::TextStyle::new(primitives::Color::white())
                .with_size(primitives::FontSize::Large)
                .with_alignment(primitives::TextAlignment::Center);
                
            backend.draw_text("World Page", rendering_position, rendering_style)?;
        }
        
        Ok(())
    }
    
    /// Draw title bar using standard rendering (fallback)
    fn draw_title_bar_fallback(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        // This is a simplified version for compatibility with standard GraphicsContext
        ctx.fill_rect(self.title_area)?;
        Ok(())
    }
    
    /// Draw content area
    fn draw_content(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        let driver_ctx = match ctx.as_any_mut().downcast_mut::<crate::platform::rendering_driver::RenderingGraphicsContext>() {
            Some(ctx) => ctx,
            None => {
                self.logger.warn("Failed to downcast to RenderingGraphicsContext - falling back to standard rendering");
                
                // Fall back to standard rendering if we can't get the rendering context
                return self.draw_content_fallback(ctx);
            }
        };
        
        // Get the backend from the context
        if let Some(backend) = driver_ctx.backend.as_any_mut().downcast_mut::<crate::rendering::sdl_backend::SDLBackend>() {
            // Create rendering primitives
            use crate::rendering::primitives;
            
            // Draw world text
            let content_position = Point::new(
                self.content_area.x + (self.content_area.width as i32 / 2),
                self.content_area.y + (self.content_area.height as i32 / 2) - 20
            );
            let rendering_position = primitives::Point::from_graphics(content_position);
            
            // Convert TextStyle to rendering primitives version
            let rendering_style = primitives::TextStyle::new(primitives::Color::rgb(30, 30, 30))
                .with_size(primitives::FontSize::ExtraLarge)
                .with_alignment(primitives::TextAlignment::Center)
                .with_bold(true);
                
            backend.draw_text("World", rendering_position, rendering_style)?;
            
            // Draw navigation arrow
            let rendering_arrow_color = primitives::Color::rgb(50, 100, 200);
            
            // Draw filled triangle for arrow
            for i in 0..3 {
                let j = (i + 1) % 3;
                // Convert platform Point to rendering Point using our helper
                let start_point = compat::point_to_rendering(&self.left_arrow_points[i]);
                let end_point = compat::point_to_rendering(&self.left_arrow_points[j]);
                backend.draw_line(start_point, end_point, rendering_arrow_color)?;
            }
            
            // Draw "back" button
            // Convert platform Rectangle to rendering Rectangle using our helper
            let rendering_button_rect = compat::rect_to_rendering(&self.button_area);
            let rendering_bg_color = primitives::Color::rgb(30, 90, 120);
            let rendering_text_color = primitives::Color::white();
            let rendering_border_color = primitives::Color::rgb(20, 60, 80);
            
            backend.draw_button(
                rendering_button_rect,
                "← Back",
                rendering_bg_color,
                rendering_text_color,
                rendering_border_color
            )?;
        }
        
        Ok(())
    }
    
    /// Draw content using standard rendering (fallback)
    fn draw_content_fallback(&self, _ctx: &mut dyn GraphicsContext) -> Result<()> {
        // This is a simplified version for compatibility with standard GraphicsContext
        Ok(())
    }
}

impl Page for WorldRenderingPage {
    fn init(&mut self) -> Result<()> {
        self.logger.info("Initializing WorldRenderingPage");
        
        // Update layout
        self.update_layout(self.width, self.height)?;
        
        Ok(())
    }
    
    fn render(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        self.logger.trace("Rendering WorldRenderingPage");
        
        // Draw title bar
        self.draw_title_bar(ctx)?;
        
        // Draw main content
        self.draw_content(ctx)?;
        
        self.logger.trace("WorldRenderingPage rendering complete");
        Ok(())
    }
    
    fn handle_event(&mut self, event: &Event) -> Result<Option<String>> {
        match event {
            Event::Touch { x, y, action: _ } => {
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
        self.logger.info("WorldRenderingPage activated");
        Ok(())
    }
    
    fn on_deactivate(&mut self) -> Result<()> {
        self.logger.info("WorldRenderingPage deactivated");
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