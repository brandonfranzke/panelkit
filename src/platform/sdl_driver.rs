//! SDL2-based display and input driver
//!
//! This module provides SDL2 implementation for the PanelKit platform interface.

use crate::event::{Event, TouchAction};
use crate::platform::{GraphicsContext, PlatformDriver};

use anyhow::{Result, Context};
use sdl2::event::Event as SdlEvent;
use sdl2::keyboard::Keycode;
use sdl2::mouse::MouseButton;
use sdl2::pixels::Color;
use std::sync::{Arc, Mutex};
use std::time::Duration;
use std::any::Any;

/// SDL2 rendering context
pub struct SDLGraphicsContext {
    canvas: Arc<Mutex<sdl2::render::Canvas<sdl2::video::Window>>>,
}

impl SDLGraphicsContext {
    /// Create a new SDL graphics context
    pub fn new(canvas: Arc<Mutex<sdl2::render::Canvas<sdl2::video::Window>>>) -> Self {
        Self { canvas }
    }
    
    /// Get the canvas for rendering
    pub fn canvas(&self) -> Arc<Mutex<sdl2::render::Canvas<sdl2::video::Window>>> {
        self.canvas.clone()
    }
}

impl GraphicsContext for SDLGraphicsContext {
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
    graphics_context: SDLGraphicsContext,
    width: u32,
    height: u32,
    last_touch_point: (i32, i32),
    is_pressed: bool,
}

// We're not implementing Send for SDLDriver since SDL2 isn't thread-safe
// Instead, we'll modify the PlatformDriver trait to not require Send

impl SDLDriver {
    /// Create a new SDL driver
    pub fn new(width: u32, height: u32, title: &str) -> Result<Self> {
        let sdl_context = sdl2::init()
            .map_err(|e| anyhow::anyhow!("Failed to initialize SDL2: {}", e))?;
            
        let video_subsystem = sdl_context
            .video()
            .map_err(|e| anyhow::anyhow!("Failed to initialize SDL2 video subsystem: {}", e))?;

        // Use different SDL window settings for macOS vs other platforms
        #[cfg(target_os = "macos")]
        let window = video_subsystem
            .window("PanelKit - SDL2 Window - macOS", width, height)
            .position_centered()
            .allow_highdpi() // Enable Retina display support
            .build()
            .map_err(|e| anyhow::anyhow!("Failed to create SDL2 window on macOS: {}", e))?;

        #[cfg(not(target_os = "macos"))]
        let window = video_subsystem
            .window(title, width, height)
            .position_centered()
            .build()
            .map_err(|e| anyhow::anyhow!("Failed to create SDL2 window: {}", e))?;

        let canvas = window
            .into_canvas()
            .accelerated()
            .build()
            .map_err(|e| anyhow::anyhow!("Failed to create SDL2 canvas: {}", e))?;

        let canvas_arc = Arc::new(Mutex::new(canvas));
        let graphics_context = SDLGraphicsContext::new(canvas_arc.clone());

        Ok(Self {
            sdl_context,
            canvas: canvas_arc,
            graphics_context,
            width,
            height,
            last_touch_point: (0, 0),
            is_pressed: false,
        })
    }

    /// Update touch state based on SDL events
    pub fn update_touch(&mut self, x: i32, y: i32, pressed: bool) {
        self.last_touch_point = (x, y);
        self.is_pressed = pressed;
    }

    /// Get a reference to the canvas
    pub fn get_canvas(&self) -> Arc<Mutex<sdl2::render::Canvas<sdl2::video::Window>>> {
        self.canvas.clone()
    }

    /// Clear the screen with a specific color
    pub fn clear(&mut self, r: u8, g: u8, b: u8) -> Result<()> {
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
            
        canvas.set_draw_color(Color::RGB(r, g, b));
        canvas.clear();
        Ok(())
    }
}

// Implement the unified PlatformDriver trait
impl PlatformDriver for SDLDriver {
    fn init(&mut self, _width: u32, _height: u32) -> Result<()> {
        // Initialize SDL - set a bright red clear color to verify it's working
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
        
        // Set very bright red background for testing
        canvas.set_draw_color(sdl2::pixels::Color::RGB(255, 0, 0));
        canvas.clear();
        canvas.present();
        
        log::info!("SDL platform initialized with dimensions: {}x{} (RED BACKGROUND)", 
                  self.width, self.height);
        Ok(())
    }
    
    fn poll_events(&mut self) -> Result<Vec<Event>> {
        let mut events = Vec::new();
        let mut event_pump = self.sdl_context.event_pump()
            .map_err(|e| anyhow::anyhow!("Failed to get SDL event pump: {}", e))?;
        
        for sdl_event in event_pump.poll_iter() {
            match sdl_event {
                SdlEvent::Quit {..} | 
                SdlEvent::KeyDown { keycode: Some(Keycode::Escape), .. } => {
                    events.push(Event::Custom { 
                        event_type: "quit".to_string(), 
                        payload: "".to_string() 
                    });
                },
                SdlEvent::MouseButtonDown { mouse_btn: MouseButton::Left, x, y, .. } => {
                    self.update_touch(x, y, true);
                    events.push(Event::Touch { 
                        x, 
                        y, 
                        action: TouchAction::Press 
                    });
                },
                SdlEvent::MouseButtonUp { mouse_btn: MouseButton::Left, x, y, .. } => {
                    self.update_touch(x, y, false);
                    events.push(Event::Touch { 
                        x, 
                        y, 
                        action: TouchAction::Release 
                    });
                },
                SdlEvent::MouseMotion { x, y, mousestate, .. } => {
                    if mousestate.left() {
                        self.update_touch(x, y, true);
                        events.push(Event::Touch { 
                            x, 
                            y, 
                            action: TouchAction::Move 
                        });
                    }
                },
                _ => {}
            }
        }
        
        // Small sleep to avoid using too much CPU
        std::thread::sleep(Duration::from_millis(10));
        
        Ok(events)
    }
    
    fn present(&mut self) -> Result<()> {
        // Let's simplify for debugging
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex during present: {}", e))?;
        
        // Set a simple red background
        canvas.set_draw_color(sdl2::pixels::Color::RGB(255, 0, 0));
        canvas.clear();
        
        // Add some visual elements
        canvas.set_draw_color(sdl2::pixels::Color::RGB(255, 255, 255));
        canvas.fill_rect(sdl2::rect::Rect::new(300, 200, 200, 100))
            .map_err(|e| anyhow::anyhow!("Failed to draw white box: {}", e))?;
        
        canvas.present();
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn graphics_context(&self) -> Option<&dyn GraphicsContext> {
        Some(&self.graphics_context)
    }
    
    fn cleanup(&mut self) {
        log::info!("SDL platform cleaned up");
    }
}

// Clean implementation with only the PlatformDriver trait