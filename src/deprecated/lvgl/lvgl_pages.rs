//! LVGL page implementations for Hello/World demo
//!
//! This module provides LVGL-based implementations of UI pages.

use crate::error::Result;
use crate::event::Event;
use crate::platform::{Color, GraphicsContext};
use crate::ui::Page;
use lvgl::core::{Object, Screen};
use lvgl::style::{Opacity, Style};
use lvgl::widgets::{Label, Arc};
use lvgl::{Align, Coord, LvResult};
use std::any::Any;
use std::cell::RefCell;
use std::rc::Rc;

/// LVGL implementation of Hello page
pub struct HelloPage {
    /// Page title
    title: String,
    /// LVGL screen
    screen: Option<Rc<RefCell<Screen>>>,
    /// Is the page active
    is_active: bool,
    /// Screen dimensions
    width: u32,
    height: u32,
}

impl HelloPage {
    /// Create a new Hello page
    pub fn new() -> Self {
        Self {
            title: "Hello".to_string(),
            screen: None,
            is_active: false,
            width: 320,
            height: 240,
        }
    }
    
    /// Create page content with LVGL widgets
    fn create_content(&self, screen: &Screen) -> Result<()> {
        // Use a fallback approach similar to the simple_ui example
        // to ensure we have something that works even if full LVGL integration
        // still has issues
        
        // Create a label for "Hello" text
        let label = Label::new(Some(screen));
        
        // Position in center
        label.set_align(Align::Center, 0, 0);
        
        // Set text
        label.set_text("Hello");
        
        // Style the label
        let style = Style::default();
        style.set_text_color(lvgl::Color::from_rgb(0, 0, 0));
        
        // Apply style
        label.add_style(lvgl::Part::Main, &style);
        
        // Create right arrow indicator
        self.create_right_arrow(screen)?;
        
        Ok(())
    }
    
    /// Create right arrow navigation indicator
    fn create_right_arrow(&self, screen: &Screen) -> Result<()> {
        use lvgl::widgets::Line;
        
        // Create a line object for arrow
        let line = Line::new(Some(screen));
        
        // Arrow at right side of screen
        let arrow_x = (self.width - 50) as i16;
        let arrow_y = (self.height / 2) as i16;
        
        // Define arrow points
        let points = [
            lvgl::Point::new(Coord::new(arrow_x), Coord::new(arrow_y - 20)),
            lvgl::Point::new(Coord::new(arrow_x + 30), Coord::new(arrow_y)),
            lvgl::Point::new(Coord::new(arrow_x), Coord::new(arrow_y + 20)),
            lvgl::Point::new(Coord::new(arrow_x), Coord::new(arrow_y - 20)), // Close the triangle
        ];
        
        // Set points
        line.set_points(&points);
        
        // Style for blue arrow
        let style = Style::default();
        style.set_line_color(lvgl::Color::from_rgb(0, 0, 200));
        style.set_line_width(2);
        
        // Apply style
        line.add_style(lvgl::Part::Main, &style);
        
        Ok(())
    }
}

impl Page for HelloPage {
    fn init(&mut self) -> Result<()> {
        // Create new screen if needed
        if self.screen.is_none() {
            self.screen = Some(Rc::new(RefCell::new(Screen::default())));
            
            // Set light blue background
            let bg_style = Style::default();
            bg_style.set_bg_color(lvgl::Color::from_rgb(240, 240, 255));
            bg_style.set_bg_opa(Opacity::COVER);
            
            if let Some(screen_rc) = &self.screen {
                let screen = screen_rc.borrow();
                screen.add_style(lvgl::Part::Main, &bg_style);
                
                // Create content
                self.create_content(&screen)?;
            }
        }
        
        Ok(())
    }
    
    fn render(&self, _ctx: &mut dyn GraphicsContext) -> Result<()> {
        // LVGL handles rendering automatically, we don't need to do anything here
        Ok(())
    }
    
    fn handle_event(&mut self, event: &Event) -> Result<Option<String>> {
        // Handle navigation events
        match event {
            Event::Custom { event_type, .. } => {
                if event_type == "next_page" {
                    return Ok(Some("world".to_string()));
                }
            },
            Event::Touch { x, y, action } => {
                // Check if touch is in right third of screen
                if *x > (self.width as i32 * 2 / 3) {
                    return Ok(Some("world".to_string()));
                }
            },
            _ => {}
        }
        
        Ok(None)
    }
    
    fn on_activate(&mut self) -> Result<()> {
        self.is_active = true;
        
        // Load screen
        if let Some(screen_rc) = &self.screen {
            let screen = screen_rc.borrow();
            screen.load();
        }
        
        Ok(())
    }
    
    fn on_deactivate(&mut self) -> Result<()> {
        self.is_active = false;
        Ok(())
    }
    
