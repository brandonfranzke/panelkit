//! LVGL-based platform driver implementation
//!
//! This module provides an LVGL implementation for the PanelKit platform interface.

use crate::error::{PlatformError, Result};
use crate::event::{Event, TouchAction};
use crate::platform::{Color, GraphicsContext, PlatformDriver, Point, Rectangle};

use lvgl::core::{Display, Object, Screen};
use lvgl::display::{Driver, Draggable, DrawBuffer, MonoColor, Part};
use lvgl::event::{Event as LvEvent, EventCallback};
use lvgl::style::{Opacity, Style};
use lvgl::{Coord, LvError, LvResult};
use std::cell::RefCell;
use std::rc::Rc;
use std::sync::Mutex;

#[cfg(feature = "host")]
use sdl2::event::Event as SdlEvent;
#[cfg(feature = "host")]
use sdl2::keyboard::Keycode;
#[cfg(feature = "host")]
use sdl2::mouse::MouseButton;
#[cfg(feature = "host")]
use sdl2::pixels::PixelFormat;

/// LVGL graphics context implementing GraphicsContext trait
pub struct LvglGraphicsContext {
    /// Current screen object
    screen: Rc<RefCell<Screen>>,
    /// Display dimensions
    width: u32,
    height: u32,
    /// Current draw color
    current_color: Color,
}

impl LvglGraphicsContext {
    /// Create a new LVGL graphics context
    pub fn new(width: u32, height: u32) -> Self {
        // Create a new LVGL screen
        let screen = Screen::default();
        
        Self {
            screen: Rc::new(RefCell::new(screen)),
            width,
            height,
            current_color: Color::rgb(0, 0, 0),
        }
    }
    
    /// Get the LVGL screen object
    pub fn get_screen(&self) -> Rc<RefCell<Screen>> {
        self.screen.clone()
    }
    
    /// Convert PanelKit color to LVGL color
    fn to_lvgl_color(&self, color: Color) -> lvgl::Color {
        lvgl::Color::from_rgb(color.r, color.g, color.b)
    }
}

impl GraphicsContext for LvglGraphicsContext {
    fn clear(&mut self, color: Color) -> anyhow::Result<()> {
        // Set screen background color
        let lv_color = self.to_lvgl_color(color);
        
        // Create a style with the background color
        let style = Style::default();
        style.set_bg_color(lv_color);
        style.set_radius(0); // No rounded corners
        style.set_border_width(0); // No border
        
        // Apply the style to the screen
        self.screen.borrow_mut().add_style(Part::Main, &style);
        
        Ok(())
    }
    
    fn set_draw_color(&mut self, color: Color) -> anyhow::Result<()> {
        self.current_color = color;
        Ok(())
    }
    
    fn fill_rect(&mut self, rect: Rectangle) -> anyhow::Result<()> {
        // Create a new rectangle object
        let parent = self.screen.clone();
        let obj = Object::new(Some(&parent.borrow()));
        
        // Set size and position
        obj.set_size(Coord::new(rect.width as i16), Coord::new(rect.height as i16));
        obj.set_pos(Coord::new(rect.x), Coord::new(rect.y));
        
        // Set style for filled rectangle
        let style = Style::default();
        style.set_bg_color(self.to_lvgl_color(self.current_color));
        style.set_bg_opa(Opacity::COVER);
        style.set_radius(0); // No rounded corners
        style.set_border_width(0); // No border
        
        // Apply style
        obj.add_style(Part::Main, &style);
        
        Ok(())
    }
    
    fn draw_rect(&mut self, rect: Rectangle) -> anyhow::Result<()> {
        // Create a new rectangle object
        let parent = self.screen.clone();
        let obj = Object::new(Some(&parent.borrow()));
        
        // Set size and position
        obj.set_size(Coord::new(rect.width as i16), Coord::new(rect.height as i16));
        obj.set_pos(Coord::new(rect.x), Coord::new(rect.y));
        
        // Set style for outlined rectangle
        let style = Style::default();
        style.set_bg_opa(Opacity::TRANSP); // Transparent background
        style.set_border_color(self.to_lvgl_color(self.current_color));
        style.set_border_width(1); // 1px border
        style.set_radius(0); // No rounded corners
        
        // Apply style
        obj.add_style(Part::Main, &style);
        
        Ok(())
    }
    
