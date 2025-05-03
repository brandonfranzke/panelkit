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
.PHONY: all help build-containers local target build-headless run-headless mac-native-placeholder deploy transfer create-service install-service clean

# Default target
all: help

# Help
help:
	@echo "PanelKit - Embedded UI System"
	@echo ""
	@echo "Usage:"
	@echo "  make build-containers    - Build Docker containers for compilation"
	@echo "  make local               - Build for local development in Docker"
	@echo "  make target              - Cross-compile for Raspberry Pi target"
	@echo "  make build-headless      - Build with headless driver (in Docker)"
	@echo "  make run-headless        - Run headless build (in Docker)"
	@echo "  make deploy              - Deploy to target device (uses TARGET_HOST variable)"
	@echo "  make transfer            - Transfer binary to custom target device"
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


# DEPRECATED - Use run-headless instead
run: 
	@echo "WARNING: We recommend using 'make run-headless' instead."
	@echo ""
	@echo "To proceed anyway with 'run', type 'yes'"
	@read -p "Continue? [yes/no]: " CONFIRM; \
	if [ "$$CONFIRM" = "yes" ]; then \
		make local && \
		$(DOCKER_RUN) -it \
			-e RUST_LOG=debug \
			$(DOCKER_NATIVE_IMAGE) sh -c "cargo run --features simulator"; \
	else \
		echo "Cancelled. Consider using 'make run-headless' instead."; \
	fi

# Build the headless version
build-headless: build-containers
	@echo "Building headless version..."
	mkdir -p $(BUILD_DIR)
	$(DOCKER_RUN) -e RUST_LOG=debug $(DOCKER_NATIVE_IMAGE) sh -c "cargo build --features simulator,headless && cp -r target/debug/$(PROJECT_NAME) /src/build/$(PROJECT_NAME)-headless"
	@echo "Build complete: $(BUILD_DIR)/$(PROJECT_NAME)-headless"

# Run the headless version in Docker
run-headless: build-headless
	@echo "Running headless version in Docker container..."
	$(DOCKER_RUN) -e RUST_LOG=debug $(DOCKER_NATIVE_IMAGE) sh -c "cargo run --features simulator,headless"

# Placeholder for future macOS native build implementation
# This will be implemented correctly once we figure out cross-compilation
mac-native-placeholder:
	@echo "Native macOS build not yet implemented correctly"
	@echo "We need to create a proper cross-compilation setup for macOS"
	@echo "This will be implemented in a future update"

# Deploy to target device
deploy: target
	@echo "Deploying to target device..."
	scp $(BUILD_DIR)/$(PROJECT_NAME)-arm $(TARGET_USER)@$(TARGET_HOST):$(TARGET_DIR)/$(PROJECT_NAME)
	ssh $(TARGET_USER)@$(TARGET_HOST) "chmod +x $(TARGET_DIR)/$(PROJECT_NAME)"
	@echo "Deployed to $(TARGET_HOST):$(TARGET_DIR)/$(PROJECT_NAME)"

# Transfer to custom target
transfer: target
	@echo "Transferring to custom target..."
	chmod +x $(SRC_DIR)/scripts/transfer.sh
	$(SRC_DIR)/scripts/transfer.sh

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