    fn update_layout(&mut self, width: u32, height: u32) -> Result<()> {
        self.width = width;
        self.height = height;
        
        // If screen exists, update layout
        if let Some(screen_rc) = &self.screen {
            // Reinitialize content with new dimensions
            let screen = screen_rc.borrow();
            // Clear existing content
            screen.clean();
            // Recreate content with new dimensions
            self.create_content(&screen)?;
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

/// LVGL implementation of World page
pub struct WorldPage {
    /// Page title
    title: String,
    /// LVGL screen
    screen: Option<Rc<RefCell<Screen>>>,
    /// Is the page active
    is_active: bool,
    /// Screen dimensions
    width: u32,
    height: u32,
}

impl WorldPage {
    /// Create a new World page
    pub fn new() -> Self {
        Self {
            title: "World".to_string(),
            screen: None,
            is_active: false,
            width: 320,
            height: 240,
        }
    }
    
    /// Create page content with LVGL widgets
    fn create_content(&self, screen: &Screen) -> Result<()> {
        // Create a label for "World" text
        let label = Label::new(Some(screen));
        
        // Position in center
        label.set_align(Align::Center, 0, 0);
        
        // Set text
        label.set_text("World");
        
        // Style the label
        let style = Style::default();
        style.set_text_color(lvgl::Color::from_rgb(0, 0, 0));
        
        // Apply style
        label.add_style(lvgl::Part::Main, &style);
        
        // Create left arrow indicator
        self.create_left_arrow(screen)?;
        
        Ok(())
    }
    
    /// Create left arrow navigation indicator
    fn create_left_arrow(&self, screen: &Screen) -> Result<()> {
        use lvgl::widgets::Line;
        
        // Create a line object for arrow
        let line = Line::new(Some(screen));
        
        // Arrow at left side of screen
        let arrow_x = 50;
        let arrow_y = (self.height / 2) as i16;
        
        // Define arrow points
        let points = [
            lvgl::Point::new(Coord::new(arrow_x), Coord::new(arrow_y - 20)),
            lvgl::Point::new(Coord::new(arrow_x - 30), Coord::new(arrow_y)),
            lvgl::Point::new(Coord::new(arrow_x), Coord::new(arrow_y + 20)),
            lvgl::Point::new(Coord::new(arrow_x), Coord::new(arrow_y - 20)), // Close the triangle
        ];
        
        // Set points
        line.set_points(&points);
        
        // Style for blue arrow
        let style = Style::default();
        style.set_line_color(lvgl::Color::from_rgb(0, 0, 200));
        style.set_line_width(2);
        
        // Apply style
        line.add_style(lvgl::Part::Main, &style);
        
        Ok(())
    }
}

impl Page for WorldPage {
    fn init(&mut self) -> Result<()> {
        // Create new screen if needed
        if self.screen.is_none() {
            self.screen = Some(Rc::new(RefCell::new(Screen::default())));
            
            // Set light green background
            let bg_style = Style::default();
            bg_style.set_bg_color(lvgl::Color::from_rgb(240, 255, 240));
            bg_style.set_bg_opa(Opacity::COVER);
            
            if let Some(screen_rc) = &self.screen {
                let screen = screen_rc.borrow();
                screen.add_style(lvgl::Part::Main, &bg_style);
                
                // Create content
                self.create_content(&screen)?;
            }
        }
        
        Ok(())
    }
    
    fn render(&self, _ctx: &mut dyn GraphicsContext) -> Result<()> {
        // LVGL handles rendering automatically
        Ok(())
    }
    
    fn handle_event(&mut self, event: &Event) -> Result<Option<String>> {
        // Handle navigation events
        match event {
            Event::Custom { event_type, .. } => {
                if event_type == "prev_page" {
                    return Ok(Some("hello".to_string()));
                }
            },
            Event::Touch { x, y, action } => {
                // Check if touch is in left third of screen
                if *x < (self.width as i32 / 3) {
                    return Ok(Some("hello".to_string()));
                }
            },
            _ => {}
        }
        
        Ok(None)
    }
    
    fn on_activate(&mut self) -> Result<()> {
        self.is_active = true;
        
        // Load screen
        if let Some(screen_rc) = &self.screen {
            let screen = screen_rc.borrow();
            screen.load();
        }
        
        Ok(())
    }
    
    fn on_deactivate(&mut self) -> Result<()> {
        self.is_active = false;
        Ok(())
    }
    
    fn update_layout(&mut self, width: u32, height: u32) -> Result<()> {
        self.width = width;
        self.height = height;
        
        // If screen exists, update layout
        if let Some(screen_rc) = &self.screen {
            // Reinitialize content with new dimensions
            let screen = screen_rc.borrow();
            // Clear existing content
            screen.clean();
            // Recreate content with new dimensions
            self.create_content(&screen)?;
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