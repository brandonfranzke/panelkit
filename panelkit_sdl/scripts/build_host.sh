#!/bin/bash

# Local development build script
# Builds for the host architecture (useful for testing)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Building PanelKit for host development${NC}"

BUILD_DIR="build/host"

echo -e "${YELLOW}Host architecture: $(uname -m)${NC}"
echo -e "${YELLOW}Build directory: $BUILD_DIR${NC}"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Run cmake and build
echo -e "${YELLOW}Configuring build...${NC}"
if cmake ../..; then
    echo -e "${GREEN}Configuration successful${NC}"
else
    echo -e "${RED}Configuration failed${NC}"
    exit 1
fi

echo -e "${YELLOW}Building...${NC}"
if make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4); then
    echo -e "${GREEN}Build successful!${NC}"
    echo -e "${YELLOW}Binary location: ./$BUILD_DIR/panelkit${NC}"
    
    echo -e "${YELLOW}Host development build completed successfully!${NC}"
else
    echo -e "${RED}Build failed${NC}"
    exit 1
fi