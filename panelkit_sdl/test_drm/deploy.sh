#!/bin/bash
# Deploy DRM test programs to Raspberry Pi

set -e

# Configuration
PI_HOST="${PI_HOST:-panelkit}"
PI_USER="${PI_USER:-brandon}"
DEPLOY_DIR="/home/${PI_USER}/test_drm"

echo "Deploying to ${PI_USER}@${PI_HOST}:${DEPLOY_DIR}"

# Build if not already built
if [ ! -d "build" ] || [ -z "$(ls -A build)" ]; then
    echo "Building first..."
    ./build.sh
fi

# Create remote directory
ssh "${PI_USER}@${PI_HOST}" "mkdir -p ${DEPLOY_DIR}"

# Copy test programs
echo "Copying test programs..."
scp build/test_drm_basic build/test_drm_buffer build/test_sdl_drm "${PI_USER}@${PI_HOST}:${DEPLOY_DIR}/"

# Copy source files for reference
echo "Copying source files..."
scp *.c "${PI_USER}@${PI_HOST}:${DEPLOY_DIR}/"

# Create run script on target
echo "Creating run script..."
ssh "${PI_USER}@${PI_HOST}" "cat > ${DEPLOY_DIR}/run_tests.sh" << 'EOF'
#!/bin/bash
# Run DRM tests on Raspberry Pi

echo "=== DRM Basic Test ==="
echo "Testing basic DRM device access..."
sudo ./test_drm_basic

echo -e "\n=== DRM Buffer Test ==="
echo "Testing dumb buffer creation..."
sudo ./test_drm_buffer

echo -e "\n=== SDL + DRM Test ==="
echo "Testing SDL with direct DRM output..."
echo "This should display a red screen for 3 seconds..."
sudo ./test_sdl_drm

echo -e "\nAll tests complete!"
EOF

ssh "${PI_USER}@${PI_HOST}" "chmod +x ${DEPLOY_DIR}/run_tests.sh"

echo "Deployment complete!"
echo ""
echo "To run tests on Raspberry Pi:"
echo "  ssh ${PI_USER}@${PI_HOST}"
echo "  cd ${DEPLOY_DIR}"
echo "  ./run_tests.sh"
