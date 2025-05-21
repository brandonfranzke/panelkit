#!/bin/bash

# Script to build PanelKit SDL for ARM64 using Docker

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Building Docker image for cross-compilation...${NC}"
docker build -t panelkit-sdl-cross -f Dockerfile.cross .

echo -e "${YELLOW}Running cross-compilation...${NC}"
docker run --rm -v $(pwd):/project panelkit-sdl-cross

if [ -f "./build-arm64/panelkit" ]; then
    echo -e "${GREEN}Build successful!${NC}"
    echo -e "${YELLOW}Binary location: ./build-arm64/panelkit${NC}"
else
    echo -e "${RED}Build failed. Check output for errors.${NC}"
    exit 1
fi

echo -e "${YELLOW}Creating deployment package...${NC}"
mkdir -p deploy
cp ./build-arm64/panelkit deploy/
cp -r assets deploy/ 2>/dev/null || echo "No assets directory found"

cat > deploy/panelkit.service << EOF
[Unit]
Description=PanelKit SDL Application
After=network.target

[Service]
ExecStart=/usr/local/bin/panelkit
Restart=always
User=root
Environment=SDL_FBDEV=/dev/fb0
Environment=SDL_VIDEODRIVER=fbcon

[Install]
WantedBy=multi-user.target
EOF

echo -e "${YELLOW}Creating deployment instructions...${NC}"
cat > deploy/README.md << EOF
# PanelKit SDL Deployment

## Installation

1. Copy the 'panelkit' binary to the target device:
   \`\`\`
   scp panelkit pi@<TARGET_IP>:/tmp/
   \`\`\`

2. Install the binary on the target:
   \`\`\`
   ssh pi@<TARGET_IP> "sudo mv /tmp/panelkit /usr/local/bin/ && sudo chmod +x /usr/local/bin/panelkit"
   \`\`\`

3. Copy and enable the systemd service:
   \`\`\`
   scp panelkit.service pi@<TARGET_IP>:/tmp/
   ssh pi@<TARGET_IP> "sudo mv /tmp/panelkit.service /etc/systemd/system/ && sudo systemctl daemon-reload && sudo systemctl enable panelkit.service"
   \`\`\`

4. Start the service:
   \`\`\`
   ssh pi@<TARGET_IP> "sudo systemctl start panelkit.service"
   \`\`\`

## Troubleshooting

- Check service status:
  \`\`\`
  systemctl status panelkit.service
  \`\`\`

- View logs:
  \`\`\`
  journalctl -u panelkit.service
  \`\`\`

- For framebuffer access issues, ensure the user has the right permissions:
  \`\`\`
  sudo usermod -a -G video $USER
  sudo chmod 666 /dev/fb0
  \`\`\`
EOF

echo -e "${GREEN}Deployment package created in ./deploy directory${NC}"
echo -e "${GREEN}Cross-compilation completed successfully!${NC}"