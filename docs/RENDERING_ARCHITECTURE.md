# PanelKit Rendering Architecture

This document describes the rendering architecture for PanelKit, focusing on the abstraction layer that enables clean separation between UI components and platform-specific rendering implementations.

## Architecture Overview

PanelKit uses a layered architecture that provides proper isolation between UI logic and platform implementations:

```
┌───────────────────────────────────────────┐
│ Application Layer                         │
├───────────────────────────────────────────┤
│ UI Components (Pages, Widgets)            │
├───────────────────────────────────────────┤
│ Platform Abstraction (PlatformDriver)     │
├───────────────────────────────────────────┤
│ Rendering Abstraction (RenderingContext)  │
├─────────────────────┬─────────────────────┤
│ SDL2 Backend        │ Framebuffer Backend │
│ (Host Development)  │ (Embedded Targets)  │
└─────────────────────┴─────────────────────┘
```

## Key Interfaces

### 1. Platform Abstraction (`PlatformDriver`)

This layer provides a unified interface for all platform-specific operations:
- Event handling (touch, keyboard, etc.)
- Screen dimensions and properties
- Resource management
- Access to the rendering backend

```rust
pub trait PlatformDriver {
    fn init(&mut self, width: u32, height: u32) -> Result<()>;
    fn poll_events(&mut self) -> Result<Vec<Box<dyn Event>>>;
    fn present(&mut self) -> Result<()>;
    fn dimensions(&self) -> (u32, u32);
    fn create_rendering_context(&mut self) -> Result<Box<dyn RenderingContext>>;
    fn cleanup(&mut self);
}
```

### 2. Rendering Abstraction (`RenderingContext`)

This abstraction layer provides high-level rendering operations without exposing backend-specific details:
- Drawing primitives (rectangles, lines, text)
- Widget rendering (buttons, sliders, etc.)
- Layout management
- Screen management

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

### 3. Backend Implementations

#### SDL2 Backend (Host Development)
- Uses SDL2 for rendering during development
- Handles SDL2-specific events and window management
- Supports developer-friendly features like keyboard shortcuts

```rust
pub struct SDLBackend {
    sdl_context: sdl2::Sdl,
    canvas: Arc<Mutex<Canvas<Window>>>,
    ttf_context: Sdl2TtfContext,
    font_path: String,
    width: u32,
    height: u32,
}
```

#### Framebuffer Backend (Embedded Targets)
- Direct framebuffer access for embedded Linux devices
- Optimized for performance and low resource usage
- Handles touchscreen input via evdev or similar

```rust
pub struct FramebufferBackend {
    width: u32,
    height: u32,
    logger: &'static crate::logging::ComponentLogger,
    // These would be implemented in the actual embedded version
    // fb_device: Option<File>,
    // mmap_data: Option<*mut u8>,
}
```

## Implementation Details

### 1. Rendering Platform Driver

The `RenderingPlatformDriver` acts as a bridge between the platform abstraction and the rendering backend:

```rust
pub struct RenderingPlatformDriver {
    rendering_backend: Box<dyn RenderingBackend>,
    width: u32,
    height: u32,
    title: String,
}
```

This driver implements the `PlatformDriver` trait and delegates to the rendering backend for actual rendering operations.

### 2. Rendering Context Implementation

The rendering subsystem provides implementations of the `RenderingContext` trait:

```rust
// SDL Backend implementation
pub struct SDLBackend {
    sdl_context: sdl2::Sdl,
    canvas: Arc<Mutex<Canvas<Window>>>,
    ttf_context: Sdl2TtfContext,
    font_path: String,
    width: u32,
    height: u32,
}

// Framebuffer Backend implementation
pub struct FramebufferBackend {
    width: u32,
    height: u32,
    logger: &'static crate::logging::ComponentLogger,
    // These would be fully implemented for embedded targets
}
```

This allows UI components to use the `RenderingContext` interface consistently across different rendering backends.

### 3. Rendering Factory

The `RenderingFactory` creates the appropriate rendering backend based on runtime platform detection:

