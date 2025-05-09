//! SDL2-based display and input driver
//!
//! This module provides SDL2 implementation for the PanelKit platform interface.

use crate::event::{Event, TouchEvent, KeyboardEvent, CustomEvent, SystemEvent, TouchAction, SystemEventType};
use crate::platform::PlatformDriver;
use crate::primitives::{Color, Point, Rectangle, RenderingContext, TextStyle, Surface};

use anyhow::Result;
use sdl2::event::Event as SdlEvent;
use sdl2::keyboard::Keycode;
use sdl2::mouse::MouseButton;
use std::sync::{Arc, Mutex};
use std::time::Duration;
use std::fmt::Display;
use std::any::Any;

/// SDL2 rendering context that implements the RenderingContext trait
pub struct SDLRenderingContext {
    canvas: Arc<Mutex<sdl2::render::Canvas<sdl2::video::Window>>>,
    width: u32,
    height: u32,
}

impl SDLRenderingContext {
    /// Create a new SDL rendering context
    pub fn new(canvas: Arc<Mutex<sdl2::render::Canvas<sdl2::video::Window>>>, width: u32, height: u32) -> Self {
        log::debug!("Creating new SDLRenderingContext with canvas");
        Self { canvas, width, height }
    }
}

impl RenderingContext for SDLRenderingContext {
    fn init(&mut self, width: u32, height: u32) -> Result<()> {
        self.width = width;
        self.height = height;
        
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
        
        // Try to resize the window (this may not work on all platforms)
        let window = canvas.window_mut();
        window.set_size(width, height)
            .map_err(|e| anyhow::anyhow!("Failed to resize SDL window: {}", e))?;
        
        Ok(())
    }
    