    fn draw_line(&mut self, start: Point, end: Point) -> anyhow::Result<()> {
        use lvgl::widgets::Line;
        
        // Create a new line object
        let parent = self.screen.clone();
        let line = Line::new(Some(&parent.borrow()));
        
        // Define points
        let points = [
            lvgl::Point::new(Coord::new(start.x), Coord::new(start.y)),
            lvgl::Point::new(Coord::new(end.x), Coord::new(end.y)),
        ];
        
        // Set line points
        line.set_points(&points);
        
        // Set line style
        let style = Style::default();
        style.set_line_color(self.to_lvgl_color(self.current_color));
        style.set_line_width(1); // 1px line width
        
        // Apply style
        line.add_style(Part::Main, &style);
        
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
}

/// LVGL-based platform driver
pub struct LvglDriver {
    /// LVGL display
    display: Display,
    /// Screen dimensions
    width: u32,
    height: u32,
    /// Platform-specific data
    #[cfg(feature = "host")]
    sdl_context: sdl2::Sdl,
    #[cfg(feature = "host")]
    event_pump: Option<sdl2::EventPump>,
}

impl LvglDriver {
    /// Create a new LVGL driver
    pub fn new(width: u32, height: u32, title: &str) -> Result<Self> {
        // Initialize LVGL
        lvgl::init().map_err(|e| PlatformError::Initialization(
            format!("Failed to initialize LVGL: {:?}", e)
        ))?;
        
        #[cfg(feature = "host")]
        let (display, sdl_context) = Self::init_sdl_display(width, height, title)?;
        
        #[cfg(not(feature = "host"))]
        let display = Self::init_fb_display(width, height)?;
        
        #[cfg(feature = "host")]
        {
            Ok(Self {
                display,
                width,
                height,
                sdl_context,
                event_pump: None,
            })
        }
        
        #[cfg(not(feature = "host"))]
        {
            Ok(Self {
                display,
                width,
                height,
            })
        }
    }
    
    /// Initialize SDL-based display for host
    #[cfg(feature = "host")]
    fn init_sdl_display(width: u32, height: u32, title: &str) -> Result<(Display, sdl2::Sdl)> {
        // Initialize SDL2
        let sdl_context = sdl2::init()
            .map_err(|e| PlatformError::Initialization(format!("Failed to initialize SDL2: {}", e)))?;
            
        let video_subsystem = sdl_context.video()
            .map_err(|e| PlatformError::Initialization(format!("Failed to initialize SDL2 video: {}", e)))?;
            
        // Create window
        let window = video_subsystem
            .window(title, width, height)
            .position_centered()
            .build()
            .map_err(|e| PlatformError::Initialization(format!("Failed to create SDL2 window: {}", e)))?;
            
        // Create canvas
        let canvas = window.into_canvas()
            .accelerated()
            .present_vsync()
            .build()
            .map_err(|e| PlatformError::Initialization(format!("Failed to create SDL2 canvas: {}", e)))?;
            
        // Setup LVGL display
        let buffer_size = (width * height) as usize;
        let draw_buf = DrawBuffer::new(buffer_size);
        
        // Create SDL driver
        let driver = Driver::default();
        
        // Create LVGL display with the driver
        let display = Display::new(driver);
        
        Ok((display, sdl_context))
    }
    
    /// Initialize framebuffer display for embedded target
    #[cfg(not(feature = "host"))]
    fn init_fb_display(width: u32, height: u32) -> Result<Display> {
        // Setup LVGL display
        let buffer_size = (width * height) as usize;
        let draw_buf = DrawBuffer::new(buffer_size);
        
        // Create dummy driver for now - we'd implement a proper framebuffer driver here
        let driver = Driver::default();
        
        // Create LVGL display with the driver
        let display = Display::new(driver);
        
        Ok(display)
    }
}

impl PlatformDriver for LvglDriver {
    fn init(&mut self, width: u32, height: u32) -> Result<()> {
        log::info!("LVGL platform initialized with dimensions: {}x{}", width, height);
        
        // Update dimensions if provided
        if width > 0 && height > 0 {
            self.width = width;
            self.height = height;
        }
        
        // Initialize input device
        #[cfg(feature = "host")]
        {
            // Initialize event pump for SDL2
            self.event_pump = Some(self.sdl_context.event_pump()
                .map_err(|e| PlatformError::Initialization(format!("Failed to create SDL2 event pump: {}", e)))?);
        }
        
        Ok(())
    }
    
    fn poll_events(&mut self) -> Result<Vec<Event>> {
        let mut events = Vec::new();
        
        #[cfg(feature = "host")]
        {
            // Poll SDL2 events
            if let Some(event_pump) = &mut self.event_pump {
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
                            events.push(Event::Touch { 
                                x, 
                                y, 
                                action: TouchAction::Press 
                            });
                        },
                        SdlEvent::MouseButtonUp { mouse_btn: MouseButton::Left, x, y, .. } => {
                            events.push(Event::Touch { 
                                x, 
                                y, 
                                action: TouchAction::Release 
                            });
                        },
                        SdlEvent::MouseMotion { x, y, mousestate, .. } => {
                            if mousestate.left() {
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
            }
            
            // Sleep to avoid using too much CPU
            std::thread::sleep(std::time::Duration::from_millis(10));
        }
        
        #[cfg(not(feature = "host"))]
        {
            // Poll events from framebuffer device
            // This would need to be implemented based on the embedded platform
        }
        
        Ok(events)
    }
    
    fn present(&mut self) -> Result<()> {
        // Process LVGL tasks
        lvgl::task_handler();
        
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn create_graphics_context(&mut self) -> Result<Box<dyn GraphicsContext>> {
        // Create a new LVGL graphics context
        let context = LvglGraphicsContext::new(self.width, self.height);
        Ok(Box::new(context))
    }
    
    fn cleanup(&mut self) {
        log::info!("LVGL platform cleaned up");
    }
}