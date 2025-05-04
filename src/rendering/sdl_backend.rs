//! SDL2-based rendering backend
//!
//! This module implements the RenderingBackend trait using SDL2 for host development.

use anyhow::{Result, Context};
use crate::rendering::{RenderingBackend, Surface};
use crate::rendering::primitives::{Color, Point, Rectangle, TextStyle, FontSize, TextAlignment};
use sdl2::event::Event as SdlEvent;
use sdl2::pixels::Color as SdlColor;
use sdl2::rect::{Point as SdlPoint, Rect as SdlRect};
use sdl2::render::{Canvas, Texture, TextureCreator};
use sdl2::ttf::{Font, Sdl2TtfContext};
use sdl2::video::{Window, WindowContext};
use std::any::Any;
use std::path::Path;
use std::sync::{Arc, Mutex};

/// SDL2 implementation of the RenderingBackend trait
pub struct SDLBackend {
    sdl_context: sdl2::Sdl,
    canvas: Arc<Mutex<Canvas<Window>>>,
    ttf_context: Sdl2TtfContext,
    font_path: String,
    width: u32,
    height: u32,
}

impl SDLBackend {
    /// Create a new SDL backend with the given window title
    pub fn new(title: &str) -> Result<Self> {
        // Initialize SDL2
        let sdl_context = sdl2::init()
            .context("Failed to initialize SDL2")?;
            
        let video_subsystem = sdl_context.video()
            .context("Failed to initialize SDL2 video subsystem")?;
        
        // Create window
        let window = video_subsystem
            .window(title, 800, 480)  // Default size, will be updated in init()
            .position_centered()
            .allow_highdpi() // Enable Retina display support
            .build()
            .context("Failed to create SDL2 window")?;
            
        // Create canvas
        let canvas = window
            .into_canvas()
            .accelerated()
            .present_vsync() // Use vsync for smoother rendering
            .build()
            .context("Failed to create SDL2 canvas")?;
            
        // Initialize TTF
        let ttf_context = sdl2::ttf::init()
            .context("Failed to initialize SDL2 TTF")?;
            
        // Find a usable font
        let font_path = find_system_font()?;
        
        Ok(Self {
            sdl_context,
            canvas: Arc::new(Mutex::new(canvas)),
            ttf_context,
            font_path,
            width: 800,
            height: 480,
        })
    }
    
    /// Poll for events and convert them to PanelKit events
    pub fn poll_events(&self) -> Result<Vec<crate::event::Event>> {
        let mut event_pump = self.sdl_context.event_pump()
            .context("Failed to get SDL event pump")?;
            
        let mut events = Vec::new();
        
        for event in event_pump.poll_iter() {
            match event {
                SdlEvent::Quit {..} => {
                    // Map quit event to a custom event
                    events.push(crate::event::Event::Custom {
                        event_type: "quit".to_string(),
                        payload: String::new(),
                    });
                },
                SdlEvent::KeyDown { keycode: Some(key), .. } => {
                    // Map key events
                    let key_str = format!("{:?}", key).to_lowercase();
                    events.push(crate::event::Event::Key {
                        key: key_str,
                        pressed: true,
                    });
                },
                SdlEvent::KeyUp { keycode: Some(key), .. } => {
                    // Map key events
                    let key_str = format!("{:?}", key).to_lowercase();
                    events.push(crate::event::Event::Key {
                        key: key_str,
                        pressed: false,
                    });
                },
                SdlEvent::MouseButtonDown { x, y, mouse_btn, .. } => {
                    // Map mouse events to touch events for simplicity
                    match mouse_btn {
                        sdl2::mouse::MouseButton::Left => {
                            events.push(crate::event::Event::Touch {
                                x,
                                y,
                                action: crate::event::TouchAction::Press,
                            });
                        },
                        _ => {}
                    }
                },
                SdlEvent::MouseButtonUp { x, y, mouse_btn, .. } => {
                    // Map mouse events to touch events for simplicity
                    match mouse_btn {
                        sdl2::mouse::MouseButton::Left => {
                            events.push(crate::event::Event::Touch {
                                x,
                                y,
                                action: crate::event::TouchAction::Release,
                            });
                        },
                        _ => {}
                    }
                },
                SdlEvent::MouseMotion { x, y, mousestate, .. } => {
                    // If mouse button is pressed, send a Move event
                    if mousestate.left() {
                        events.push(crate::event::Event::Touch {
                            x,
                            y,
                            action: crate::event::TouchAction::Move,
                        });
                    }
                },
                _ => {}
            }
        }
        
        Ok(events)
    }
    