```rust
pub struct RenderingFactory;

impl RenderingFactory {
    /// Create a new rendering context appropriate for the current environment
    pub fn create(title: &str) -> Result<Box<dyn RenderingContext>> {
        Self::create_for_platform(title, TargetPlatform::Auto)
    }
    
    /// Create a new rendering context for a specific platform
    pub fn create_for_platform(title: &str, platform: TargetPlatform) -> Result<Box<dyn RenderingContext>> {
        // Resolve auto platform
        let target = if platform == TargetPlatform::Auto {
            Self::detect_platform()
        } else {
            platform
        };
        
        log::info!("Creating rendering context for target: {:?}", target);
        
        match target {
            TargetPlatform::Host => Self::create_sdl_backend(title),
            TargetPlatform::Embedded => Self::create_fb_backend(),
            TargetPlatform::Auto => unreachable!("Auto platform should have been resolved"),
        }
    }
    
    /// Detect the current platform
    fn detect_platform() -> TargetPlatform {
        // Check if we're likely on an embedded device
        let is_embedded = cfg!(target_arch = "arm") || 
                         std::env::var("PANELKIT_EMBEDDED").is_ok() ||
                         std::path::Path::new("/dev/fb0").exists();
        
        if is_embedded {
            log::info!("Auto-detected embedded platform for rendering");
            TargetPlatform::Embedded
        } else {
            log::info!("Auto-detected host platform for rendering");
            TargetPlatform::Host
        }
    }
}
```

### 4. UI Integration

The UI manager and pages use the rendering abstraction uniformly:

```rust
// UI Manager initialization
self.logger.info("Initializing UI system");

// Register Hello page
self.register_page("hello", Box::new(hello_rendering_page::HelloRenderingPage::new()))
    .context("Failed to register hello page")?;

// Register World page  
self.register_page("world", Box::new(world_rendering_page::WorldRenderingPage::new()))
    .context("Failed to register world page")?;

// Example page implementation using the rendering context
impl Page for HelloRenderingPage {
    fn render(&self, ctx: &mut dyn RenderingContext) -> Result<()> {
        // Draw title bar
        self.draw_title_bar(ctx)?;
        
        // Draw main content
        self.draw_content(ctx)?;
        
        Ok(())
    }

    fn draw_title_bar(&self, ctx: &mut dyn RenderingContext) -> Result<()> {
        // Draw title background
        ctx.clear(Color::rgb(240, 240, 240))?;
        
        // Draw title area
        ctx.fill_rect(self.title_area, Color::rgb(30, 120, 60))?;
        
        // Draw title text
        let title_position = Point::new(
            self.title_area.x + (self.title_area.width as i32 / 2),
            self.title_area.y + (self.title_area.height as i32 / 2) - 10
        );
        
        ctx.draw_text("Hello Page", title_position, 
            TextStyle {
                font_size: FontSize::Large,
                color: Color::rgb(255, 255, 255),
                alignment: TextAlignment::Center,
                bold: false,
                italic: false,
            }
        )?;
        
        Ok(())
    }
}
```

## Data Flow

1. User interacts with the application (touch, keyboard)
2. Backend captures events and converts them to platform-agnostic events
3. Platform driver passes events to the UI layer
4. UI components process events and determine render updates
5. Render commands are sent through the rendering abstraction
6. Backend-specific implementation handles the actual drawing

## Rendering Model

The rendering system uses a simplified retained-mode approach:
- UI components describe what to render (not how)
- The rendering backend determines the optimal way to draw
- Optimizations like dirty region tracking can be implemented at the backend level

## Benefits of This Architecture

1. **Clear Separation of Concerns**
   - UI code doesn't need to know about rendering details
   - Platform-specific code is isolated in backend implementations

2. **Improved Testability**
   - UI components can be tested with mock rendering backends
   - Platform drivers can be tested separately from UI logic

3. **Flexible Backend Support**
   - New rendering backends can be added without affecting UI code
   - Future backends (OpenGL, Vulkan, etc.) can be supported

4. **Simplified Development**
   - Developers focus on UI logic, not rendering details
   - Common rendering patterns are abstracted and reusable

5. **Type-Safe Downcasting**
   - Backend-specific features are accessible through safe downcasting when needed
   - No unsafe code required for backend-specific optimizations

## Runtime Platform Selection

The rendering architecture automatically detects the appropriate platform at runtime. You can override this with environment variables:

```bash
# Force embedded mode even on host systems
PANELKIT_EMBEDDED=1 cargo run

# Force specific backend selection
PANELKIT_PLATFORM=host cargo run
PANELKIT_PLATFORM=embedded cargo run
```

## Key Benefits

The rendering architecture provides several advantages:

1. No dependency on third-party UI frameworks (simpler dependency management)
2. Clean abstraction boundaries between UI and rendering code
3. Complete control over the rendering pipeline
4. Flexible backend implementation options
5. Consistent interface for UI components regardless of backend

This architectural approach provides a solid foundation that can evolve into a production-ready embedded UI framework with the flexibility to support different rendering backends as needed for various target platforms.