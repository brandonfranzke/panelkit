#!/bin/bash
# Fix lock file version if needed and build for ARM64 target

# Check Cargo.lock version and fix if needed
if grep -q "version = 4" /project/Cargo.lock; then
  echo "Fixing Cargo.lock version from 4 to 3..."
  sed -i 's/^version = 4/version = 3/' /project/Cargo.lock
fi

# Always do a build to ensure we have the latest binary
echo "Building for ARM64 target..."
# Build for ARM64 target with required flags
RUSTFLAGS="-C link-arg=-lgcc" cargo build --release --target aarch64-unknown-linux-gnu --features embedded

# Verify the binary was created
if [ ! -f "/project/target/aarch64-unknown-linux-gnu/release/slint_panelkit" ]; then
  echo "ERROR: Build failed - binary not found"
  exit 1
else
  echo "Build successful"
fi

# Set permissions for target directory
chmod -R 777 /project/target