    /// Convert a Color to an SDL Color
    fn to_sdl_color(&self, color: Color) -> SdlColor {
        SdlColor::RGBA(color.r, color.g, color.b, color.a)
    }
    
    /// Convert a Point to an SDL Point
    fn to_sdl_point(&self, point: Point) -> SdlPoint {
        SdlPoint::new(point.x, point.y)
    }
    
    /// Convert a Rectangle to an SDL Rect
    fn to_sdl_rect(&self, rect: Rectangle) -> SdlRect {
        SdlRect::new(rect.x, rect.y, rect.width, rect.height)
    }
    
    /// Load a font with the requested size
    fn load_font(&self, size: u16) -> Result<Font> {
        self.ttf_context.load_font(&self.font_path, size)
            .context(format!("Failed to load font {} at size {}", self.font_path, size))
    }
}

impl RenderingBackend for SDLBackend {
    fn init(&mut self, width: u32, height: u32) -> Result<()> {
        self.width = width;
        self.height = height;
        
        // Try to resize the window (this may not work on all platforms)
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
            
        let window = canvas.window_mut();
        window.set_size(width, height)
            .context("Failed to resize SDL window")?;
            
        // Initial clear to gray
        canvas.set_draw_color(SdlColor::RGB(100, 100, 100));
        canvas.clear();
        canvas.present();
        
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
        // SDL2 will handle cleanup automatically when the context is dropped
    }
    
    fn clear(&mut self, color: Color) -> Result<()> {
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
            
        canvas.set_draw_color(self.to_sdl_color(color));
        canvas.clear();
        
        Ok(())
    }
    
    fn fill_rect(&mut self, rect: Rectangle, color: Color) -> Result<()> {
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
            
        canvas.set_draw_color(self.to_sdl_color(color));
        canvas.fill_rect(self.to_sdl_rect(rect))
            .context("Failed to fill rectangle")?;
            
        Ok(())
    }
    
    fn draw_rect(&mut self, rect: Rectangle, color: Color) -> Result<()> {
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
            
        canvas.set_draw_color(self.to_sdl_color(color));
        canvas.draw_rect(self.to_sdl_rect(rect))
            .context("Failed to draw rectangle")?;
            
        Ok(())
    }
    
    fn draw_line(&mut self, start: Point, end: Point, color: Color) -> Result<()> {
        let mut canvas = self.canvas.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL canvas mutex: {}", e))?;
            
        canvas.set_draw_color(self.to_sdl_color(color));
        canvas.draw_line(self.to_sdl_point(start), self.to_sdl_point(end))
            .context("Failed to draw line")?;
            
        Ok(())
    }
    
    fn draw_text(&mut self, text: &str, position: Point, style: TextStyle) -> Result<()> {
        let canvas_lock = self.canvas.lock()
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
            .context("Failed to render text to surface")?;
            
        // Create texture from surface
        let texture = texture_creator.create_texture_from_surface(&surface)
            .context("Failed to create texture from text surface")?;
            
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
        let dest_rect = SdlRect::new(x, position.y, text_width, text_height);
        
        // Draw the text
        let mut canvas = canvas_lock;
        canvas.copy(&texture, None, Some(dest_rect))
            .context("Failed to copy text texture to canvas")?;
            
        Ok(())
    }
    
    fn draw_button(&mut self, rect: Rectangle, text: &str, bg_color: Color, text_color: Color, border_color: Color) -> Result<()> {
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
        // This is a stub implementation that doesn't actually create a usable surface due to lifetime issues
        // In a real implementation, we would refactor to handle the lifetime issues properly
        // For now, return an error to indicate that this functionality is not available
        anyhow::bail!("SDLBackend::create_surface is not implemented due to lifetime issues")
    }
    
    fn as_any(&self) -> &dyn Any {
        self
    }
    
    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}

/// SDL implementation of a rendering surface
pub struct SDLSurface {
    texture: Arc<Mutex<Texture<'static>>>,
    texture_creator: TextureCreator<WindowContext>,
    ttf_context: Sdl2TtfContext,
    font_path: String,
    width: u32,
    height: u32,
}

