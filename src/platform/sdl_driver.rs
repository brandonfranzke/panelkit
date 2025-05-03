//! SDL2-based display and input driver
//!
//! This module provides SDL2 implementations for display rendering and input handling.

use crate::event::{Event, TouchAction};
use crate::platform::{DisplayDriver, InputDriver};

use anyhow::Result;
use sdl2::event::Event as SdlEvent;
use sdl2::keyboard::Keycode;
use sdl2::mouse::MouseButton;
use sdl2::pixels::Color;
use std::sync::{Arc, Mutex};
use std::time::Duration;

/// SDL2-based display driver
pub struct SDLDriver {
    sdl_context: sdl2::Sdl,
    canvas: Arc<Mutex<sdl2::render::Canvas<sdl2::video::Window>>>,
    width: u32,
    height: u32,
    last_touch_point: (i32, i32),
    is_pressed: bool,
}

// Implement the combined Driver trait
impl crate::platform::Driver for SDLDriver {
    fn init(&mut self, _width: u32, _height: u32) -> Result<()> {
        // Initialize SDL
        self.clear(0, 0, 0)?;
        
        // Present the canvas
        {
            let mut canvas = self.canvas.lock().unwrap();
            canvas.present();
        }
        
        log::info!("SDL display initialized with dimensions: {}x{}", self.width, self.height);
        Ok(())
    }
    
    fn flush(&mut self, _buffer: &[u8]) -> Result<()> {
        // Present the canvas
        let mut canvas = self.canvas.lock().unwrap();
        canvas.present();
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn init_input(&mut self) -> Result<()> {
        log::info!("SDL input driver initialized");
        Ok(())
    }
    
    fn poll_events(&mut self) -> Result<Vec<Event>> {
        let mut events = Vec::new();
        let mut event_pump = self.sdl_context.event_pump().unwrap();
        
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
    
    fn cleanup(&mut self) {
        log::info!("SDL driver cleaned up");
    }
    
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
}

impl SDLDriver {
    /// Create a new SDL driver
    pub fn new(width: u32, height: u32, title: &str) -> Result<Self> {
        let sdl_context = sdl2::init().map_err(|e| anyhow::anyhow!("SDL init error: {}", e))?;
        let video_subsystem = sdl_context
            .video()
            .map_err(|e| anyhow::anyhow!("SDL video subsystem error: {}", e))?;

        // Use different SDL window settings for macOS vs other platforms
        #[cfg(target_os = "macos")]
        let window = video_subsystem
            .window(title, width, height)
            .position_centered()
            .allow_highdpi() // Enable Retina display support
            .build()
            .map_err(|e| anyhow::anyhow!("SDL window error on macOS: {}", e))?;

        #[cfg(not(target_os = "macos"))]
        let window = video_subsystem
            .window(title, width, height)
            .position_centered()
            .build()
            .map_err(|e| anyhow::anyhow!("SDL window error: {}", e))?;

        let canvas = window
            .into_canvas()
            .accelerated()
            .build()
            .map_err(|e| anyhow::anyhow!("SDL canvas error: {}", e))?;

        Ok(Self {
            sdl_context,
            canvas: Arc::new(Mutex::new(canvas)),
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
        let mut canvas = self.canvas.lock().unwrap();
        canvas.set_draw_color(Color::RGB(r, g, b));
        canvas.clear();
        Ok(())
    }
}

impl DisplayDriver for SDLDriver {
    fn init(&mut self, _width: u32, _height: u32) -> Result<()> {
        // Initialize SDL
        self.clear(0, 0, 0)?;
        
        // Present the canvas
        {
            let mut canvas = self.canvas.lock().unwrap();
            canvas.present();
        }
        
        log::info!("SDL display initialized with dimensions: {}x{}", self.width, self.height);
        Ok(())
    }
    
    fn flush(&mut self, _buffer: &[u8]) -> Result<()> {
        // Present the canvas
        let mut canvas = self.canvas.lock().unwrap();
        canvas.present();
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn cleanup(&mut self) {
        log::info!("SDL display cleaned up");
    }
    
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
}

impl InputDriver for SDLDriver {
    fn init(&mut self) -> Result<()> {
        log::info!("SDL input driver initialized");
        Ok(())
    }
    
    fn poll_events(&mut self) -> Result<Vec<Event>> {
        let mut events = Vec::new();
        let mut event_pump = self.sdl_context.event_pump().unwrap();
        
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
    
    fn cleanup(&mut self) {
        log::info!("SDL input driver cleaned up");
    }
    
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
}

