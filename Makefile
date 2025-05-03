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
DOCKER_RUN := docker run --rm -v $(SRC_DIR):/src

# Cargo settings
CARGO_NATIVE := cargo
CARGO_ARM := cargo build --target=armv7-unknown-linux-gnueabihf

# Default target
.PHONY: all
all: help

# Help
.PHONY: help
help:
	@echo "PanelKit - Embedded UI System"
	@echo ""
	@echo "Usage:"
	@echo "  make build-containers    - Build Docker containers for compilation"
	@echo "  make local               - Build for local development (macOS)"
	@echo "  make target              - Cross-compile for Raspberry Pi target"
	@echo "  make run                 - Run the local build for testing"
	@echo "  make deploy              - Deploy to target device"
	@echo "  make clean               - Clean build artifacts"
	@echo "  make help                - Show this help message"

# Build Docker containers
.PHONY: build-containers
build-containers:
	@echo "Building Docker containers..."
	docker build -t $(DOCKER_NATIVE_IMAGE) -f $(DOCKER_NATIVE_DOCKERFILE) .
	docker build -t $(DOCKER_CROSS_IMAGE) -f $(DOCKER_CROSS_DOCKERFILE) .

# Build for local development
.PHONY: local
local: build-containers
	@echo "Building for local development..."
	mkdir -p $(BUILD_DIR)
	$(DOCKER_RUN) $(DOCKER_NATIVE_IMAGE) sh -c "cargo build --features simulator && cp -r target/debug/$(PROJECT_NAME) /src/build/"
	@echo "Build complete: $(BUILD_DIR)/$(PROJECT_NAME)"

# Cross-compile for ARM target
.PHONY: target
target: build-containers
	@echo "Cross-compiling for Raspberry Pi target..."
	mkdir -p $(BUILD_DIR)
	$(DOCKER_RUN) $(DOCKER_CROSS_IMAGE) sh -c "$(CARGO_ARM) --features target --release && cp -r target/armv7-unknown-linux-gnueabihf/release/$(PROJECT_NAME) /src/build/$(PROJECT_NAME)-arm"
	@echo "Build complete: $(BUILD_DIR)/$(PROJECT_NAME)-arm"

# Run local build for testing
.PHONY: run
run: local
	@echo "Running local build..."
	$(BUILD_DIR)/$(PROJECT_NAME)

# Deploy to target device
.PHONY: deploy
deploy: target
	@echo "Deploying to target device..."
	scp $(BUILD_DIR)/$(PROJECT_NAME)-arm $(TARGET_USER)@$(TARGET_HOST):$(TARGET_DIR)/$(PROJECT_NAME)
	ssh $(TARGET_USER)@$(TARGET_HOST) "chmod +x $(TARGET_DIR)/$(PROJECT_NAME)"
	@echo "Deployed to $(TARGET_HOST):$(TARGET_DIR)/$(PROJECT_NAME)"

# Create systemd service file for target
.PHONY: create-service
create-service:
	@echo "Creating systemd service file..."
	@echo "[Unit]" > $(BUILD_DIR)/$(PROJECT_NAME).service
	@echo "Description=PanelKit UI Application" >> $(BUILD_DIR)/$(PROJECT_NAME).service
	@echo "After=network.target" >> $(BUILD_DIR)/$(PROJECT_NAME).service
	@echo "" >> $(BUILD_DIR)/$(PROJECT_NAME).service
	@echo "[Service]" >> $(BUILD_DIR)/$(PROJECT_NAME).service
	@echo "Type=simple" >> $(BUILD_DIR)/$(PROJECT_NAME).service
	@echo "User=$(TARGET_USER)" >> $(BUILD_DIR)/$(PROJECT_NAME).service
	@echo "WorkingDirectory=$(TARGET_DIR)" >> $(BUILD_DIR)/$(PROJECT_NAME).service
	@echo "ExecStart=$(TARGET_DIR)/$(PROJECT_NAME)" >> $(BUILD_DIR)/$(PROJECT_NAME).service
	@echo "Restart=on-failure" >> $(BUILD_DIR)/$(PROJECT_NAME).service
	@echo "StandardOutput=journal" >> $(BUILD_DIR)/$(PROJECT_NAME).service
	@echo "StandardError=journal" >> $(BUILD_DIR)/$(PROJECT_NAME).service
	@echo "" >> $(BUILD_DIR)/$(PROJECT_NAME).service
	@echo "[Install]" >> $(BUILD_DIR)/$(PROJECT_NAME).service
	@echo "WantedBy=multi-user.target" >> $(BUILD_DIR)/$(PROJECT_NAME).service
	@echo "Service file created: $(BUILD_DIR)/$(PROJECT_NAME).service"

# Install systemd service on target
.PHONY: install-service
install-service: create-service
	@echo "Installing systemd service on target..."
	scp $(BUILD_DIR)/$(PROJECT_NAME).service $(TARGET_USER)@$(TARGET_HOST):/tmp/
	ssh $(TARGET_USER)@$(TARGET_HOST) "sudo mv /tmp/$(PROJECT_NAME).service /etc/systemd/system/ && sudo systemctl daemon-reload && sudo systemctl enable $(PROJECT_NAME).service"
	@echo "Service installed and enabled on target device"

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	docker run --rm -v $(SRC_DIR):/src $(DOCKER_NATIVE_IMAGE) sh -c "cargo clean"
	@echo "Clean complete"