//! Core UI trait definitions
//!
//! This module defines the core traits for UI components and behaviors.

use crate::primitives::{RenderingContext, Rectangle, Point};
use crate::event::{
    Event, TouchEvent, KeyboardEvent, EventType, TouchAction, EventHandler
};
use anyhow::Result;

/// Trait for objects that can be positioned on screen
pub trait Positioned {
    /// Get the bounding rectangle of this object
    fn bounds(&self) -> Rectangle;
    
    /// Get the position (top-left corner)
    fn position(&self) -> Point {
        let bounds = self.bounds();
        Point::new(bounds.x, bounds.y)
    }
    
    /// Get the dimensions (width and height)
    fn dimensions(&self) -> (u32, u32) {
        let bounds = self.bounds();
        (bounds.width, bounds.height)
    }
}

/// Trait for objects that can determine if they contain a point
pub trait Contains: Positioned {
    /// Check if the object contains the given point
    fn contains(&self, point: &Point) -> bool {
        self.bounds().contains(point)
    }
}

/// Trait for objects that can be rendered
pub trait Renderable {
    /// Render this object to the given rendering context
    fn render(&self, ctx: &mut dyn RenderingContext) -> Result<()>;
}

/// Core trait for UI components
/// 
/// UI Components are objects that:
/// 1. Have a position and dimensions (Positioned)
/// 2. Can determine if they contain a point (Contains)
/// 3. Can be rendered (Renderable)
/// 4. Can be interacted with
pub trait Component: Positioned + Contains + Renderable + EventHandler {
    /// Set whether the component is enabled (can receive input)
    fn set_enabled(&mut self, enabled: bool);
    
    /// Check if the component is enabled
    fn is_enabled(&self) -> bool;
    
    /// Set whether the component is visible
    fn set_visible(&mut self, visible: bool);
    
    /// Check if the component is visible
    fn is_visible(&self) -> bool;

    /// Process an event specifically for this component
    fn process_event(&mut self, event: &mut dyn Event) -> Result<bool> {
        if !self.is_enabled() || !self.is_visible() {
            return Ok(false);
        }

        match event.event_type() {
            EventType::Touch => {
                if let Some(touch_event) = event.as_any_mut().downcast_mut::<TouchEvent>() {
                    match touch_event.action {
                        TouchAction::Down => {
                            if self.contains(&touch_event.position) {
                                let result = self.on_touch_down(touch_event)?;
                                if result {
                                    touch_event.mark_handled();
                                }
                                return Ok(result);
                            }
                        },
                        TouchAction::Move => {
                            let result = self.on_touch_move(touch_event)?;
                            if result {
                                touch_event.mark_handled();
                            }
                            return Ok(result);
                        },
                        TouchAction::Up => {
                            let result = self.on_touch_up(touch_event)?;
                            if result {
                                touch_event.mark_handled();
                            }
                            return Ok(result);
                        },
                        TouchAction::LongPress => {
                            if self.contains(&touch_event.position) {
                                let result = self.on_touch_long_press(touch_event)?;
                                if result {
                                    touch_event.mark_handled();
                                }
                                return Ok(result);
                            }
                        },
                        TouchAction::Gesture(_) => {
                            if self.contains(&touch_event.position) {
                                let result = self.on_gesture(touch_event)?;
                                if result {
                                    touch_event.mark_handled();
                                }
                                return Ok(result);
                            }
                        }
                    }
                }
            },
            EventType::Keyboard => {
                if let Some(key_event) = event.as_any_mut().downcast_mut::<KeyboardEvent>() {
                    if self.is_focused() {
                        let result = self.on_key_event(key_event)?;
                        if result {
                            key_event.mark_handled();
                        }
                        return Ok(result);
                    }
                }
            },
            _ => {}
        }
        
        Ok(false)
    }

    /// Handle a touch down event
    fn on_touch_down(&mut self, _event: &mut TouchEvent) -> Result<bool> {
        Ok(false)
    }
    
    /// Handle a touch move event
    fn on_touch_move(&mut self, _event: &mut TouchEvent) -> Result<bool> {
        Ok(false)
    }
    
    /// Handle a touch up event
    fn on_touch_up(&mut self, _event: &mut TouchEvent) -> Result<bool> {
        Ok(false)
    }
    
    /// Handle a long press event
    fn on_touch_long_press(&mut self, _event: &mut TouchEvent) -> Result<bool> {
        Ok(false)
    }
    
    /// Handle a gesture event
    fn on_gesture(&mut self, _event: &mut TouchEvent) -> Result<bool> {
        Ok(false)
    }
    
    /// Handle a key event
    fn on_key_event(&mut self, _event: &mut KeyboardEvent) -> Result<bool> {
        Ok(false)
    }
    
    /// Check if the component has focus
    fn is_focused(&self) -> bool {
        false
    }
    
    /// Set focus state
    fn set_focused(&mut self, _focused: bool) { }
}

/// Implementation of EventHandler for all Components
impl<T: Component> EventHandler for T {
    fn handle_event(&mut self, event: &mut dyn Event) -> Result<bool> {
        self.process_event(event)
    }
}

/// A container of components that can have children
pub trait Container: Component {
    /// Add a child component
    fn add_child<T: Component + 'static>(&mut self, child: T);
    
    /// Access all children
    fn children(&self) -> &[Box<dyn Component>];
    
    /// Access all children mutably
    fn children_mut(&mut self) -> &mut [Box<dyn Component>];
    
    /// Propagate events to children
    /// 
    /// This method should implement the event propagation phases:
    /// 1. Capturing: From container down to target (parent to child)
    /// 2. At Target: Processing at the target element
    /// 3. Bubbling: From target up to container (child to parent)
    fn propagate_to_children(&mut self, event: &mut dyn Event) -> Result<bool>;
}