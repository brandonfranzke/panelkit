//! Layout components for UI
//!
//! This module provides layout elements for arranging components.

use crate::primitives::{RenderingContext, Rectangle, Color, Point};
use crate::ui::components::{ColoredRectangle, ComponentBase};
use crate::ui::traits::{Component, Positioned, Contains, Renderable};
use anyhow::Result;

/// A container for grouping multiple UI components
pub struct Container {
    base: ComponentBase,
    components: Vec<Box<dyn Component>>,
    background: Option<ColoredRectangle>,
}

impl Container {
    /// Create a new container
    pub fn new(x: i32, y: i32, width: u32, height: u32) -> Self {
        Self {
            base: ComponentBase::with_bounds(x, y, width, height),
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
    pub fn add<T: Component + 'static>(&mut self, component: T) {
        self.components.push(Box::new(component));
    }
}

impl Positioned for Container {
    fn bounds(&self) -> Rectangle {
        self.base.bounds()
    }
}

impl Contains for Container {}

impl Renderable for Container {
    fn render(&self, ctx: &mut dyn RenderingContext) -> Result<()> {
        if !self.is_visible() {
            return Ok(());
        }
        
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

impl Component for Container {
    fn set_enabled(&mut self, enabled: bool) {
        self.base.enabled = enabled;
        
        // Propagate to children
        for component in &mut self.components {
            component.set_enabled(enabled);
        }
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
}

/// A title bar component
pub struct TitleBar {
    container: Container,
}

impl TitleBar {
    /// Create a new title bar
    pub fn new(x: i32, y: i32, width: u32, height: u32, title: &str, text_color: Color, bg_color: Color) -> Self {
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

impl Positioned for TitleBar {
    fn bounds(&self) -> Rectangle {
        self.container.bounds()
    }
}

impl Contains for TitleBar {}

impl Renderable for TitleBar {
    fn render(&self, ctx: &mut dyn RenderingContext) -> Result<()> {
        if !self.is_visible() {
            return Ok(());
        }
        
        self.container.render(ctx)
    }
}

impl Component for TitleBar {
    fn set_enabled(&mut self, enabled: bool) {
        self.container.set_enabled(enabled);
    }
    
    fn is_enabled(&self) -> bool {
        self.container.is_enabled()
    }
    
    fn set_visible(&mut self, visible: bool) {
        self.container.set_visible(visible);
    }
    
    fn is_visible(&self) -> bool {
        self.container.is_visible()
    }
}