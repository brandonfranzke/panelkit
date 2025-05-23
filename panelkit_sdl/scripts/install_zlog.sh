#!/bin/bash
# Install zlog for development (macOS only)
# For target builds, zlog is statically linked via Docker

set -e

ZLOG_VERSION="1.2.18"
ZLOG_URL="https://github.com/HardySimpson/zlog/archive/refs/tags/${ZLOG_VERSION}.tar.gz"

echo "=== Installing zlog for development ==="

# Check if already installed
if [ -f "/usr/local/include/zlog.h" ] && [ -f "/usr/local/lib/libzlog.dylib" ]; then
    echo "✓ zlog already installed"
    echo "  Header: /usr/local/include/zlog.h"
    echo "  Library: /usr/local/lib/libzlog.dylib"
    exit 0
fi

# macOS only
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "This script is for macOS development only."
    echo "For Linux targets, zlog is statically linked during cross-compilation."
    exit 1
fi

# Try Homebrew first (preferred method)
if command -v brew &> /dev/null; then
    echo "Installing via Homebrew..."
    if brew install zlog; then
        echo "✓ zlog installed successfully via Homebrew"
        exit 0
    else
        echo "Homebrew installation failed, will build from source..."
    fi
fi

# Build from source as fallback
echo "Building zlog ${ZLOG_VERSION} from source..."

# Create temporary directory
TEMP_DIR=$(mktemp -d)
cd "$TEMP_DIR"

# Download and extract
echo "Downloading..."
curl -L "$ZLOG_URL" -o zlog.tar.gz
tar xzf zlog.tar.gz
cd "zlog-${ZLOG_VERSION}"

# Build and install
echo "Building..."
make -j$(sysctl -n hw.ncpu)

echo "Installing (requires sudo)..."
sudo make install

# Clean up
cd /
rm -rf "$TEMP_DIR"

# Verify installation
if [ -f "/usr/local/include/zlog.h" ] && [ -f "/usr/local/lib/libzlog.dylib" ]; then
    echo ""
    echo "✓ zlog ${ZLOG_VERSION} installed successfully!"
    echo "  Header: /usr/local/include/zlog.h"
    echo "  Library: /usr/local/lib/libzlog.dylib"
else
    echo "✗ Installation verification failed"
    exit 1
fi