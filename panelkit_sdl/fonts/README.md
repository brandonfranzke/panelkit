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
./embed_font.sh <font-file.ttf>
```

### Script Configuration

Edit these variables at the top of `embed_font.sh` to customize:
- `OUTPUT_DIR` - Where to place the embedded_font.h file
- `OUTPUT_FILE` - Name of the output header file  
- `VARIABLE_NAME` - C variable name for font data
- `SIZE_VARIABLE` - C variable name for font size

## Testing Fonts

To test a font before embedding:
1. Update the `#define FONT_PATH` in `app.c` to point to your font file
2. Build and run: `./build.sh`
3. Once satisfied, embed it with the script above

## Benefits of Embedded Fonts

- **Zero dependencies** - No font files needed on target system
- **Consistent rendering** - Same font across all platforms
- **Single binary** - Self-contained executable
- **Faster startup** - No file I/O for font loading