    fn present(&mut self) -> Result<()> {
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
        
        canvas.present();
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn cleanup(&mut self) {
        // SDL cleanup happens in the driver, not here
        log::debug!("SDL rendering context cleanup");
    }
    
    fn clear(&mut self, color: Color) -> Result<()> {
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
            
        canvas.set_draw_color(sdl2::pixels::Color::RGBA(color.r, color.g, color.b, color.a));
        canvas.clear();
        Ok(())
    }
    
    fn fill_rect(&mut self, rect: Rectangle, color: Color) -> Result<()> {
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
        
        canvas.set_draw_color(sdl2::pixels::Color::RGBA(color.r, color.g, color.b, color.a));
        
        SDLDriver::convert_sdl_error(
            canvas.fill_rect(sdl2::rect::Rect::new(rect.x, rect.y, rect.width, rect.height)),
            "Failed to fill rectangle"
        )?;
            
        Ok(())
    }
    
    fn draw_rect(&mut self, rect: Rectangle, color: Color) -> Result<()> {
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
        
        canvas.set_draw_color(sdl2::pixels::Color::RGBA(color.r, color.g, color.b, color.a));
            
        SDLDriver::convert_sdl_error(
            canvas.draw_rect(sdl2::rect::Rect::new(rect.x, rect.y, rect.width, rect.height)),
            "Failed to draw rectangle outline"
        )?;
            
        Ok(())
    }
    
    fn draw_line(&mut self, start: Point, end: Point, color: Color) -> Result<()> {
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
        
        canvas.set_draw_color(sdl2::pixels::Color::RGBA(color.r, color.g, color.b, color.a));
            
        SDLDriver::convert_sdl_error(
            canvas.draw_line(
                sdl2::rect::Point::new(start.x, start.y),
                sdl2::rect::Point::new(end.x, end.y)
            ),
            "Failed to draw line"
        )?;
        
        Ok(())
    }
    
    fn draw_text(&mut self, _text: &str, _position: Point, _style: TextStyle) -> Result<()> {
        // Not implemented in basic SDL
        log::warn!("Text rendering not implemented in SDL driver");
        Ok(())
    }
    
    fn draw_button(&mut self, rect: Rectangle, _text: &str, bg_color: Color, _text_color: Color, border_color: Color) -> Result<()> {
        // Simple implementation - draw filled rectangle with border
        self.fill_rect(rect, bg_color)?;
        self.draw_rect(rect, border_color)?;
        // No text rendering in this simple implementation
        Ok(())
    }
    
    fn create_surface(&mut self, _width: u32, _height: u32) -> Result<Box<dyn Surface>> {
        anyhow::bail!("Surface creation not implemented in SDL driver")
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}

/// SDL2-based platform driver
pub struct SDLDriver {
    sdl_context: sdl2::Sdl,
    canvas: Arc<Mutex<sdl2::render::Canvas<sdl2::video::Window>>>,
    width: u32,
    height: u32,
    last_touch_point: (i32, i32),
    is_pressed: bool,
}

impl SDLDriver {
    /// Helper function to convert SDL errors to our error type
    fn convert_sdl_error<T, E: Display>(result: std::result::Result<T, E>, context: &str) -> Result<T> {
        result.map_err(|e| {
            anyhow::anyhow!("{}: {}", context, e)
        })
    }
    
    /// Create a new SDL driver
    pub fn new(width: u32, height: u32, title: &str) -> Result<Self> {
        let sdl_context = Self::convert_sdl_error(
            sdl2::init(),
            "Failed to initialize SDL2"
        )?;
            
        let video_subsystem = Self::convert_sdl_error(
            sdl_context.video(),
            "Failed to initialize SDL2 video subsystem"
        )?;

        // Use different SDL window settings for macOS vs other platforms
        #[cfg(target_os = "macos")]
        let window = Self::convert_sdl_error(
            video_subsystem
                .window(title, width, height)
                .position_centered()
                .allow_highdpi() // Enable Retina display support
                .build(),
            "Failed to create SDL2 window on macOS"
        )?;

        #[cfg(not(target_os = "macos"))]
        let window = Self::convert_sdl_error(
            video_subsystem
                .window(title, width, height)
                .position_centered()
                .build(),
            "Failed to create SDL2 window"
        )?;

        let mut canvas = Self::convert_sdl_error(
            window
                .into_canvas()
                .accelerated()
                .present_vsync() // Use vsync for smoother rendering
                .build(),
            "Failed to create SDL2 canvas"
        )?;
            
        // Initial color to verify it's working
        canvas.set_draw_color(sdl2::pixels::Color::RGB(100, 100, 100));
        canvas.clear();
        canvas.present();

        let canvas_arc = Arc::new(Mutex::new(canvas));

        Ok(Self {
            sdl_context,
            canvas: canvas_arc,
            width,
            height,
            last_touch_point: (0, 0),
            is_pressed: false,
        })
    }

    /// Update touch state based on SDL events
    fn update_touch(&mut self, x: i32, y: i32, pressed: bool) {
        self.last_touch_point = (x, y);
        self.is_pressed = pressed;
    }
}

// Implement the unified PlatformDriver trait
impl PlatformDriver for SDLDriver {
    fn init(&mut self, width: u32, height: u32) -> Result<()> {
        log::info!("SDL platform initialized with dimensions: {}x{}", width, height);
        
        // Update dimensions if provided
        if width > 0 && height > 0 {
            self.width = width;
            self.height = height;
        }
        
        // Initialize with gray background
        let mut canvas = self.canvas.lock().unwrap();
        canvas.set_draw_color(sdl2::pixels::Color::RGB(100, 100, 100));
        canvas.clear();
        canvas.present();
        
        Ok(())
    }
    
    fn poll_events(&mut self) -> Result<Vec<Box<dyn Event>>> {
        let mut events: Vec<Box<dyn Event>> = Vec::new();
        let mut event_pump = self.sdl_context.event_pump()
            .map_err(|e| anyhow::anyhow!("Failed to get SDL event pump: {}", e))?;
        
        for sdl_event in event_pump.poll_iter() {
            match sdl_event {
                SdlEvent::Quit {..} | 
                SdlEvent::KeyDown { keycode: Some(Keycode::Escape), .. } => {
                    events.push(Box::new(CustomEvent::new("quit", "")));
                },
                SdlEvent::KeyDown { keycode: Some(Keycode::Right), .. } => {
                    events.push(Box::new(CustomEvent::new("next_page", "")));
                },
                SdlEvent::KeyDown { keycode: Some(Keycode::Left), .. } => {
                    events.push(Box::new(CustomEvent::new("prev_page", "")));
                },
                SdlEvent::KeyDown { keycode: Some(keycode), .. } => {
                    // For other key down events, use KeyboardEvent
                    let key_code = keycode as u32;
                    events.push(Box::new(KeyboardEvent::new(key_code, true)));
                },
                SdlEvent::KeyUp { keycode: Some(keycode), .. } => {
                    // Key up events
                    let key_code = keycode as u32;
                    events.push(Box::new(KeyboardEvent::new(key_code, false)));
                },
                SdlEvent::MouseButtonDown { mouse_btn: MouseButton::Left, x, y, .. } => {
                    self.update_touch(x, y, true);
                    events.push(Box::new(TouchEvent::new(
                        TouchAction::Down,
                        Point::new(x, y)
                    )));
                },
                SdlEvent::MouseButtonUp { mouse_btn: MouseButton::Left, x, y, .. } => {
                    self.update_touch(x, y, false);
                    events.push(Box::new(TouchEvent::new(
                        TouchAction::Up,
                        Point::new(x, y)
                    )));
                },
                SdlEvent::MouseMotion { x, y, mousestate, .. } => {
                    if mousestate.left() {
                        let prev_point = Point::new(self.last_touch_point.0, self.last_touch_point.1);
                        self.update_touch(x, y, true);
                        
                        // Create a move event with previous position
                        let touch_event = TouchEvent::new_move(
                            Point::new(x, y),
                            prev_point
                        );
                        events.push(Box::new(touch_event));
                    }
                },
                SdlEvent::Window { win_event: sdl2::event::WindowEvent::Resized(w, h), .. } => {
                    // Handle window resize event
                    events.push(Box::new(SystemEvent::new(
                        SystemEventType::Resize {
                            width: w as u32,
                            height: h as u32,
                        }
                    )));
                },
                _ => {}
            }
        }
        
        // Small sleep to avoid using too much CPU
        std::thread::sleep(Duration::from_millis(10));
        
        Ok(events)
    }
    
    fn present(&mut self) -> Result<()> {
        // Present the canvas, showing any content rendered so far
        log::trace!("Presenting SDL canvas");
        
        let canvas_result = self.canvas.lock();
        
        if let Err(e) = &canvas_result {
            return Err(anyhow::anyhow!("Failed to lock SDL canvas mutex during present: {}", e));
        }
        
        let mut canvas = canvas_result.unwrap();
        canvas.present();
        
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn create_rendering_context(&mut self) -> Result<Box<dyn RenderingContext>> {
        // Create a new rendering context that shares the canvas
        let context = SDLRenderingContext::new(self.canvas.clone(), self.width, self.height);
        Ok(Box::new(context))
    }
    
    fn cleanup(&mut self) {
        log::info!("SDL platform cleaned up");
    }
}