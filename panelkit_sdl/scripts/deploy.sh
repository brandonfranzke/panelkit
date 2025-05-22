#!/bin/bash

# Deploy built binary to target device
# Usage: ./deploy.sh --host <TARGET_HOST> [--user <USER>] [--target-dir <DIRECTORY>]

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

show_usage() {
    echo -e "${RED}Usage: $0 --host <TARGET_HOST> [OPTIONS]${NC}"
    echo ""
    echo "Required:"
    echo "  --host <TARGET_HOST>      SSH host or IP address"
    echo ""
    echo "Optional:"
    echo "  --user <USER>             SSH user (uses SSH config if not specified)"
    echo "  --target-dir <DIRECTORY>  Target directory (default: /tmp/panelkit)"
    echo "  --help                    Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 --host panelkit"
    echo "  $0 --host 192.168.1.100 --user pi"
    echo "  $0 --host panelkit --target-dir /opt/panelkit"
}

# Default values
TARGET_HOST=""
USER=""
TARGET_DIRECTORY="/tmp/panelkit"
BINARY_PATH="build/target/panelkit"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --host)
            TARGET_HOST="$2"
            shift 2
            ;;
        --user)
            USER="$2"
            shift 2
            ;;
        --target-dir)
            TARGET_DIRECTORY="$2"
            shift 2
            ;;
        --help)
            show_usage
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_usage
            exit 1
            ;;
    esac
done

# Validate required arguments
if [ -z "$TARGET_HOST" ]; then
    echo -e "${RED}Error: --host is required${NC}"
    show_usage
    exit 1
fi

# Check if binary exists
if [ ! -f "$BINARY_PATH" ]; then
    echo -e "${RED}Error: Binary not found at $BINARY_PATH${NC}"
    echo "Run 'make target' first to build the ARM64 binary"
    exit 1
fi

# Build SSH target (handle optional user)
if [ -n "$USER" ]; then
    SSH_TARGET="$USER@$TARGET_HOST"
else
    SSH_TARGET="$TARGET_HOST"
fi

echo -e "${YELLOW}Deploying PanelKit to $SSH_TARGET:$TARGET_DIRECTORY${NC}"

# Step 1: Create target directory
echo -e "${YELLOW}1. Creating target directory...${NC}"
if ssh "$SSH_TARGET" "mkdir -p $TARGET_DIRECTORY"; then
    echo -e "${GREEN}Target directory created${NC}"
else
    echo -e "${RED}Failed to create target directory${NC}"
    exit 1
fi

# Step 2: Copy binary
echo -e "${YELLOW}2. Copying binary...${NC}"
if scp "$BINARY_PATH" "$SSH_TARGET:$TARGET_DIRECTORY/"; then
    echo -e "${GREEN}Binary copied successfully${NC}"
else
    echo -e "${RED}Failed to copy binary${NC}"
    exit 1
fi

# Step 3: Copy all deploy/ contents
echo -e "${YELLOW}3. Copying deploy files...${NC}"
if scp -r deploy/* "$SSH_TARGET:$TARGET_DIRECTORY/"; then
    echo -e "${GREEN}Deploy files copied successfully${NC}"
else
    echo -e "${RED}Failed to copy deploy files${NC}"
    exit 1
fi

echo -e "${GREEN}Deployment completed successfully!${NC}"
echo -e "${YELLOW}Files deployed to: $SSH_TARGET:$TARGET_DIRECTORY${NC}"
echo "Binary: $TARGET_DIRECTORY/panelkit"
echo "Service: $TARGET_DIRECTORY/panelkit.service"
echo "README: $TARGET_DIRECTORY/README.md"
echo "Makefile: $TARGET_DIRECTORY/Makefile"