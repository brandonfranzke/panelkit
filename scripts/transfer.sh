#!/bin/bash
# Simple script to transfer the PanelKit binary to a target device

# Default settings
TARGET_HOST=${TARGET_HOST:-"raspberrypi.local"}
TARGET_USER=${TARGET_USER:-"pi"}
TARGET_DIR=${TARGET_DIR:-"/home/$TARGET_USER/panelkit"}
BUILD_DIR="./build"
BINARY_NAME="panelkit"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    --host=*)
      TARGET_HOST="${1#*=}"
      shift
      ;;
    --user=*)
      TARGET_USER="${1#*=}"
      shift
      ;;
    --dir=*)
      TARGET_DIR="${1#*=}"
      shift
      ;;
    --help)
      echo "Usage: $0 [options]"
      echo ""
      echo "Options:"
      echo "  --host=HOSTNAME    Target hostname or IP address (default: $TARGET_HOST)"
      echo "  --user=USERNAME    Target username (default: $TARGET_USER)"
      echo "  --dir=DIRECTORY    Target directory (default: $TARGET_DIR)"
      echo "  --help             Show this help message"
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done

# Check if the binary exists
if [ ! -f "$BUILD_DIR/$BINARY_NAME-arm" ]; then
    echo "Error: Binary not found at $BUILD_DIR/$BINARY_NAME-arm"
    echo "Please run 'make target' first to build the ARM binary"
    exit 1
fi

# Create target directory if it doesn't exist
echo "Creating target directory on $TARGET_HOST..."
ssh $TARGET_USER@$TARGET_HOST "mkdir -p $TARGET_DIR"

# Transfer the binary
echo "Transferring binary to $TARGET_HOST:$TARGET_DIR..."
scp "$BUILD_DIR/$BINARY_NAME-arm" "$TARGET_USER@$TARGET_HOST:$TARGET_DIR/$BINARY_NAME"

# Make binary executable
echo "Making binary executable..."
ssh $TARGET_USER@$TARGET_HOST "chmod +x $TARGET_DIR/$BINARY_NAME"

echo "Transfer complete!"
echo "You can run the binary on the target device with:"
echo "  ssh $TARGET_USER@$TARGET_HOST \"cd $TARGET_DIR && RUST_LOG=debug ./$BINARY_NAME\""