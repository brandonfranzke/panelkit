# Font Management

This directory contains font files and utilities for the PanelKit application.

## Font Naming Convention

Fonts should follow this naming pattern:
- `font-sans-regular.ttf` - Sans-serif regular weight
- `font-sans-dejavu.ttf` - DejaVu Sans variant
- `font-sans-roboto.ttf` - Roboto variant
- `font-sans-noto.ttf` - Noto Sans variant

## Available Fonts

- **font-sans-regular.ttf** - Liberation Sans (Helvetica-like, recommended)
- **font-sans-dejavu.ttf** - DejaVu Sans (good for embedded)
- **font-sans-roboto.ttf** - Roboto (Google's modern font)
- **font-sans-noto.ttf** - Noto Sans (comprehensive Unicode)

## Font Embedding

To change the embedded font used in the application:

1. Choose your font (or add a new TTF file following the naming convention)
2. Run the embedding script:
   ```bash
   cd fonts
   ./embed_font.sh font-sans-regular.ttf
   ```
3. Rebuild the application

## Embedding Script

The `embed_font.sh` script converts any TTF font to a C header file for embedding:

```bash
./embed_font.sh <font-file.ttf> [output-directory]
```

The script automatically places the output in `../src/` by default, or you can specify a custom output directory.

### Build System Integration

Font embedding is integrated into the main build system:

```bash
# Embed default font
make font

# Embed specific font
make font DEFAULT_FONT=font-sans-dejavu.ttf
```

The font embedding happens automatically as a prerequisite for both host and target builds.

### Script Configuration

The script uses these default settings:
- **Output file**: `embedded_font.h`
- **Variable name**: `embedded_font_data`
- **Size variable**: `embedded_font_size`
- **Output directory**: `../src/` (relative to fonts/ directory)

## Benefits of Embedded Fonts

- **Zero dependencies** - No font files needed on target system
- **Consistent rendering** - Same font across all platforms
- **Single binary** - Self-contained executable
- **Faster startup** - No file I/O for font loading