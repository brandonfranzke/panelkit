#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Building SDL2 from source for target deployment${NC}"

# Build the minimal Docker image (this will take a while)
echo -e "${YELLOW}Building target Docker image...${NC}"
if docker build -f Dockerfile.target -t panelkit-sdl-target .; then
    echo -e "${GREEN}Docker image built successfully!${NC}"
else
    echo -e "${RED}Docker image build failed.${NC}"
    exit 1
fi

# Run the cross-compilation
echo -e "${YELLOW}Running target cross-compilation...${NC}"
if docker run --rm -v "$(pwd):/project" panelkit-sdl-target; then
    echo -e "${GREEN}Build successful!${NC}"
    echo -e "${YELLOW}Binary location: ./build/target/panelkit${NC}"
    
    # Show what libraries the binary depends on
    echo -e "${YELLOW}Checking binary dependencies...${NC}"
    docker run --rm -v "$(pwd):/project" panelkit-sdl-target /bin/bash -c "cd /project && ldd build/target/panelkit"
    
    echo -e "${GREEN}Target cross-compilation completed successfully!${NC}"
else
    echo -e "${RED}Build failed. Check output for errors.${NC}"
    exit 1
fi
