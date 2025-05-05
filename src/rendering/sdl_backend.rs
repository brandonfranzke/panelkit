//! SDL2-based rendering backend
//!
//! This module implements the RenderingContext trait using SDL2 for host development.
//! It uses runtime detection rather than compile-time feature flags.

use anyhow::{Result, Context, bail};
use crate::primitives::{Color, Point, Rectangle, TextStyle, FontSize, TextAlignment, RenderingContext, Surface};
use crate::event::{Event, TouchEvent, KeyboardEvent, CustomEvent, SystemEvent, TouchAction, SystemEventType};
use std::any::Any;
use std::path::Path;

/// SDL2 implementation of the RenderingContext trait
pub struct SDLBackend {
    // When SDL2 is available, these fields will be populated
    sdl_available: bool,
    width: u32,
    height: u32,
    #[allow(dead_code)] // May not be used in all code paths
    font_path: String,
    
    // These fields are Option types and will be None when SDL2 is not available
    sdl_context: Option<sdl2::Sdl>,
    canvas: Option<std::sync::Arc<std::sync::Mutex<sdl2::render::Canvas<sdl2::video::Window>>>>,
    ttf_context: Option<sdl2::ttf::Sdl2TtfContext>,
}

impl SDLBackend {
    /// Create a new SDL backend with the given window title
    pub fn new(title: &str) -> Result<Self> {
        // Check if SDL2 is available at runtime
        let sdl_available = Self::is_sdl_available();
        
        if !sdl_available {
            log::warn!("SDL2 is not available at runtime");
            return Ok(Self {
                sdl_available: false,
                width: 800,
                height: 480,
                font_path: String::new(),
                sdl_context: None,
                canvas: None,
                ttf_context: None,
            });
        }
        
        // Try to initialize SDL components
        match Self::initialize_sdl(title) {
            Ok((sdl_context, canvas, ttf_context, font_path)) => {
                Ok(Self {
                    sdl_available: true,
                    width: 800,
                    height: 480,
                    font_path,
                    sdl_context: Some(sdl_context),
                    canvas: Some(std::sync::Arc::new(std::sync::Mutex::new(canvas))),
                    ttf_context: Some(ttf_context),
                })
            },
            Err(e) => {
                log::warn!("Failed to initialize SDL2: {}", e);
                Ok(Self {
                    sdl_available: false,
                    width: 800,
                    height: 480,
                    font_path: String::new(),
                    sdl_context: None,
                    canvas: None,
                    ttf_context: None,
                })
            }
        }
    }
    
    /// Check if SDL2 is available at runtime
    fn is_sdl_available() -> bool {
        // Try to dynamically load SDL2
        // This is a simplified check - in reality, we'd use the sdl2 crate's functionality
        match sdl2::init() {
            Ok(_) => true,
            Err(e) => {
                log::warn!("SDL2 not available: {}", e);
                false
            }
        }
    }
    
    /// Initialize SDL components
    fn initialize_sdl(title: &str) -> Result<(sdl2::Sdl, sdl2::render::Canvas<sdl2::video::Window>, sdl2::ttf::Sdl2TtfContext, String)> {
        // Initialize SDL2
        let sdl_context = sdl2::init()
            .map_err(|e| anyhow::anyhow!("Failed to initialize SDL2: {}", e))?;
            
        let video_subsystem = sdl_context.video()
            .map_err(|e| anyhow::anyhow!("Failed to initialize SDL2 video subsystem: {}", e))?;
        
        // Create window
        let window = video_subsystem
            .window(title, 800, 480)  // Default size, will be updated in init()
            .position_centered()
            .allow_highdpi() // Enable Retina display support
            .build()
            .map_err(|e| anyhow::anyhow!("Failed to create SDL2 window: {}", e))?;
            
        // Create canvas
        let canvas = window
            .into_canvas()
            .accelerated()
            .present_vsync() // Use vsync for smoother rendering
            .build()
            .map_err(|e| anyhow::anyhow!("Failed to create SDL2 canvas: {}", e))?;
            
        // Initialize TTF
        let ttf_context = sdl2::ttf::init()
            .map_err(|e| anyhow::anyhow!("Failed to initialize SDL2 TTF: {}", e))?;
            
        // Find a usable font
        let font_path = find_system_font()?;
        
        Ok((sdl_context, canvas, ttf_context, font_path))
    }
    
