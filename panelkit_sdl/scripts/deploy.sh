#!/bin/bash

# Deploy built binary to target device
# Usage: ./deploy.sh <TARGET_HOST> [user]

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

if [ $# -lt 1 ]; then
    echo -e "${RED}Usage: $0 <TARGET_HOST> [user]${NC}"
    exit 1
fi

TARGET_HOST="$1"
USER="$2"  # Optional user (may use SSH config)
BINARY_PATH="../build/arm64/panelkit"
SERVICE_PATH="../deploy/panelkit.service"

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

echo -e "${YELLOW}Deploying PanelKit to $SSH_TARGET${NC}"

# Step 1: Copy binary
echo -e "${YELLOW}1. Copying binary...${NC}"
if scp "$BINARY_PATH" "$SSH_TARGET:/tmp/"; then
    echo -e "${GREEN}Binary copied successfully${NC}"
else
    echo -e "${RED}Failed to copy binary${NC}"
    exit 1
fi

# Step 2: Install binary
echo -e "${YELLOW}2. Installing binary...${NC}"
if ssh "$SSH_TARGET" "sudo mv /tmp/panelkit /usr/local/bin/ && sudo chmod +x /usr/local/bin/panelkit"; then
    echo -e "${GREEN}Binary installed successfully${NC}"
else
    echo -e "${RED}Failed to install binary${NC}"
    exit 1
fi

# Step 3: Copy and enable systemd service
echo -e "${YELLOW}3. Installing systemd service...${NC}"
if scp "$SERVICE_PATH" "$SSH_TARGET:/tmp/"; then
    echo -e "${GREEN}Service file copied${NC}"
else
    echo -e "${RED}Failed to copy service file${NC}"
    exit 1
fi

if ssh "$SSH_TARGET" "sudo mv /tmp/panelkit.service /etc/systemd/system/ && sudo systemctl daemon-reload && sudo systemctl enable panelkit.service"; then
    echo -e "${GREEN}Service installed and enabled${NC}"
else
    echo -e "${RED}Failed to install service${NC}"
    exit 1
fi

# Step 4: Start the service
echo -e "${YELLOW}4. Starting service...${NC}"
if ssh "$SSH_TARGET" "sudo systemctl start panelkit.service"; then
    echo -e "${GREEN}Service started successfully${NC}"
else
    echo -e "${RED}Failed to start service${NC}"
    exit 1
fi

# Step 5: Check status
echo -e "${YELLOW}5. Checking service status...${NC}"
ssh "$SSH_TARGET" "systemctl status panelkit.service --no-pager"

echo -e "${GREEN}Deployment completed successfully!${NC}"