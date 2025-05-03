//! SDL2-based display and input driver for LVGL
//!
//! This module provides SDL2 implementations for display rendering and input handling.

use crate::event::{Event, TouchAction};
use crate::platform::{DisplayDriver, InputDriver};

use anyhow::Result;
use lvgl::{Display, DrawBuffer, InputDevice, TouchPad};
use sdl2::event::Event as SdlEvent;
use sdl2::keyboard::Keycode;
use sdl2::mouse::MouseButton;
use sdl2::pixels::PixelFormatEnum;
use sdl2::rect::Rect;
use std::cell::RefCell;
use std::sync::{Arc, Mutex};
use std::time::Duration;

/// SDL2-based display driver for LVGL
pub struct SDLDriver {
    sdl_context: sdl2::Sdl,
    canvas: sdl2::render::Canvas<sdl2::video::Window>,
    width: u32,
    height: u32,
    lv_display: Option<Display>,
    draw_buffer: Option<DrawBuffer<lvgl::color::Color16>>,
    input_device: Option<InputDevice<TouchPad>>,
    last_touch_point: (i32, i32),
    is_pressed: bool,
}

impl SDLDriver {
    /// Create a new SDL driver
    pub fn new(width: u32, height: u32, title: &str) -> Result<Self> {
        let sdl_context = sdl2::init().map_err(|e| anyhow::anyhow!("SDL init error: {}", e))?;
        let video_subsystem = sdl_context
            .video()
            .map_err(|e| anyhow::anyhow!("SDL video subsystem error: {}", e))?;

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
            canvas,
            width,
            height,
            lv_display: None,
            draw_buffer: None,
            input_device: None,
            last_touch_point: (0, 0),
            is_pressed: false,
        })
    }

    /// Initialize the LVGL display
    pub fn init_lvgl(&mut self) -> Result<()> {
        // Create a draw buffer
        let buf_size = (self.width * self.height) as usize;
        let draw_buffer = DrawBuffer::new(buf_size / 10);

        // Create LVGL display
        let display = Display::new(&lvgl::DISPLAY_LARGE).with_draw_buffer(draw_buffer);

        // Set flush callback
        let display_ref = RefCell::new(self.canvas.clone());
        display.set_flush_cb(Box::new(move |_, area, colors| {
            let mut display = display_ref.borrow_mut();

            let x = area.x1 as i32;
            let y = area.y1 as i32;
            let width = (area.x2 - area.x1 + 1) as u32;
            let height = (area.y2 - area.y1 + 1) as u32;

            // Create texture for this area
            let creator = display.texture_creator();
            let mut texture = creator
                .create_texture_streaming(PixelFormatEnum::RGB565, width, height)
                .unwrap();

            // Update the texture with the color data
            texture
                .update(
                    None,
                    unsafe {
                        std::slice::from_raw_parts(
                            colors.as_ptr() as *const u8,
                            (width * height * 2) as usize,
                        )
                    },
                    (width * 2) as usize,
                )
                .unwrap();

            // Copy the texture to the canvas
            display
                .copy(&texture, None, Some(Rect::new(x, y, width, height)))
                .unwrap();
        }));

        // Create input device for touch events
        let mut input_device = InputDevice::new(TouchPad::new());
        
        // Set the read callback
        let is_pressed = Arc::new(Mutex::new(self.is_pressed));
        let last_point = Arc::new(Mutex::new(self.last_touch_point));
        
        input_device.set_read_cb(Box::new(move |_, data| {
            let pressed = *is_pressed.lock().unwrap();
            let (x, y) = *last_point.lock().unwrap();
            
            data.update_points(&[(x, y)], pressed);
        }));

        // Store the LVGL components
        self.draw_buffer = Some(draw_buffer);
        self.lv_display = Some(display);
        self.input_device = Some(input_device);

        Ok(())
    }

    /// Update touch state based on SDL events
    pub fn update_touch(&mut self, x: i32, y: i32, pressed: bool) {
        self.last_touch_point = (x, y);
        self.is_pressed = pressed;
    }

    /// Flush the display by presenting the SDL canvas
    pub fn flush(&mut self) -> Result<()> {
        self.canvas.present();
        Ok(())
    }

    /// Clear the screen with a specific color
    pub fn clear(&mut self, r: u8, g: u8, b: u8) -> Result<()> {
        self.canvas.set_draw_color(sdl2::pixels::Color::RGB(r, g, b));
        self.canvas.clear();
        Ok(())
    }
}

impl DisplayDriver for SDLDriver {
    fn init(&mut self, _width: u32, _height: u32) -> Result<()> {
        // Initialize SDL
        self.clear(0, 0, 0)?;
        self.flush()?;
        
        // Initialize LVGL
        self.init_lvgl()?;
        
        log::info!("SDL display initialized with dimensions: {}x{}", self.width, self.height);
        Ok(())
    }
    
    fn flush(&mut self, _buffer: &[u8]) -> Result<()> {
        // Process SDL events
        lvgl::task_handler();
        
        // Present the canvas
        self.flush()?;
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn cleanup(&mut self) {
        log::info!("SDL display cleaned up");
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
        
        // Tick LVGL
        std::thread::sleep(Duration::from_millis(10));
        
        Ok(events)
    }
    
    fn cleanup(&mut self) {
        log::info!("SDL input driver cleaned up");
    }
}