impl SDLSurface {
    /// Convert a Color to an SDL Color
    fn to_sdl_color(&self, color: Color) -> SdlColor {
        SdlColor::RGBA(color.r, color.g, color.b, color.a)
    }
    
    /// Convert a Point to an SDL Point
    fn to_sdl_point(&self, point: Point) -> SdlPoint {
        SdlPoint::new(point.x, point.y)
    }
    
    /// Convert a Rectangle to an SDL Rect
    fn to_sdl_rect(&self, rect: Rectangle) -> SdlRect {
        SdlRect::new(rect.x, rect.y, rect.width, rect.height)
    }
    
    /// Load a font with the requested size
    fn load_font(&self, size: u16) -> Result<Font> {
        self.ttf_context.load_font(&self.font_path, size)
            .context(format!("Failed to load font {} at size {}", self.font_path, size))
    }
}

impl Surface for SDLSurface {
    fn clear(&mut self, color: Color) -> Result<()> {
        let mut texture = self.texture.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL texture mutex: {}", e))?;
            
        texture.with_lock(None, |buffer: &mut [u8], pitch: usize| {
            // Fill buffer with color
            let stride = pitch / 4; // 4 bytes per pixel (RGBA)
            
            for y in 0..self.height {
                for x in 0..self.width {
                    let offset = (y as usize * stride + x as usize) * 4;
                    buffer[offset] = color.r;
                    buffer[offset + 1] = color.g;
                    buffer[offset + 2] = color.b;
                    buffer[offset + 3] = color.a;
                }
            }
        }).context("Failed to lock texture")?;
        
        Ok(())
    }
    
    fn dimensions(&self) -> (u32, u32) {
        (self.width, self.height)
    }
    
    fn fill_rect(&mut self, rect: Rectangle, color: Color) -> Result<()> {
        let mut texture = self.texture.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL texture mutex: {}", e))?;
            
        // Create a canvas for this texture
        let mut canvas = self.texture_creator.create_renderer(texture.as_mut())?;
        
        canvas.set_draw_color(self.to_sdl_color(color));
        canvas.fill_rect(self.to_sdl_rect(rect))
            .context("Failed to fill rectangle on surface")?;
            
        Ok(())
    }
    
    fn draw_rect(&mut self, rect: Rectangle, color: Color) -> Result<()> {
        let mut texture = self.texture.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL texture mutex: {}", e))?;
            
        // Create a canvas for this texture
        let mut canvas = self.texture_creator.create_renderer(texture.as_mut())?;
        
        canvas.set_draw_color(self.to_sdl_color(color));
        canvas.draw_rect(self.to_sdl_rect(rect))
            .context("Failed to draw rectangle on surface")?;
            
        Ok(())
    }
    
    fn draw_line(&mut self, start: Point, end: Point, color: Color) -> Result<()> {
        let mut texture = self.texture.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL texture mutex: {}", e))?;
            
        // Create a canvas for this texture
        let mut canvas = self.texture_creator.create_renderer(texture.as_mut())?;
        
        canvas.set_draw_color(self.to_sdl_color(color));
        canvas.draw_line(self.to_sdl_point(start), self.to_sdl_point(end))
            .context("Failed to draw line on surface")?;
            
        Ok(())
    }
    
    fn draw_text(&mut self, text: &str, position: Point, style: TextStyle) -> Result<()> {
        let mut texture = self.texture.lock()
            .map_err(|e| anyhow::anyhow!("Failed to lock SDL texture mutex: {}", e))?;
            
        // Create a canvas for this texture
        let mut canvas = self.texture_creator.create_renderer(texture.as_mut())?;
        
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
            .context("Failed to render text to surface")?;
            
        // Create texture from surface
        let text_texture = self.texture_creator.create_texture_from_surface(&surface)
            .context("Failed to create texture from text surface")?;
            
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
        let dest_rect = SdlRect::new(x, position.y, text_width, text_height);
        
        // Draw the text
        canvas.copy(&text_texture, None, Some(dest_rect))
            .context("Failed to copy text texture to surface")?;
            
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
        let font_path = path.join("src/simple_ui/fonts/DejaVuSans.ttf");
        if font_path.exists() {
            return Ok(font_path.to_string_lossy().to_string());
        }
        
        let font_path = path.join("src/simple_ui/assets/fonts/DejaVuSans.ttf");
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