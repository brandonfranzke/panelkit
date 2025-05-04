//! Hello Page implementation with LVGL
//!
//! This module provides the "Hello" page for the PanelKit demo.

use crate::error::Result;
use crate::event::Event;
use crate::platform::{GraphicsContext, Renderable};
use crate::ui::lvgl_page::LvglPage;
use crate::ui::Page;
use lvgl::core::{Display, Object};
use lvgl::widgets::{Label, Arc};
use lvgl::{Align, Color, State};
use std::any::Any;

/// The Hello page implementation
pub struct HelloLvglPage {
    /// Base LVGL page
    base: LvglPage,
    
    /// Label for hello text
    hello_label: Option<Label>,
    
    /// Right arrow indicator
    right_indicator: Option<Arc>,
}

impl HelloLvglPage {
    /// Create a new Hello page
    pub fn new() -> Self {
        Self {
            base: LvglPage::new("Hello"),
            hello_label: None,
            right_indicator: None,
        }
    }
    
    /// Create the hello label
    fn create_hello_label(&mut self) -> Result<()> {
        let screen = self.base.get_screen();
        
        // Create label
        let label = Label::new(Some(&screen.borrow()));
        
        // Set text
        label.set_text("Hello");
        
        // Center the label
        label.set_align(Align::Center, 0, 0);
        
        // Set style - make it large and visible
        let style = label.get_style().unwrap_or_default();
        style.set_text_color(Color::from_rgb(0, 0, 0)); // Black text
        style.set_text_font(None); // Default font
        
        // Apply style
        label.add_style(&style, State::DEFAULT);
        
        // Store label
        self.hello_label = Some(label);
        
        Ok(())
    }
    
    /// Create the right arrow indicator
    fn create_right_indicator(&mut self) -> Result<()> {
        let screen = self.base.get_screen();
        
        // Create arc widget as a simple indicator
        let arc = Arc::new(Some(&screen.borrow()));
        
        // Set size
        arc.set_size(50, 50);
        
        // Position on right side of screen
        let (width, _) = (self.base.width, self.base.height);
        arc.set_pos((width as i16) - 60, (self.base.height as i16) / 2 - 25);
        
        // Set arc properties
        arc.set_angles(0, 270); // Create a partial circle as an arrow
        arc.set_rotation(90); // Rotate to point right
        
        // Set style
        let style = arc.get_style().unwrap_or_default();
        style.set_line_color(Color::from_rgb(0, 0, 255)); // Blue
        style.set_line_width(2); // 2px width
        
        // Apply style
        arc.add_style(&style, State::DEFAULT);
        
        // Store indicator
        self.right_indicator = Some(arc);
        
        Ok(())
    }
}

/// Implement the Page trait
impl Page for HelloLvglPage {
    fn init(&mut self) -> Result<()> {
        // Initialize the base page
        self.base.init()?;
        
        // Create UI elements
        self.create_hello_label()?;
        self.create_right_indicator()?;
        
        Ok(())
    }
    
    fn render(&self, ctx: &mut dyn GraphicsContext) -> Result<()> {
        // LVGL handles rendering automatically, just pass through to base page
        self.base.render(ctx)
    }
    
    fn handle_event(&mut self, event: &Event) -> Result<Option<String>> {
        // Handle navigation events
        match event {
            Event::Custom { event_type, .. } => {
                if event_type == "next_page" {
                    // Navigate to world page
                    return Ok(Some("world".to_string()));
                }
            },
            Event::Touch { x, y, action } => {
                // Check if touch is in the right side of the screen
                if *x > (self.base.width as i32) / 2 {
                    // Navigate to world page on right side touch
                    return Ok(Some("world".to_string()));
                }
            },
            _ => {}
        }
        
        // Pass to base page for default handling
        self.base.handle_event(event)
    }
    
    fn on_activate(&mut self) -> Result<()> {
        // Activate base page
        self.base.on_activate()
    }
    
    fn on_deactivate(&mut self) -> Result<()> {
        // Deactivate base page
        self.base.on_deactivate()
    }
    
    fn update_layout(&mut self, width: u32, height: u32) -> Result<()> {
        // Update base page dimensions
        self.base.update_layout(width, height)?;
        
        // Update label position (center)
        if let Some(label) = &self.hello_label {
            label.set_align(Align::Center, 0, 0);
        }
        
        // Update indicator position (right side)
        if let Some(arc) = &self.right_indicator {
            arc.set_pos((width as i16) - 60, (height as i16) / 2 - 25);
        }
        
        Ok(())
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}