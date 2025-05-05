//! Layout components for UI
//!
//! This module provides layout elements for arranging components.

use crate::primitives::{RenderingContext, Rectangle, Color};
use crate::ui::components::{ColoredRectangle, ComponentBase};
use crate::ui::traits::{Component, Positioned, Contains, Renderable, Container as ContainerTrait};
use crate::event::{Event, TouchEvent, KeyboardEvent};
use anyhow::Result;

/// A container for grouping multiple UI components
pub struct Container {
    base: ComponentBase,
    components: Vec<Box<dyn Component>>,
    background: Option<ColoredRectangle>,
    focused: bool,
    focused_index: Option<usize>,
}

impl Container {
    /// Create a new container
    pub fn new(x: i32, y: i32, width: u32, height: u32) -> Self {
        Self {
            base: ComponentBase::with_bounds(x, y, width, height),
            components: Vec::new(),
            background: None,
            focused: false,
            focused_index: None,
        }
    }
    
    /// Set the background color of the container
    pub fn with_background(mut self, color_rect: ColoredRectangle) -> Self {
        self.background = Some(color_rect);
        self
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

impl ContainerTrait for Container {
    fn add_child<T: Component + 'static>(&mut self, child: T) {
        self.components.push(Box::new(child));
    }
    
    fn children(&self) -> &[Box<dyn Component>] {
        &self.components
    }
    
    fn children_mut(&mut self) -> &mut [Box<dyn Component>] {
        &mut self.components
    }
    
    fn propagate_to_children(&mut self, event: &mut dyn Event) -> Result<bool> {
        // Set the event phase to capturing first
        if let Some(event_data) = event.as_any_mut().downcast_mut::<crate::event::EventData>() {
            event_data.set_phase(crate::event::EventPhase::Capturing);
        }

        // Capturing phase: propagate down from parent to children
        // Process in reverse order (top components get events first)
        let children = self.children_mut();
        let children_len = children.len();
        
        // Find the target component (the one that contains the point for touch events)
        let mut target_index = None;
        if let Some(touch_event) = event.as_any_mut().downcast_mut::<crate::event::TouchEvent>() {
            for i in (0..children_len).rev() {
                let child = &children[i];
                if child.is_visible() && child.is_enabled() && child.contains(&touch_event.position) {
                    target_index = Some(i);
                    break;
                }
            }
        }
        
        // Capturing phase: propagate to all ancestors of the target
        let mut handled = false;
        if let Some(target_idx) = target_index {
            // Process all components before the target in reverse order (capturing)
            for i in (0..target_idx).rev() {
                let child = &mut children[i];
                if child.is_visible() && child.is_enabled() {
                    handled = child.handle_event(event)? || event.is_handled();
                    if handled {
                        return Ok(true);
                    }
                }
            }
            
            // At target phase
            if let Some(event_data) = event.as_any_mut().downcast_mut::<crate::event::EventData>() {
                event_data.set_phase(crate::event::EventPhase::AtTarget);
            }
            
            let target = &mut children[target_idx];
            if target.is_visible() && target.is_enabled() {
                handled = target.handle_event(event)? || event.is_handled();
                if handled {
                    return Ok(true);
                }
            }
            
            // Bubbling phase
            if let Some(event_data) = event.as_any_mut().downcast_mut::<crate::event::EventData>() {
                event_data.set_phase(crate::event::EventPhase::Bubbling);
            }
            
            // Process all components after the target
            for i in (target_idx+1)..children_len {
                let child = &mut children[i];
                if child.is_visible() && child.is_enabled() {
                    handled = child.handle_event(event)? || event.is_handled();
                    if handled {
                        return Ok(true);
                    }
                }
            }
            
            return Ok(handled);
        } else {
            // No target found, just try all components in reverse order
            for i in (0..children_len).rev() {
                let child = &mut children[i];
                if child.is_visible() && child.is_enabled() {
                    handled = child.handle_event(event)? || event.is_handled();
                    if handled {
                        return Ok(true);
                    }
                }
            }
        }
        
        Ok(handled)
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
    
    fn is_focused(&self) -> bool {
        self.focused
    }
    
    fn set_focused(&mut self, focused: bool) {
        self.focused = focused;
        
        // If this container is focused, make sure the currently focused child is also focused
        if let Some(index) = self.focused_index {
            if index < self.components.len() {
                for (i, child) in self.components.iter_mut().enumerate() {
                    child.set_focused(focused && i == index);
                }
            }
        }
    }
    
    // Override event handler methods to propagate to children
    fn on_touch_down(&mut self, event: &mut TouchEvent) -> Result<bool> {
        self.propagate_to_children(event)
    }
    
    fn on_touch_move(&mut self, event: &mut TouchEvent) -> Result<bool> {
        self.propagate_to_children(event)
    }
    
    fn on_touch_up(&mut self, event: &mut TouchEvent) -> Result<bool> {
        self.propagate_to_children(event)
    }
    
    fn on_touch_long_press(&mut self, event: &mut TouchEvent) -> Result<bool> {
        self.propagate_to_children(event)
    }
    
    fn on_gesture(&mut self, event: &mut TouchEvent) -> Result<bool> {
        self.propagate_to_children(event)
    }
    
    fn on_key_event(&mut self, event: &mut KeyboardEvent) -> Result<bool> {
        // If we have a focused child, delegate to it
        if let Some(index) = self.focused_index {
            if index < self.components.len() {
                return self.components[index].on_key_event(event);
            }
        }
        
        // Otherwise, try all children in order
        self.propagate_to_children(event)
    }
    
    fn process_event(&mut self, event: &mut dyn Event) -> Result<bool> {
        // First check component's own event processing logic
        let base_handled = match event.event_type() {
            crate::event::EventType::Touch => {
                if let Some(touch_event) = event.as_any_mut().downcast_mut::<crate::event::TouchEvent>() {
                    match touch_event.action {
                        crate::event::TouchAction::Down => {
                            if self.contains(&touch_event.position) {
                                self.on_touch_down(touch_event)?
                            } else {
                                false
                            }
                        },
                        crate::event::TouchAction::Move => self.on_touch_move(touch_event)?,
                        crate::event::TouchAction::Up => self.on_touch_up(touch_event)?,
                        crate::event::TouchAction::LongPress => {
                            if self.contains(&touch_event.position) {
                                self.on_touch_long_press(touch_event)?
                            } else {
                                false
                            }
                        },
                        crate::event::TouchAction::Gesture(_) => {
                            if self.contains(&touch_event.position) {
                                self.on_gesture(touch_event)?
                            } else {
                                false
                            }
                        }
                    }
                } else {
                    false
                }
            },
            crate::event::EventType::Keyboard => {
                if let Some(key_event) = event.as_any_mut().downcast_mut::<crate::event::KeyboardEvent>() {
                    if self.is_focused() {
                        self.on_key_event(key_event)?
                    } else {
                        false
                    }
                } else {
                    false
                }
            },
            _ => false
        };
        
        if base_handled || event.is_handled() {
            return Ok(true);
        }
        
        // Then try to propagate to children
        self.propagate_to_children(event)
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
        container.add_child(crate::ui::components::text::Text::new(
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
    
    fn is_focused(&self) -> bool {
        self.container.is_focused()
    }
    
    fn set_focused(&mut self, focused: bool) {
        self.container.set_focused(focused);
    }
    
    // Forward touch events to container
    fn on_touch_down(&mut self, event: &mut TouchEvent) -> Result<bool> {
        self.container.on_touch_down(event)
    }
    
    fn on_touch_move(&mut self, event: &mut TouchEvent) -> Result<bool> {
        self.container.on_touch_move(event)
    }
    
    fn on_touch_up(&mut self, event: &mut TouchEvent) -> Result<bool> {
        self.container.on_touch_up(event)
    }
    
    fn on_touch_long_press(&mut self, event: &mut TouchEvent) -> Result<bool> {
        self.container.on_touch_long_press(event)
    }
    
    fn on_gesture(&mut self, event: &mut TouchEvent) -> Result<bool> {
        self.container.on_gesture(event)
    }
    
    fn on_key_event(&mut self, event: &mut KeyboardEvent) -> Result<bool> {
        self.container.on_key_event(event)
    }
}

// Container trait already implemented above