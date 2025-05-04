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
│ Rendering Abstraction (RenderingBackend)  │
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
    fn poll_events(&mut self) -> Result<Vec<crate::event::Event>>;
    fn present(&mut self) -> Result<()>;
    fn dimensions(&self) -> (u32, u32);
    fn create_graphics_context(&mut self) -> Result<Box<dyn GraphicsContext>>;
    fn cleanup(&mut self);
}
```

### 2. Rendering Abstraction (`RenderingBackend`)

This abstraction layer provides high-level rendering operations without exposing backend-specific details:
- Drawing primitives (rectangles, lines, text)
- Widget rendering (buttons, sliders, etc.)
- Layout management
- Screen management

```rust
pub trait RenderingBackend {
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

### 2. Rendering Graphics Context

The `RenderingGraphicsContext` provides a compatibility layer that implements the `GraphicsContext` trait using the rendering backend:

```rust
pub struct RenderingGraphicsContext {
    pub backend: Box<dyn RenderingBackend>,
}
```

This allows UI components to use either the `GraphicsContext` interface or the rendering backend directly.

### 3. Rendering Factory

The `RenderingFactory` creates the appropriate rendering backend based on the environment:

```rust
pub struct RenderingFactory;

impl RenderingFactory {
    pub fn create(title: &str) -> Result<Box<dyn RenderingBackend>> {
        #[cfg(feature = "host")]
        {
            let backend = sdl_backend::SDLBackend::new(title)?;
            return Ok(Box::new(backend));
        }
        
        #[cfg(feature = "embedded")]
        {
            let backend = fb_backend::FramebufferBackend::new()?;
            return Ok(Box::new(backend));
        }
        
        // This should be unreachable due to the cfg blocks above
        #[allow(unreachable_code)]
        {
            anyhow::bail!("No rendering backend available for current configuration")
        }
    }
}
```

### 4. UI Integration

The UI manager and pages can use the rendering abstraction:

```rust
// UI Manager initialization
#[cfg(not(feature = "use_lvgl"))]
{
    // Check for PANELKIT_USE_RENDERING env variable 
    // to enable the rendering abstraction-based pages
    if std::env::var("PANELKIT_USE_RENDERING").is_ok() {
        self.logger.info("Using rendering abstraction-based pages");
        
        // Register Hello page using rendering abstraction
        self.register_page("hello", Box::new(HelloPage::new()))
            .context("Failed to register hello page")?;
        
        // Register World page using rendering abstraction  
        self.register_page("world", Box::new(WorldPage::new()))
            .context("Failed to register world page")?;
    } else {
        // Use standard pages
        // ...
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

## Enabling the Rendering Architecture

To enable the rendering abstraction:

```bash
PANELKIT_USE_RENDERING=1 cargo run
```

## Key Benefits

The rendering architecture provides several advantages:

1. No dependency on third-party UI frameworks (simpler dependency management)
2. Clean abstraction boundaries between UI and rendering code
3. Complete control over the rendering pipeline
4. Flexible backend implementation options

This architectural approach provides a solid foundation that can evolve into a production-ready embedded UI framework.