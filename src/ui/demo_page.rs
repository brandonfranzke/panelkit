//! Demo page with LVGL widgets for testing
//!
//! This module provides a simple demo page with buttons, labels, and sliders.

use crate::event::Event;
use crate::ui::Page;
use anyhow::Result;
use lvgl::{Align, LabelLongMode, ButtonPart, SliderMode, Screen, Part, Style, StyleProp};
use lvgl::widgets::{Label, Button, Slider};
use std::cell::RefCell;
use std::rc::Rc;

/// A demo page with various LVGL widgets
pub struct DemoPage {
    screen: Screen,
    title: Option<Label>,
    counter: usize,
    counter_label: Option<Label>,
    button: Option<Button>,
    slider: Option<Slider>,
}

impl DemoPage {
    /// Create a new demo page
    pub fn new() -> Self {
        Self {
            screen: Screen::new(),
            title: None,
            counter: 0,
            counter_label: None,
            button: None,
            slider: None,
        }
    }
    
    /// Update the counter display
    fn update_counter(&mut self) {
        if let Some(label) = &self.counter_label {
            label.set_text(&format!("Counter: {}", self.counter));
        }
    }
}

impl Page for DemoPage {
    fn init(&mut self) -> Result<()> {
        // Create a title label
        let title = Label::create(&self.screen);
        title.set_text("PanelKit Demo");
        title.set_align(Align::TopMid, 0, 10);
        title.set_long_mode(LabelLongMode::Break);
        title.set_width(400);
        
        // Add a custom style to the title
        let title_style = Style::new();
        title_style.set_text_color(lvgl::Color::from_rgb((0, 0, 255)));
        title_style.set_text_font(&lvgl::Font::montserrat_28());
        title.add_style(Part::Main, &title_style);
        
        // Create a counter label
        let counter_label = Label::create(&self.screen);
        counter_label.set_text("Counter: 0");
        counter_label.set_align(Align::TopMid, 0, 70);
        counter_label.set_long_mode(LabelLongMode::Break);
        counter_label.set_width(200);
        
        // Create a button
        let button = Button::create(&self.screen);
        button.set_align(Align::Center, 0, 0);
        button.set_size(150, 50);
        
        // Add a label to the button
        let button_label = Label::create(&button);
        button_label.set_text("Click Me!");
        button_label.set_align(Align::Center, 0, 0);
        
        // Add custom style to the button
        let button_style = Style::new();
        button_style.set_bg_color(lvgl::Color::from_rgb((48, 169, 255)));
        button_style.set_border_width(2);
        button_style.set_border_color(lvgl::Color::from_rgb((0, 72, 255)));
        button_style.set_radius(10);
        button_style.set_shadow_width(5);
        button_style.set_shadow_ofs_y(5);
        button_style.set_shadow_opa(lvgl::Opacity::OPA70);
        button.add_style(ButtonPart::Main, &button_style);
        
        // Create a slider
        let slider = Slider::create(&self.screen);
        slider.set_align(Align::BottomMid, 0, -20);
        slider.set_size(300, 20);
        slider.set_mode(SliderMode::Normal);
        slider.set_range(0, 100);
        slider.set_value(50, lvgl::Anim::Off);
        
        // Store the widgets
        self.title = Some(title);
        self.counter_label = Some(counter_label);
        self.button = Some(button);
        self.slider = Some(slider);
        
        // Set up button event handler
        let counter_rc = Rc::new(RefCell::new(&mut self.counter));
        let counter_label_rc = Rc::new(RefCell::new(&mut self.counter_label));
        
        button.on_event(lvgl::Event::Clicked, move || {
            // Update counter
            let mut counter = counter_rc.borrow_mut();
            **counter += 1;
            
            // Update label
            if let Some(ref label) = **counter_label_rc.borrow() {
                label.set_text(&format!("Counter: {}", **counter));
            }
        });
        
        log::info!("DemoPage initialized");
        Ok(())
    }
    
    fn render(&self) -> Result<()> {
        // LVGL handles its own rendering
        Ok(())
    }
    
    fn handle_event(&mut self, event: &Event) -> Result<()> {
        match event {
            Event::Touch { action, .. } => {
                log::info!("DemoPage received touch event: {:?}", action);
                // LVGL handles its own touch events via the input driver
            }
            _ => {}
        }
        
        Ok(())
    }
    
    fn on_activate(&mut self) -> Result<()> {
        log::info!("DemoPage activated");
        self.screen.load();
        Ok(())
    }
    
    fn on_deactivate(&mut self) -> Result<()> {
        log::info!("DemoPage deactivated");
        Ok(())
    }
}