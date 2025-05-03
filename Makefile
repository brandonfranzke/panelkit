# PanelKit Makefile
# Orchestrates build and deployment workflows

# Variables
PROJECT_NAME := panelkit
SRC_DIR := $(shell pwd)
BUILD_DIR := $(SRC_DIR)/build
TARGET_HOST ?= raspberrypi.local
TARGET_USER ?= pi
TARGET_DIR ?= /home/$(TARGET_USER)/$(PROJECT_NAME)

# Docker settings
DOCKER_NATIVE_IMAGE := $(PROJECT_NAME)-builder-native
DOCKER_CROSS_IMAGE := $(PROJECT_NAME)-builder-cross
DOCKER_NATIVE_DOCKERFILE := $(SRC_DIR)/containers/Dockerfile.native
DOCKER_CROSS_DOCKERFILE := $(SRC_DIR)/containers/Dockerfile.cross
CARGO_CACHE_VOLUME := $(PROJECT_NAME)-cargo-cache
DOCKER_RUN := docker run --rm -v $(SRC_DIR):/src -v $(CARGO_CACHE_VOLUME):/root/.cargo/registry

# Cargo settings
CARGO_NATIVE := cargo
CARGO_ARM := cargo build --target=armv7-unknown-linux-gnueabihf

# Declare phony targets
.PHONY: all help build-containers local mac target run run-mac deploy create-service install-service clean

# Default target
all: help

# Help
help:
	@echo "PanelKit - Embedded UI System"
	@echo ""
	@echo "Usage:"
	@echo "  make build-containers    - Build Docker containers for compilation"
	@echo "  make local               - Build for local testing (run in Docker with X11)"
	@echo "  make mac                 - Build for macOS (in Docker, run natively)"
	@echo "  make target              - Cross-compile for Raspberry Pi target"
	@echo "  make run                 - Run with X11 forwarding (needs XQuartz)"
	@echo "  make run-mac             - Run natively on macOS (no X11 needed)"
	@echo "  make deploy              - Deploy to target device"
	@echo "  make clean               - Clean build artifacts"
	@echo "  make help                - Show this help message"

# Build Docker containers
build-containers:
	@echo "Building Docker containers..."
	docker volume create $(CARGO_CACHE_VOLUME) || true
	docker build -t $(DOCKER_NATIVE_IMAGE) -f $(DOCKER_NATIVE_DOCKERFILE) .
	docker build -t $(DOCKER_CROSS_IMAGE) -f $(DOCKER_CROSS_DOCKERFILE) .

# Build for local development
local: build-containers
	@echo "Building for local development..."
	mkdir -p $(BUILD_DIR)
	$(DOCKER_RUN) -e DEP_LV_CONFIG_PATH=/src/config/lvgl $(DOCKER_NATIVE_IMAGE) sh -c "cargo build --features simulator"
	@echo "Build complete in Docker container"

# Cross-compile for ARM target
target: build-containers
	@echo "Cross-compiling for Raspberry Pi target..."
	mkdir -p $(BUILD_DIR)
	$(DOCKER_RUN) -e DEP_LV_CONFIG_PATH=/src/config/lvgl $(DOCKER_CROSS_IMAGE) sh -c "$(CARGO_ARM) --features target --release && cp -r target/armv7-unknown-linux-gnueabihf/release/$(PROJECT_NAME) /src/build/$(PROJECT_NAME)-arm"
	@echo "Build complete: $(BUILD_DIR)/$(PROJECT_NAME)-arm"

# Build for macOS within Docker but run natively
mac: build-containers
	@echo "Building for macOS within Docker..."
	mkdir -p $(BUILD_DIR)
	$(DOCKER_RUN) -e DEP_LV_CONFIG_PATH=/src/config/lvgl \
		-e LIBCLANG_PATH=/usr/lib/llvm/lib \
		-e LLVM_CONFIG_PATH=/usr/bin/llvm-config \
		$(DOCKER_NATIVE_IMAGE) sh -c "cargo build --features simulator && cp -r target/debug/$(PROJECT_NAME) /src/build/"
	@echo "Build complete: $(BUILD_DIR)/$(PROJECT_NAME)"

# Run macOS-compatible build
run-mac: mac
	@echo "Running macOS-compatible build..."
	RUST_LOG=debug $(BUILD_DIR)/$(PROJECT_NAME)

# Run local build for testing (in Docker)
run: local
	@echo "Running local build inside Docker container..."
	$(DOCKER_RUN) -it --net=host -e DISPLAY=$$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix $(DOCKER_NATIVE_IMAGE) sh -c "RUST_LOG=debug /src/target/debug/$(PROJECT_NAME)"

# Deploy to target device
deploy: target
	@echo "Deploying to target device..."
	scp $(BUILD_DIR)/$(PROJECT_NAME)-arm $(TARGET_USER)@$(TARGET_HOST):$(TARGET_DIR)/$(PROJECT_NAME)
	ssh $(TARGET_USER)@$(TARGET_HOST) "chmod +x $(TARGET_DIR)/$(PROJECT_NAME)"
	@echo "Deployed to $(TARGET_HOST):$(TARGET_DIR)/$(PROJECT_NAME)"

# Create systemd service file for target
create-service:
	@echo "Creating systemd service file..."
	@mkdir -p $(BUILD_DIR)
	@sed -e "s|__USER__|$(TARGET_USER)|g" \
		-e "s|__WORKDIR__|$(TARGET_DIR)|g" \
		$(SRC_DIR)/config/panelkit.service.template > $(BUILD_DIR)/$(PROJECT_NAME).service
	@echo "Service file created: $(BUILD_DIR)/$(PROJECT_NAME).service"

# Install systemd service on target
install-service: create-service
	@echo "Installing systemd service on target..."
	scp $(BUILD_DIR)/$(PROJECT_NAME).service $(TARGET_USER)@$(TARGET_HOST):/tmp/
	ssh $(TARGET_USER)@$(TARGET_HOST) "sudo mv /tmp/$(PROJECT_NAME).service /etc/systemd/system/ && sudo systemctl daemon-reload && sudo systemctl enable $(PROJECT_NAME).service"
	@echo "Service installed and enabled on target device"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	-docker run --rm -v $(SRC_DIR):/src $(DOCKER_NATIVE_IMAGE) sh -c "cargo clean" 2>/dev/null || true
	@echo "Clean complete"