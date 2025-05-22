#!/bin/bash
# Build script for DRM test programs

set -e

# Clean previous builds
rm -rf build
mkdir -p build

# Build Docker image if not exists
if ! docker images | grep -q "panelkit-drm-test"; then
    echo "Building Docker image..."
    docker build -t panelkit-drm-test .
fi

# Run Docker container to build
echo "Building DRM test programs..."
docker run --rm \
    -v "$(pwd):/workspace" \
    -w /workspace \
    panelkit-drm-test \
    sh -c "
        echo 'Compiling test_drm_basic...'
        aarch64-linux-gnu-gcc -o build/test_drm_basic test_drm_basic.c \
            -I/usr/include/aarch64-linux-gnu -I/usr/include/libdrm \
            -ldrm

        echo 'Compiling test_drm_buffer...'
        aarch64-linux-gnu-gcc -o build/test_drm_buffer test_drm_buffer.c \
            -I/usr/include/aarch64-linux-gnu -I/usr/include/libdrm \
            -ldrm

        echo 'Compiling SDL+DRM library and examples...'
        
        # Compile SDL+DRM renderer library
        aarch64-linux-gnu-gcc -c -o build/sdl_drm_renderer.o sdl_drm_renderer.c \
            -I/usr/local/aarch64/include/SDL2 \
            -I/usr/include/aarch64-linux-gnu -I/usr/include/libdrm
        
        # Compile clean example
        aarch64-linux-gnu-gcc -o build/example_usage example_usage.c build/sdl_drm_renderer.o \
            -I/usr/local/aarch64/include/SDL2 \
            -I/usr/include/aarch64-linux-gnu -I/usr/include/libdrm \
            /usr/local/aarch64/lib/libSDL2.a \
            -ldrm -lm -lpthread -ldl -lrt -static-libgcc
        
        # Compile legacy test (for reference)
        aarch64-linux-gnu-gcc -o build/test_sdl_drm test_sdl_drm.c \
            -I/usr/local/aarch64/include/SDL2 \
            -I/usr/include/aarch64-linux-gnu -I/usr/include/libdrm \
            /usr/local/aarch64/lib/libSDL2.a \
            -ldrm -lm -lpthread -ldl -lrt -static-libgcc
    "

echo "Build complete! Binaries in build/"
ls -la build/