    /// Poll for events and convert them to PanelKit events
    pub fn poll_events(&self) -> Result<Vec<Box<dyn Event>>> {
        // If SDL2 is not available, return an empty vec
        if !self.sdl_available || self.sdl_context.is_none() {
            return Ok(Vec::new());
        }
        
        let sdl_context = self.sdl_context.as_ref().unwrap();
        
        let mut event_pump = sdl_context.event_pump()
            .map_err(|e| anyhow::anyhow!("Failed to get SDL event pump: {}", e))?;
            
        let mut events: Vec<Box<dyn Event>> = Vec::new();
        
        for event in event_pump.poll_iter() {
            match event {
                sdl2::event::Event::Quit {..} => {
                    // Map quit event to a custom event
                    events.push(Box::new(CustomEvent::new("quit", "")));
                },
                sdl2::event::Event::KeyDown { keycode: Some(key), .. } => {
                    // Map key events
                    let key_code = key as u32;
                    events.push(Box::new(KeyboardEvent::new(key_code, true)));
                },
                sdl2::event::Event::KeyUp { keycode: Some(key), .. } => {
                    // Map key events
                    let key_code = key as u32;
                    events.push(Box::new(KeyboardEvent::new(key_code, false)));
                },
                sdl2::event::Event::MouseButtonDown { x, y, mouse_btn, .. } => {
                    // Map mouse events to touch events for simplicity
                    match mouse_btn {
                        sdl2::mouse::MouseButton::Left => {
                            events.push(Box::new(TouchEvent::new(
                                TouchAction::Down,
                                Point::new(x, y)
                            )));
                        },
                        _ => {}
                    }
                },
                sdl2::event::Event::MouseButtonUp { x, y, mouse_btn, .. } => {
                    // Map mouse events to touch events for simplicity
                    match mouse_btn {
                        sdl2::mouse::MouseButton::Left => {
                            events.push(Box::new(TouchEvent::new(
                                TouchAction::Up,
                                Point::new(x, y)
                            )));
                        },
                        _ => {}
                    }
                },
                sdl2::event::Event::MouseMotion { x, y, mousestate, .. } => {
                    // If mouse button is pressed, send a Move event
                    if mousestate.left() {
                        events.push(Box::new(TouchEvent::new(
                            TouchAction::Move,
                            Point::new(x, y)
                        )));
                    }
                },
                sdl2::event::Event::Window { win_event: sdl2::event::WindowEvent::Resized(w, h), .. } => {
                    // Handle window resize events
                    events.push(Box::new(SystemEvent::new(
                        SystemEventType::Resize {
                            width: w as u32,
                            height: h as u32
                        }
                    )));
                },
                _ => {}
            }
        }
        
        Ok(events)
    }
    
    /// Convert a Color to an SDL Color
    fn to_sdl_color(&self, color: Color) -> sdl2::pixels::Color {
        sdl2::pixels::Color::RGBA(color.r, color.g, color.b, color.a)
    }
    
    /// Convert a Point to an SDL Point
    fn to_sdl_point(&self, point: Point) -> sdl2::rect::Point {
        sdl2::rect::Point::new(point.x, point.y)
    }
    
    /// Convert a Rectangle to an SDL Rect
    fn to_sdl_rect(&self, rect: Rectangle) -> sdl2::rect::Rect {
        sdl2::rect::Rect::new(rect.x, rect.y, rect.width, rect.height)
    }
    
    /// Load a font with the requested size
    fn load_font(&self, size: u16) -> Result<sdl2::ttf::Font> {
        if let Some(ttf_context) = &self.ttf_context {
            ttf_context.load_font(&self.font_path, size)
                .map_err(|e| anyhow::anyhow!("Failed to load font {} at size {}: {}", self.font_path, size, e))
        } else {
            bail!("TTF context not available")
        }
    }
}

impl RenderingContext for SDLBackend {
    fn init(&mut self, width: u32, height: u32) -> Result<()> {
        self.width = width;
        self.height = height;
        
        if !self.sdl_available {
            log::warn!("Using stub SDL backend - no real rendering will occur");
            return Ok(());
        }
        
        // Try to resize the window (this may not work on all platforms)
        if let Some(canvas_arc) = &self.canvas {
            let mut canvas = canvas_arc.lock()
                .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
                
            let window = canvas.window_mut();
            window.set_size(width, height)
                .context("Failed to resize SDL window")?;
                
            // Initial clear to gray
            canvas.set_draw_color(sdl2::pixels::Color::RGB(100, 100, 100));
            canvas.clear();
            canvas.present();
        }
        
        Ok(())
    }
    
