#!/bin/bash
# Deploy and run with debug logging

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== PanelKit Deploy and Debug Script ===${NC}"

# Check if binary exists
if [ ! -f "build/panelkit" ]; then
    echo -e "${RED}Error: build/panelkit not found. Run 'make' first.${NC}"
    exit 1
fi

# Deploy
echo -e "${YELLOW}Deploying to target...${NC}"
make deploy

if [ $? -ne 0 ]; then
    echo -e "${RED}Deployment failed${NC}"
    exit 1
fi

echo -e "${GREEN}Deployment successful${NC}"
echo

# Show deployment info
echo -e "${BLUE}Deployment Information:${NC}"
echo "  Binary: /tmp/panelkit/panelkit"
echo "  Config: /tmp/panelkit/config/"
echo "  Service: /tmp/panelkit/panelkit.service"
echo

# Run with debug
echo -e "${YELLOW}Running with debug logging...${NC}"
echo "Use --sdl-drm to force SDL+DRM backend (and evdev input)"
echo

cd /tmp/panelkit

# Set debug environment
export PANELKIT_LOG_LEVEL=DEBUG

# Run with all debug output
echo -e "${BLUE}Full debug output:${NC}"
./panelkit --display-backend sdl_drm 2>&1 | tee panelkit_debug.log

echo
echo -e "${YELLOW}Debug log saved to: /tmp/panelkit/panelkit_debug.log${NC}"
echo
echo -e "${BLUE}To filter for input events only:${NC}"
echo "grep -E '(Input|Touch|evdev|finger|Build|SDL|Device)' /tmp/panelkit/panelkit_debug.log"