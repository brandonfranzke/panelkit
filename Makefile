# PanelKit Makefile - Build System
# Provides workflows for local development and Raspberry Pi deployment

# Project variables
PROJECT_NAME := panelkit
SRC_DIR := $(shell pwd)
BUILD_DIR := $(SRC_DIR)/build
TARGET_HOST ?= raspberrypi.local
TARGET_USER ?= pi
TARGET_DIR ?= /home/$(TARGET_USER)/$(PROJECT_NAME)

# Docker settings 
DOCKER_IMAGE := $(PROJECT_NAME)-builder
DOCKER_DOCKERFILE := $(SRC_DIR)/containers/Dockerfile
CARGO_CACHE_VOLUME := $(PROJECT_NAME)-cargo-cache
DOCKER_RUN := docker run --rm -v $(SRC_DIR):/src -v $(CARGO_CACHE_VOLUME):/root/.cargo/registry

# Target architectures
ARM_TARGET := armv7-unknown-linux-gnueabihf

# Primary targets
.PHONY: all help check-deps build run build-container cross-compile deploy clean

# Default target
all: help

# Help message
help:
	@echo "PanelKit - Embedded UI System"
	@echo ""
	@echo "Development targets:"
	@echo "  make check-deps      - Check if required dependencies are installed"
	@echo "  make build           - Build for development (with runtime platform detection)"
	@echo "  make run             - Run the application (auto-detects platform)"
	@echo "  make clean           - Clean build artifacts"
	@echo ""
	@echo "Deployment targets:"
	@echo "  make cross-compile   - Cross-compile for embedded target (Raspberry Pi)"
	@echo "  make deploy          - Deploy to embedded target device"
	@echo "  make create-service  - Create systemd service file"
	@echo "  make install-service - Install systemd service on target"
	@echo ""
	@echo "Configuration:"
	@echo "  TARGET_HOST=${TARGET_HOST}"
	@echo "  TARGET_USER=${TARGET_USER}"
	@echo "  TARGET_DIR=${TARGET_DIR}"

# Check for required dependencies
check-deps:
	@echo "Checking for required dependencies..."
	@which cargo > /dev/null || (echo "Rust/Cargo not found. Install with 'brew install rust'" && exit 1)
	@brew list sdl2 > /dev/null 2>&1 || (echo "SDL2 not found. Install with 'brew install sdl2'" && exit 1)
	@which docker > /dev/null || (echo "Docker not found. Install Docker Desktop for Mac" && exit 1)
	@echo "✅ All dependencies are installed"

# Build Docker container (used for cross-compilation)
build-container: check-deps
	@echo "Building Docker container for cross-compilation..."
	@docker volume create $(CARGO_CACHE_VOLUME) > /dev/null || true
	@docker build -t $(DOCKER_IMAGE) -f $(DOCKER_DOCKERFILE) .
	@echo "✅ Docker container built successfully"

# Build for development (with runtime platform detection)
build: check-deps
	@echo "Building application with runtime platform detection..."
	@mkdir -p $(BUILD_DIR)
	@RUSTFLAGS="-C link-args=-Wl,-rpath,/opt/homebrew/lib -L/opt/homebrew/lib" \
		LIBRARY_PATH="/opt/homebrew/lib" \
		cargo build
	@cp target/debug/$(PROJECT_NAME) $(BUILD_DIR)/$(PROJECT_NAME)
	@chmod +x $(BUILD_DIR)/$(PROJECT_NAME)
	@echo "✅ Build complete: $(BUILD_DIR)/$(PROJECT_NAME)"

# Run the application
run: build
	@echo "Running application (auto-detecting platform)..."
	@DYLD_LIBRARY_PATH=/opt/homebrew/lib RUST_LOG=debug $(BUILD_DIR)/$(PROJECT_NAME) --platform auto

# Cross-compile for embedded target
cross-compile: build-container
	@echo "Cross-compiling for embedded target..."
	@mkdir -p $(BUILD_DIR)
	@$(DOCKER_RUN) $(DOCKER_IMAGE) \
		bash -c "cargo build --target=$(ARM_TARGET) --release"
	@$(DOCKER_RUN) $(DOCKER_IMAGE) cp -r /src/target/$(ARM_TARGET)/release/$(PROJECT_NAME) /src/build/$(PROJECT_NAME)-arm
	@chmod +x $(BUILD_DIR)/$(PROJECT_NAME)-arm
	@echo "✅ Cross-compilation complete: $(BUILD_DIR)/$(PROJECT_NAME)-arm"

# Deploy to target device
deploy: cross-compile
	@echo "Deploying to target device..."
	@ssh $(TARGET_USER)@$(TARGET_HOST) "mkdir -p $(TARGET_DIR)" || \
		(echo "❌ Failed to connect to $(TARGET_HOST). Check your connection and try again." && exit 1)
	@scp $(BUILD_DIR)/$(PROJECT_NAME)-arm $(TARGET_USER)@$(TARGET_HOST):$(TARGET_DIR)/$(PROJECT_NAME)
	@ssh $(TARGET_USER)@$(TARGET_HOST) "chmod +x $(TARGET_DIR)/$(PROJECT_NAME)"
	@echo "✅ Deployed to $(TARGET_HOST):$(TARGET_DIR)/$(PROJECT_NAME)"
	@echo "   Run on target with: ssh $(TARGET_USER)@$(TARGET_HOST) \"cd $(TARGET_DIR) && RUST_LOG=debug ./$(PROJECT_NAME) --platform auto\""

# Create systemd service file for target
create-service:
	@echo "Creating systemd service file..."
	@mkdir -p $(BUILD_DIR)
	@sed -e "s|__USER__|$(TARGET_USER)|g" \
		-e "s|__WORKDIR__|$(TARGET_DIR)|g" \
		$(SRC_DIR)/config/panelkit.service.template > $(BUILD_DIR)/$(PROJECT_NAME).service
	@echo "✅ Service file created: $(BUILD_DIR)/$(PROJECT_NAME).service"

# Install systemd service on target
install-service: create-service
	@echo "Installing systemd service on target..."
	@scp $(BUILD_DIR)/$(PROJECT_NAME).service $(TARGET_USER)@$(TARGET_HOST):/tmp/
	@ssh $(TARGET_USER)@$(TARGET_HOST) "sudo mv /tmp/$(PROJECT_NAME).service /etc/systemd/system/ && sudo systemctl daemon-reload && sudo systemctl enable $(PROJECT_NAME).service"
	@echo "✅ Service installed and enabled on target device"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@cargo clean 2>/dev/null || true
	@rm -rf $(BUILD_DIR)
	@echo "✅ Clean complete"