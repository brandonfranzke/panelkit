# PanelKit Makefile

# Project configuration
PROJECT = slint_panelkit
TARGET = aarch64-unknown-linux-gnu
TARGET_HOST = panelkit
TARGET_DIR = ~/panelkit

# Local configuration variables
WIDTH = 640
HEIGHT = 480
ORIENTATION = portrait
LOG_LEVEL = debug
EXECUTABLE = ./target/debug/$(PROJECT)

# Set DEBUG to 1 for debug overlay, 0 to disable
DEBUG = 1

# Set TOUCH_LOGGING to 1 to enable, 0 to disable
TOUCH_LOGGING = 1

# Define all phony targets in one place
.PHONY: help build run clean target deploy

# Default target shows help
help:
	@echo "PanelKit Makefile"
	@echo "================="
	@echo "make [target] [VAR=value]"
	@echo ""
	@echo "Targets:"
	@echo "  help    Show this help message (default)"
	@echo "  build   Build in debug mode"
	@echo "  run     Build and run in debug mode"
	@echo "  target  Cross-compile for ARM64 (Raspberry Pi)"
	@echo "  deploy  Deploy to target device (requires SSH config)"
	@echo "  clean   Remove build artifacts"
	@echo ""
	@echo "Variables:"
	@echo "  WIDTH=$(WIDTH) HEIGHT=$(HEIGHT) ORIENTATION=$(ORIENTATION)"
	@echo "  LOG_LEVEL=$(LOG_LEVEL) DEBUG=$(DEBUG) TOUCH_LOGGING=$(TOUCH_LOGGING)"
	@echo "  TARGET_HOST=$(TARGET_HOST)"

# Build the application in debug mode
build:
	@echo "Building PanelKit..."
	@cargo build

# Run the application in debug mode
run: build
	@echo "Running PanelKit ($(WIDTH)x$(HEIGHT), $(ORIENTATION))"
	@$(EXECUTABLE) \
		--width $(WIDTH) \
		--height $(HEIGHT) \
		--orientation $(ORIENTATION) \
		--log-level $(LOG_LEVEL) \
		$(shell if [ $(DEBUG) -eq 1 ]; then echo "--debug-overlay"; fi) \
		$(shell if [ $(TOUCH_LOGGING) -eq 1 ]; then echo "--touch-logging"; fi)

# Cross-compile for target architecture using Docker
target:
	@echo "Cross-compiling for $(TARGET)..."
	@docker volume inspect cargo-registry >/dev/null 2>&1 || docker volume create cargo-registry
	@docker volume inspect cargo-target >/dev/null 2>&1 || docker volume create cargo-target
	@docker build -t slint-cross .
	@mkdir -p target/$(TARGET)/release
	@docker run --rm \
		-v $(shell pwd):/project \
		-v cargo-registry:/usr/local/cargo/registry \
		-v cargo-target:/project/target \
		slint-cross bash -c "cd /project && RUSTFLAGS='-C link-arg=-lgcc' cargo build --release --target $(TARGET) --features embedded"

# Deploy to target device
deploy:
	@echo "Deploying to $(TARGET_HOST)..."
	@ssh $(TARGET_HOST) "mkdir -p $(TARGET_DIR)/logs"
	@scp target/$(TARGET)/release/$(PROJECT) $(TARGET_HOST):$(TARGET_DIR)/
	@scp deploy/Makefile.target $(TARGET_HOST):$(TARGET_DIR)/Makefile
	@scp deploy/slint_panelkit.service $(TARGET_HOST):$(TARGET_DIR)/$(PROJECT).service
	@echo "Deployed successfully to $(TARGET_HOST):$(TARGET_DIR)"
	@echo "To complete setup, run: ssh $(TARGET_HOST) \"cd $(TARGET_DIR) && make setup\""

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@cargo clean
