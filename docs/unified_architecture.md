# PanelKit Architecture

This document describes the runtime polymorphism approach and unified rendering architecture used in PanelKit.

## Core Architecture

### 1. Runtime Polymorphism for Platform Selection

PanelKit uses runtime polymorphism through trait objects for platform selection and rendering backends:

```rust
pub fn create_for_platform(title: &str, platform: TargetPlatform) -> Result<Box<dyn RenderingContext>> {
    // Resolve auto platform
    let target = if platform == TargetPlatform::Auto {
        Self::detect_platform()
    } else {
        platform
    };
    
    match target {
        TargetPlatform::Host => Self::create_sdl_backend(title),
        TargetPlatform::Embedded => Self::create_fb_backend(),
        TargetPlatform::Auto => unreachable!("Auto platform should have been resolved"),
    }
}
```

Key features of this approach:
- Dynamic selection of the appropriate implementation at runtime
- Single binary that can adapt to different environments
- Testing across platform variants with the same binary
- Error handling with graceful fallbacks

### 2. Unified Platform Driver Interface

Now, a single `PlatformDriver` trait unifies all platform-specific functionality:

```rust
pub trait PlatformDriver {
    fn init(&mut self, width: u32, height: u32) -> Result<()>;
    fn poll_events(&mut self) -> Result<Vec<crate::event::Event>>;
    fn present(&mut self) -> Result<()>;
    fn dimensions(&self) -> (u32, u32);
    fn create_rendering_context(&mut self) -> Result<Box<dyn RenderingContext>>;
    fn cleanup(&mut self);
}
```

Features:
- Single trait for platform functionality
- Coherent component interactions
- Clear responsibility boundaries
- Extensible for new platforms

### 3. Rendering Abstraction Layer

The rendering abstraction layer decouples UI from specific rendering implementations:

```rust
pub trait RenderingContext {
    fn init(&mut self, width: u32, height: u32) -> Result<()>;
    fn present(&mut self) -> Result<()>;
    fn dimensions(&self) -> (u32, u32);
    fn cleanup(&mut self);
    fn clear(&mut self, color: Color) -> Result<()>;
    fn fill_rect(&mut self, rect: Rectangle, color: Color) -> Result<()>;
    fn draw_rect(&mut self, rect: Rectangle, color: Color) -> Result<()>;
    fn draw_line(&mut self, start: Point, end: Point, color: Color) -> Result<()>;
    fn draw_text(&mut self, text: &str, position: Point, style: TextStyle) -> Result<()>;
    fn draw_button(&mut self, rect: Rectangle, text: &str, bg_color: Color, text_color: Color, border_color: Color) -> Result<()>;
    fn create_surface(&mut self, width: u32, height: u32) -> Result<Box<dyn Surface>>;
    fn as_any(&self) -> &dyn Any;
    fn as_any_mut(&mut self) -> &mut dyn Any;
}
```

Different backends implement this interface:
- `SDLBackend`: For host development using SDL2
- `FramebufferBackend`: For embedded devices using the Linux framebuffer

Benefits:
- Complete decoupling of UI components from rendering backends
- Consistent rendering API across all platforms
- Ability to add new backends without changing UI code
- Simplified testing with mock rendering contexts

### 4. Type-Safe Downcasting

The architecture provides safe type-based downcasting for cases where backend-specific functionality is needed:

```rust
fn as_any(&self) -> &dyn Any;
fn as_any_mut(&mut self) -> &mut dyn Any;
```

Example usage:
```rust
if let Some(sdl_backend) = rendering_context.as_any_mut().downcast_mut::<sdl_backend::SDLBackend>() {
    // Access SDL-specific functionality when available
}
```

This pattern:
- Provides type safety through Rust's downcasting
- Allows specialized functionality when needed
- Maintains abstraction boundaries
- Supports progressive enhancement based on available backends

## Factory Pattern Implementation

The unified architecture makes extensive use of the Factory pattern for creating platform-specific implementations:

### PlatformFactory

```rust
pub struct PlatformFactory;

impl PlatformFactory {
    pub fn create(target_platform: TargetPlatform) -> Result<Box<dyn PlatformDriver>> {
        // Auto-detect platform if requested
        let platform = if target_platform == TargetPlatform::Auto {
            Self::detect_platform()
        } else {
            target_platform
        };
        
        match platform {
            TargetPlatform::Host => Self::create_host_driver(...),
            TargetPlatform::Embedded => Self::create_embedded_driver(),
            TargetPlatform::Auto => unreachable!("Auto platform should have been resolved"),
        }
    }
}
```

### RenderingFactory

```rust
pub struct RenderingFactory;

impl RenderingFactory {
    pub fn create(title: &str) -> Result<Box<dyn RenderingContext>> {
        Self::create_for_platform(title, TargetPlatform::Auto)
    }
    
    pub fn create_for_platform(title: &str, platform: TargetPlatform) -> Result<Box<dyn RenderingContext>> {
        // Platform detection and backend creation
    }
}
```

## Platform Detection

The system can automatically detect the appropriate platform:

