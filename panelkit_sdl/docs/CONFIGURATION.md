# PanelKit Configuration System

PanelKit uses a YAML-based configuration system that supports multiple configuration sources with a clear precedence hierarchy.

## Configuration Files

Configuration files are loaded in the following order (later files override earlier ones):

1. **System Configuration**: `/etc/panelkit/config.yaml`
2. **User Configuration**: `~/.config/panelkit/config.yaml`
3. **Local Configuration**: `./config.yaml`
4. **Command-line Arguments**: `--config-override key=value`

## File Format

Configuration files use YAML format. See `config/config.example.yaml` for a complete example.

## Required Fields

The following fields must be specified (either in configuration files or as defaults):

- `display.width` - Display width in pixels
- `display.height` - Display height in pixels
- `api.base_url` - API endpoint URL
- `logging.file` - Log file path

## Command-line Options

### Configuration Override
```bash
panelkit --config-override display.width=800 --config-override display.height=600
```

### Validate Configuration
```bash
panelkit --validate-config /path/to/config.yaml
```

### Generate Default Configuration
```bash
panelkit --generate-config /path/to/config.yaml
```

## Configuration Sections

### Display
Controls display settings and backend selection.

```yaml
display:
  width: 480          # Display width in pixels
  height: 640         # Display height in pixels
  fullscreen: false   # Fullscreen mode
  vsync: true         # Vertical sync
  backend: "auto"     # Backend: auto, sdl, sdl_drm
```

### Input
Configures input handling and device selection.

```yaml
input:
  source: "auto"              # Input source: auto, sdl_native, evdev
  device_path: "auto"         # Device path or "auto"
  mouse_emulation: false      # Enable mouse-to-touch emulation
  auto_detect_devices: true   # Auto-detect input devices
```

### API
API client configuration for data fetching.

```yaml
api:
  base_url: "https://api.example.com/"
  timeout: 10              # Request timeout in seconds
  auto_refresh: false      # Enable automatic refresh
  refresh_interval: 30     # Refresh interval in seconds
```

### UI
User interface appearance and behavior.

```yaml
ui:
  colors:
    background: "#212121"  # Background color
    primary: "#ffffff"     # Primary text color
    secondary: "#cccccc"   # Secondary text color
    accent: "#00ff00"      # Accent color
    error: "#ff0000"       # Error color
    warning: "#ff9800"     # Warning color
    success: "#4caf50"     # Success color
  
  fonts:
    regular_size: 18       # Regular font size
    large_size: 32         # Large font size
    small_size: 14         # Small font size
    family: "default"      # Font family
  
  animations:
    enabled: true          # Enable animations
    page_transition_ms: 300
    scroll_friction: 0.95
    button_press_scale: 0.95
  
  layout:                    # DEPRECATED - Being replaced by layout engine
    button_padding: 20       # Use LayoutSpec padding instead
    header_height: 60        # Use explicit widget sizing
    margin: 10               # Use LayoutSpec gap/padding
    scroll_threshold: 10     # Input handling, not layout
    swipe_threshold: 50      # Input handling, not layout
```

### Logging
Logging configuration.

```yaml
logging:
  level: "info"              # Log level: debug, info, warn, error
  file: "/var/log/panelkit/panelkit.log"
  max_size: 10485760         # Max log file size in bytes
  max_files: 5               # Number of log files to keep
  console: true              # Also log to console
```

### System
System behavior settings.

```yaml
system:
  startup_page: 0            # Initial page to display
  debug_overlay: false       # Show debug overlay
  allow_exit: true           # Allow exit via UI
  idle_timeout: 0            # Idle timeout in seconds (0=disabled)
  config_check_interval: 0   # Config change check interval (0=disabled)
```

## Color Format

Colors must be specified in hexadecimal format: `#RRGGBB`

Examples:
- `#ffffff` - White
- `#000000` - Black
- `#ff0000` - Red
- `#00ff00` - Green
- `#0000ff` - Blue

## Boolean Values

Boolean values can be specified as:
- `true`, `yes`, `on`, `1` for true
- `false`, `no`, `off`, `0` for false

## Notes

- Unknown configuration keys will generate warnings but won't prevent startup
- Invalid values will fall back to defaults where possible
- Configuration changes require application restart