//! SDL2-based display and input driver
//!
//! This module provides SDL2 implementation for the PanelKit platform interface.

use crate::event::{Event, TouchAction};
use crate::platform::{GraphicsContext, PlatformDriver};
use crate::platform::graphics::{Color, Point, Rectangle};

use crate::error::PlatformError;
use anyhow::{Result, Context};
use sdl2::event::Event as SdlEvent;
use sdl2::keyboard::Keycode;
use sdl2::mouse::MouseButton;
use std::sync::{Arc, Mutex};
use std::time::Duration;
use std::fmt::Display;

/// SDL2 rendering context that implements the GraphicsContext trait
pub struct SDLGraphicsContext {
    canvas: Arc<Mutex<sdl2::render::Canvas<sdl2::video::Window>>>,
    width: u32,
    height: u32,
}

impl SDLGraphicsContext {
    /// Create a new SDL graphics context
    pub fn new(canvas: Arc<Mutex<sdl2::render::Canvas<sdl2::video::Window>>>, width: u32, height: u32) -> Self {
        log::debug!("Creating new SDLGraphicsContext with canvas");
        Self { canvas, width, height }
    }
}

impl GraphicsContext for SDLGraphicsContext {
    fn clear(&mut self, color: Color) -> Result<()> {
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
            
        canvas.set_draw_color(sdl2::pixels::Color::RGB(color.r, color.g, color.b));
        canvas.clear();
        Ok(())
    }
    
    fn set_draw_color(&mut self, color: Color) -> Result<()> {
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
            
        canvas.set_draw_color(sdl2::pixels::Color::RGB(color.r, color.g, color.b));
        Ok(())
    }
    
    fn fill_rect(&mut self, rect: Rectangle) -> Result<()> {
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
            
        SDLDriver::convert_sdl_error(
            canvas.fill_rect(sdl2::rect::Rect::new(rect.x, rect.y, rect.width, rect.height)),
            "Failed to fill rectangle"
        )?;
            
        Ok(())
    }
    
    fn draw_rect(&mut self, rect: Rectangle) -> Result<()> {
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
            
        SDLDriver::convert_sdl_error(
            canvas.draw_rect(sdl2::rect::Rect::new(rect.x, rect.y, rect.width, rect.height)),
            "Failed to draw rectangle outline"
        )?;
            
        Ok(())
    }
    
    fn draw_line(&mut self, start: Point, end: Point) -> Result<()> {
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
            
        SDLDriver::convert_sdl_error(
            canvas.draw_line(
                sdl2::rect::Point::new(start.x, start.y),
                sdl2::rect::Point::new(end.x, end.y)
            ),
            "Failed to draw line"
        )?;
        
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
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
        
        // Initialize with gray background using a temporary context
        let mut ctx = self.create_graphics_context()?;
        ctx.clear(Color::rgb(100, 100, 100))?;
        
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
                SdlEvent::KeyDown { keycode: Some(Keycode::Right), .. } => {
                    events.push(Event::Custom { 
                        event_type: "next_page".to_string(), 
                        payload: "".to_string() 
                    });
                },
                SdlEvent::KeyDown { keycode: Some(Keycode::Left), .. } => {
                    events.push(Event::Custom { 
                        event_type: "prev_page".to_string(), 
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
    
    fn create_graphics_context(&mut self) -> Result<Box<dyn GraphicsContext>> {
        // Create a new graphics context that shares the canvas
        let context = SDLGraphicsContext::new(self.canvas.clone(), self.width, self.height);
        Ok(Box::new(context))
    }
    
    fn cleanup(&mut self) {
        log::info!("SDL platform cleaned up");
    }
}