#!/bin/bash
# Build script for input tests with static SDL

set -e

# Clean previous builds
rm -rf build
mkdir -p build

# Build Docker image if not exists
if ! docker images | grep -q "panelkit-input-test"; then
    echo "Building Docker image..."
    docker build -t panelkit-input-test .
fi

# Run Docker container to build all tests
echo "Building input test programs with static SDL..."
docker run --rm \
    -v "$(pwd):/workspace" \
    -w /workspace \
    panelkit-input-test \
    sh -c "
        echo '=== Building non-SDL test ==='
        echo 'Compiling test_touch_raw...'
        aarch64-linux-gnu-gcc -o build/test_touch_raw test_touch_raw.c \
            -static -Wall -Wextra -g

        echo ''
        echo '=== Building SDL tests with static linking ==='
        
        echo 'Compiling test_touch_minimal...'
        aarch64-linux-gnu-gcc -o build/test_touch_minimal test_touch_minimal.c \
            -I/usr/local/aarch64/include/SDL2 \
            /usr/local/aarch64/lib/libSDL2.a \
            -lm -lpthread -ldl -lrt -static-libgcc \
            -Wall -Wextra -g

        echo 'Compiling test_sdl_dummy...'
        aarch64-linux-gnu-gcc -o build/test_sdl_dummy test_sdl_dummy.c \
            -I/usr/local/aarch64/include/SDL2 \
            /usr/local/aarch64/lib/libSDL2.a \
            -lm -lpthread -ldl -lrt -static-libgcc \
            -Wall -Wextra -g

        echo 'Compiling test_manual_inject...'
        aarch64-linux-gnu-gcc -o build/test_manual_inject test_manual_inject.c \
            -I/usr/local/aarch64/include/SDL2 \
            /usr/local/aarch64/lib/libSDL2.a \
            -lm -lpthread -ldl -lrt -static-libgcc \
            -Wall -Wextra -g

        echo 'Compiling test_sdl_drm_touch (SDL+DRM+Touch combined)...'
        aarch64-linux-gnu-gcc -o build/test_sdl_drm_touch test_sdl_drm_touch.c \
            -I/usr/local/aarch64/include/SDL2 \
            -I/usr/include/aarch64-linux-gnu -I/usr/include/libdrm \
            /usr/local/aarch64/lib/libSDL2.a \
            -ldrm -lm -lpthread -ldl -lrt -static-libgcc \
            -Wall -Wextra -g

        echo ''
        echo 'NOTE: Skipping tests that require KMSDRM (not available in static SDL build)'
        echo '  - test_sdl_hints'
        echo '  - test_kmsdrm_touch'
        echo ''
        echo 'To test these, they must be built on target with system SDL.'
    "

echo ""
echo "Build complete! Binaries in build/"
echo ""
echo "Static SDL tests built:"
ls -la build/

echo ""
echo "To deploy to target:"
echo "  scp build/* brandon@panelkit:/tmp/"
echo ""
echo "To run tests:"
echo "  sudo /tmp/test_touch_raw         # Raw touch events (no SDL)"
echo "  /tmp/test_touch_minimal           # SDL with offscreen driver" 
echo "  /tmp/test_sdl_dummy               # SDL with dummy driver"
echo "  sudo /tmp/test_manual_inject      # Manual touch injection"
echo "  sudo /tmp/test_sdl_drm_touch      # Complete SDL+DRM+Touch solution"