```rust
fn detect_platform() -> TargetPlatform {
    // Check if we're likely on an embedded device
    let is_embedded = cfg!(target_arch = "arm") || 
                     std::env::var("PANELKIT_EMBEDDED").is_ok() ||
                     std::path::Path::new("/dev/fb0").exists();
    
    if is_embedded {
        TargetPlatform::Embedded
    } else {
        TargetPlatform::Host
    }
}
```

This provides:
- Dynamic platform detection at runtime
- Environment variable overrides for testing
- Fallback detection based on hardware capabilities

## Graceful Fallbacks

The architecture implements graceful fallbacks when preferred backends are unavailable:

```rust
match rendering_driver::RenderingPlatformDriver::new(app_title, width, height) {
    Ok(driver) => {
        log::info!("Initialized rendering platform driver with SDL2 backend");
        Ok(Box::new(driver))
    },
    Err(e) => {
        // Log the error but provide a helpful message
        log::error!("Failed to initialize SDL2 rendering driver: {:?}", e);
        
        // Fallback to mock driver if SDL2 initialization failed
        log::warn!("Falling back to mock driver due to SDL2 initialization failure");
        let mock_driver = mock::MockDriver::new();
        Ok(Box::new(mock_driver))
    }
}
```

This ensures:
- Improved resilience to missing dependencies
- Better user experience with meaningful error messages
- Ability to run in reduced functionality mode when preferred backends are unavailable

## Benefits of the Unified Architecture

### 1. Simplified Build Process

- Single binary that adapts to different environments
- No need for multiple build configurations
- Reduced build complexity

### 2. Improved Testing

- Ability to test across platform variants with the same binary
- Mock implementations for testing without hardware
- Consistent behavior across environments

### 3. Enhanced Maintainability

- Clear separation of concerns
- Coherent abstraction boundaries
- Simplified component interactions

### 4. Better Error Handling

- Contextual errors with proper error chains
- Graceful fallbacks when preferred backends are unavailable
- Improved diagnostics and logging

### 5. Extensibility

- Easy to add new platform implementations
- UI components independent of platform specifics
- Clear extension points for future enhancements

## Practical Usage

### Application Initialization

```rust
// Application configuration with platform selection
let config = AppConfig {
    title: "PanelKit Demo".to_string(),
    width: 800,
    height: 480,
    target_platform: TargetPlatform::Auto, // Auto-detect platform
};

// Create platform driver using the factory with specified target platform
let platform_driver = platform::PlatformFactory::create(config.target_platform)?;

// Initialize platform driver
platform_driver.init(config.width, config.height)?;

// Create rendering context for UI
let rendering_context = platform_driver.create_rendering_context()?;

// Set rendering context for UI manager
ui_manager.set_rendering_context(rendering_context)?;
```

### UI Implementation

```rust
// UI component using the unified rendering abstraction
impl Renderable for Button {
    fn render(&self, ctx: &mut dyn RenderingContext) -> Result<()> {
        // Adjust colors if pressed
        let fill_color = if self.pressed {
            Color::rgba(
                (self.fill_color.r as u16 * 80 / 100) as u8,
                (self.fill_color.g as u16 * 80 / 100) as u8,
                (self.fill_color.b as u16 * 80 / 100) as u8,
                self.fill_color.a
            )
        } else {
            self.fill_color
        };
        
        // Use the RenderingContext's draw_button method directly
        ctx.draw_button(
            self.bounds, 
            &self.text, 
            fill_color, 
            self.text_color, 
            self.border_color
        )?;
        
        Ok(())
    }
}

// UI Page using the unified rendering abstraction
impl Page for HelloPage {
    fn render(&self, ctx: &mut dyn RenderingContext) -> Result<()> {
        // Clear with background color
        ctx.clear(Color::rgb(240, 240, 240))?;
        
        // Draw title area
        ctx.fill_rect(self.title_area, Color::rgb(30, 120, 60))?;
        
        // Draw title text
        let title_position = Point::new(
            self.title_area.x + (self.title_area.width as i32 / 2),
            self.title_area.y + (self.title_area.height as i32 / 2) - 10
        );
        
        let title_style = TextStyle {
            font_size: FontSize::Large,
            color: Color::rgb(255, 255, 255),
            alignment: TextAlignment::Center,
            bold: false,
            italic: false,
        };
                
        ctx.draw_text("Hello Page", title_position, title_style)?;
        
        Ok(())
    }
}
```

## Future Directions

Building on this unified architecture, future improvements could include:

1. **Complete Framebuffer Backend**: Implement the full framebuffer backend for embedded targets
2. **Additional Rendering Backends**: Add support for other rendering APIs (e.g., OpenGL, Vulkan)
3. **Enhanced Thread Safety**: Make rendering contexts thread-safe for parallel rendering
4. **Offscreen Rendering**: Improve support for offscreen surfaces and composition
5. **Platform-Specific Optimizations**: Add optimized paths for specific hardware while maintaining the abstraction

## Conclusion

The transition from compile-time feature flags to runtime polymorphism and the unification of the rendering architecture has significantly improved PanelKit's flexibility, maintainability, and resilience. These architectural improvements provide a solid foundation for future development while simplifying the current implementation.