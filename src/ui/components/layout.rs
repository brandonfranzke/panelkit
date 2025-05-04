//! Layout components for UI
//!
//! This module provides layout elements for arranging components.

use crate::platform::{GraphicsContext, Renderable};
use crate::platform::graphics::Rectangle;
use crate::ui::components::{ColoredRectangle, UIComponent};
use anyhow::Result;

/// A container for grouping multiple UI components
pub struct Container {
    bounds: Rectangle,
    components: Vec<Box<dyn UIComponent>>,
    background: Option<ColoredRectangle>,
}

impl Container {
    /// Create a new container
    pub fn new(x: i32, y: i32, width: u32, height: u32) -> Self {
        Self {
            bounds: Rectangle::new(x, y, width, height),
            components: Vec::new(),
            background: None,
        }
    }
    
    /// Set the background color of the container
    pub fn with_background(mut self, color_rect: ColoredRectangle) -> Self {
        self.background = Some(color_rect);
        self
    }
    
    /// Add a component to the container
    pub fn add<T: UIComponent + 'static>(&mut self, component: T) {
        self.components.push(Box::new(component));
    }
}

impl Renderable for Container {
    fn render(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        // Draw background if set
        if let Some(ref bg) = self.background {
            bg.render(ctx)?;
        }
        
        // Draw all contained components
        for component in &self.components {
            component.render(ctx)?;
        }
        
        Ok(())
    }
}

impl UIComponent for Container {
    fn bounds(&self) -> crate::platform::graphics::Rectangle {
        self.bounds
    }
}

/// A title bar component
pub struct TitleBar {
    container: Container,
}

impl TitleBar {
    /// Create a new title bar
    pub fn new(x: i32, y: i32, width: u32, height: u32, title: &str, text_color: crate::platform::graphics::Color, bg_color: crate::platform::graphics::Color) -> Self {
        let mut container = Container::new(x, y, width, height)
            .with_background(ColoredRectangle::filled(x, y, width, height, bg_color));
            
        // Add a title text component centered in the bar
        container.add(crate::ui::components::text::Text::new(
            x + (width as i32 / 2), 
            y + ((height as i32 - 16) / 2), 
            title,
            text_color
        ).with_align(crate::ui::components::text::TextAlign::Center));
        
        Self { container }
    }
}

impl Renderable for TitleBar {
    fn render(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        self.container.render(ctx)
    }
}

impl UIComponent for TitleBar {
    fn bounds(&self) -> crate::platform::graphics::Rectangle {
        self.container.bounds()
    }
}