    fn present(&mut self) -> Result<()> {
        if !self.sdl_available {
            log::trace!("Stub SDL backend: present called");
            return Ok(());
        }
        
        if let Some(canvas_arc) = &self.canvas {
            let mut canvas = canvas_arc.lock()
                .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
                
            canvas.present();
        }
        
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn cleanup(&mut self) {
        // SDL2 will handle cleanup automatically when the context is dropped
        log::trace!("SDL backend cleanup called");
    }
    
    fn clear(&mut self, color: Color) -> Result<()> {
        if !self.sdl_available {
            log::trace!("Stub SDL backend: clear with color {:?}", color);
            return Ok(());
        }
        
        if let Some(canvas_arc) = &self.canvas {
            let mut canvas = canvas_arc.lock()
                .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
                
            canvas.set_draw_color(self.to_sdl_color(color));
            canvas.clear();
        }
        
        Ok(())
    }
    
    fn fill_rect(&mut self, rect: Rectangle, color: Color) -> Result<()> {
        if !self.sdl_available {
            log::trace!("Stub SDL backend: fill_rect at {:?} with color {:?}", rect, color);
            return Ok(());
        }
        
        if let Some(canvas_arc) = &self.canvas {
            let mut canvas = canvas_arc.lock()
                .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
                
            canvas.set_draw_color(self.to_sdl_color(color));
            canvas.fill_rect(self.to_sdl_rect(rect))
                .map_err(|e| anyhow::anyhow!("Failed to fill rectangle: {}", e))?;
        }
        
        Ok(())
    }
    
    fn draw_rect(&mut self, rect: Rectangle, color: Color) -> Result<()> {
        if !self.sdl_available {
            log::trace!("Stub SDL backend: draw_rect at {:?} with color {:?}", rect, color);
            return Ok(());
        }
        
        if let Some(canvas_arc) = &self.canvas {
            let mut canvas = canvas_arc.lock()
                .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
                
            canvas.set_draw_color(self.to_sdl_color(color));
            canvas.draw_rect(self.to_sdl_rect(rect))
                .map_err(|e| anyhow::anyhow!("Failed to draw rectangle: {}", e))?;
        }
        
        Ok(())
    }
    
    fn draw_line(&mut self, start: Point, end: Point, color: Color) -> Result<()> {
        if !self.sdl_available {
            log::trace!("Stub SDL backend: draw_line from {:?} to {:?} with color {:?}", start, end, color);
            return Ok(());
        }
        
        if let Some(canvas_arc) = &self.canvas {
            let mut canvas = canvas_arc.lock()
                .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
                
            canvas.set_draw_color(self.to_sdl_color(color));
            canvas.draw_line(self.to_sdl_point(start), self.to_sdl_point(end))
                .map_err(|e| anyhow::anyhow!("Failed to draw line: {}", e))?;
        }
        
        Ok(())
    }
    
    fn draw_text(&mut self, text: &str, position: Point, style: TextStyle) -> Result<()> {
        if !self.sdl_available {
            log::trace!("Stub SDL backend: draw_text '{}' at {:?}", text, position);
            return Ok(());
        }
        
        if let Some(canvas_arc) = &self.canvas {
            let canvas_lock = canvas_arc.lock()
                .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
                
            // Create texture creator
            let texture_creator = canvas_lock.texture_creator();
            
            // Load font with appropriate style
            let mut font = self.load_font(style.font_size.to_points())?;
            
            // Set font style
            let mut sdl_font_style = sdl2::ttf::FontStyle::NORMAL;
            if style.bold {
                sdl_font_style = sdl_font_style | sdl2::ttf::FontStyle::BOLD;
            }
            if style.italic {
                sdl_font_style = sdl_font_style | sdl2::ttf::FontStyle::ITALIC;
            }
            font.set_style(sdl_font_style);
            
            // Render text to surface
            let surface = font.render(text)
                .blended(self.to_sdl_color(style.color))
                .map_err(|e| anyhow::anyhow!("Failed to render text to surface: {}", e))?;
                
            // Create texture from surface
            let texture = texture_creator.create_texture_from_surface(&surface)
                .map_err(|e| anyhow::anyhow!("Failed to create texture from text surface: {}", e))?;
                
            // Get text dimensions
            let text_width = surface.width();
            let text_height = surface.height();
            
            // Determine position based on alignment
            let mut x = position.x;
            match style.alignment {
                TextAlignment::Left => {}
                TextAlignment::Center => x -= (text_width / 2) as i32,
                TextAlignment::Right => x -= text_width as i32,
            }
            
            // Create destination rectangle
            let dest_rect = sdl2::rect::Rect::new(x, position.y, text_width, text_height);
            
            // Draw the text
            let mut canvas = canvas_lock;
            canvas.copy(&texture, None, Some(dest_rect))
                .map_err(|e| anyhow::anyhow!("Failed to copy text texture to canvas: {}", e))?;
        }
        
        Ok(())
    }
    
    fn draw_button(&mut self, rect: Rectangle, text: &str, bg_color: Color, text_color: Color, border_color: Color) -> Result<()> {
        if !self.sdl_available {
            log::trace!("Stub SDL backend: draw_button '{}' at {:?}", text, rect);
            return Ok(());
        }
        
        // Fill the button background
        self.fill_rect(rect, bg_color)?;
        
        // Draw the button border
        self.draw_rect(rect, border_color)?;
        
        // Calculate text position (centered in button)
        let center = Point::new(
            rect.x + (rect.width as i32 / 2),
            rect.y + (rect.height as i32 / 2)
        );
        
        // Draw the button text
        let text_style = TextStyle::new(text_color)
            .with_alignment(TextAlignment::Center)
            .with_size(FontSize::Medium);
            
        self.draw_text(text, center, text_style)?;
        
        Ok(())
    }
    
    fn create_surface(&mut self, width: u32, height: u32) -> Result<Box<dyn Surface>> {
        if !self.sdl_available {
            log::trace!("Stub SDL backend: create_surface {}x{}", width, height);
            bail!("Surface creation not supported in stub SDL backend");
        }
        
        // Create a surface implementation
        let surface = SDLSurface::new(width, height);
        Ok(Box::new(surface))
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}

/// SDL implementation of a rendering surface
/// 
/// This is a simplified implementation that avoids lifetime issues by
/// using a vector buffer instead of an SDL texture directly.
pub struct SDLSurface {
    width: u32,
    height: u32,
    color: Color,
    buffer: Vec<u32>,  // RGBA buffer for pixel data
}

impl SDLSurface {
    /// Create a new SDL surface
    fn new(width: u32, height: u32) -> Self {
        // Create buffer initialized with transparent pixels
        let buffer_size = (width * height) as usize;
        let buffer = vec![0; buffer_size]; // 0x00000000 = transparent black
        
        Self {
            width,
            height,
            color: Color::transparent(),
            buffer,
        }
    }
    
    /// Get the internal buffer
    pub fn buffer(&self) -> &[u32] {
        &self.buffer
    }
    
    /// Set a pixel in the buffer
    fn set_pixel(&mut self, x: u32, y: u32, color: Color) {
        if x >= self.width || y >= self.height {
            return; // Out of bounds
        }
        
        let offset = (y * self.width + x) as usize;
        if offset < self.buffer.len() {
            let pixel = ((color.a as u32) << 24) | 
                        ((color.r as u32) << 16) | 
                        ((color.g as u32) << 8) | 
                        (color.b as u32);
            self.buffer[offset] = pixel;
        }
    }
}

impl Surface for SDLSurface {
    fn clear(&mut self, color: Color) -> Result<()> {
        self.color = color;
        
        // Fill the buffer with the color
        let pixel = ((color.a as u32) << 24) | 
                    ((color.r as u32) << 16) | 
                    ((color.g as u32) << 8) | 
                    (color.b as u32);
                    
        for i in 0..self.buffer.len() {
            self.buffer[i] = pixel;
        }
        
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn fill_rect(&mut self, rect: Rectangle, color: Color) -> Result<()> {
        // Simple implementation by setting pixels directly
        let x_start = rect.x.max(0) as u32;
        let y_start = rect.y.max(0) as u32;
        let x_end = (rect.x + rect.width as i32).min(self.width as i32) as u32;
        let y_end = (rect.y + rect.height as i32).min(self.height as i32) as u32;
        
        for y in y_start..y_end {
            for x in x_start..x_end {
                self.set_pixel(x, y, color);
            }
        }
        
        Ok(())
    }
    
    fn draw_rect(&mut self, rect: Rectangle, color: Color) -> Result<()> {
        // Draw horizontal lines (top and bottom)
        for x in rect.x.max(0)..(rect.x + rect.width as i32).min(self.width as i32) {
            if rect.y >= 0 && rect.y < self.height as i32 {
                self.set_pixel(x as u32, rect.y as u32, color);
            }
            
            let bottom_y = rect.y + rect.height as i32 - 1;
            if bottom_y >= 0 && bottom_y < self.height as i32 {
                self.set_pixel(x as u32, bottom_y as u32, color);
            }
        }
        
        // Draw vertical lines (left and right)
        for y in rect.y.max(0)..(rect.y + rect.height as i32).min(self.height as i32) {
            if rect.x >= 0 && rect.x < self.width as i32 {
                self.set_pixel(rect.x as u32, y as u32, color);
            }
            
            let right_x = rect.x + rect.width as i32 - 1;
            if right_x >= 0 && right_x < self.width as i32 {
                self.set_pixel(right_x as u32, y as u32, color);
            }
        }
        
        Ok(())
    }
    
    fn draw_line(&mut self, start: Point, end: Point, color: Color) -> Result<()> {
        // Bresenham line algorithm
        let mut x0 = start.x;
        let mut y0 = start.y;
        let x1 = end.x;
        let y1 = end.y;
        
        let dx = (x1 - x0).abs();
        let dy = -(y1 - y0).abs();
        let sx = if x0 < x1 { 1 } else { -1 };
        let sy = if y0 < y1 { 1 } else { -1 };
        let mut err = dx + dy;
        
        loop {
            if x0 >= 0 && x0 < self.width as i32 && y0 >= 0 && y0 < self.height as i32 {
                self.set_pixel(x0 as u32, y0 as u32, color);
            }
            
            if x0 == x1 && y0 == y1 {
                break;
            }
            
            let e2 = 2 * err;
            if e2 >= dy {
                if x0 == x1 {
                    break;
                }
                err += dy;
                x0 += sx;
            }
            
            if e2 <= dx {
                if y0 == y1 {
                    break;
                }
                err += dx;
                y0 += sy;
            }
        }
        
        Ok(())
    }
    
    fn draw_text(&mut self, text: &str, position: Point, style: TextStyle) -> Result<()> {
        // Simple implementation - just log that the function was called
        // In a real implementation, we would need to render the text to the buffer
        log::debug!("SDLSurface::draw_text called with text: {} at position: ({}, {})", 
                  text, position.x, position.y);
                  
        // At least draw a rectangle to indicate where text would be
        let width = text.len() as u32 * 8; // Rough estimate
        let height = match style.font_size {
            FontSize::Small => 12,
            FontSize::Medium => 16,
            FontSize::Large => 20,
            FontSize::ExtraLarge => 24,
            FontSize::Custom(size) => size as u32,
        };
        
        let rect = Rectangle::new(
            position.x - match style.alignment {
                TextAlignment::Left => 0,
                TextAlignment::Center => (width / 2) as i32,
                TextAlignment::Right => width as i32,
            },
            position.y - (height / 2) as i32,
            width,
            height
        );
        
        // Just draw a colored border to show where text would be
        self.draw_rect(rect, style.color)?;
        
        Ok(())
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}

/// Helper function to find a usable system font
fn find_system_font() -> Result<String> {
    // Check for bundled fonts first
    if let Ok(path) = std::env::current_dir() {
        let font_path = path.join("assets/fonts/DejaVuSans.ttf");
        if font_path.exists() {
            return Ok(font_path.to_string_lossy().to_string());
        }
    }
    
    // Check common system font locations
    let possible_fonts = vec![
        // macOS
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        // Linux
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        // Windows
        "C:\\Windows\\Fonts\\arial.ttf",
    ];
    
    for font_path in possible_fonts {
        if Path::new(font_path).exists() {
            return Ok(font_path.to_string());
        }
    }
    
    anyhow::bail!("Could not find a usable system font")
}