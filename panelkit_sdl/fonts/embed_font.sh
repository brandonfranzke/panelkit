#!/bin/bash

# Font Embedding Script
# Converts a TTF font file to C header for embedding

# Configuration
OUTPUT_FILE="embedded_font.h"
VARIABLE_NAME="embedded_font_data"
SIZE_VARIABLE="embedded_font_size"

# Check arguments
if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo "Usage: $0 <font-file.ttf> [output-dir]"
    echo ""
    echo "Available fonts:"
    ls -1 *.ttf 2>/dev/null || echo "  No TTF files found in current directory"
    exit 1
fi

FONT_FILE="$1"
OUTPUT_DIR="${2:-../src}"  # Default to ../src if not provided

# Check if font file exists
if [ ! -f "$FONT_FILE" ]; then
    echo "Error: Font file '$FONT_FILE' not found"
    exit 1
fi

# Verify it's a TTF file
if ! file "$FONT_FILE" | grep -q "TrueType"; then
    echo "Error: '$FONT_FILE' doesn't appear to be a valid TrueType font"
    file "$FONT_FILE"
    exit 1
fi

OUTPUT_PATH="$OUTPUT_DIR/$OUTPUT_FILE"

echo "Converting font '$FONT_FILE' to embedded C header..."
echo "Output: $OUTPUT_PATH"

# Generate the header file
cat > "$OUTPUT_PATH" << EOF
// Embedded font data - auto-generated from $FONT_FILE
// Generated: $(date)

#ifndef EMBEDDED_FONT_H
#define EMBEDDED_FONT_H

static const unsigned char ${VARIABLE_NAME}[] = {
EOF

# Convert font to hex array
xxd -i < "$FONT_FILE" | sed 's/unsigned char stdin\[\]//' | sed 's/^/  /' >> "$OUTPUT_PATH"

# Add closing brace and size constant
cat >> "$OUTPUT_PATH" << EOF
};

static const unsigned int ${SIZE_VARIABLE} = $(wc -c < "$FONT_FILE");

#endif // EMBEDDED_FONT_H
EOF

echo "Font embedded successfully!"
echo "Header file: $OUTPUT_PATH"
echo "Font size: $(wc -c < "$FONT_FILE") bytes"
echo ""
echo "To use in your application:"
echo "  #include \"$OUTPUT_FILE\""
echo "  TTF_OpenFontRW(SDL_RWFromConstMem($VARIABLE_NAME, $SIZE_VARIABLE), 1, size)"