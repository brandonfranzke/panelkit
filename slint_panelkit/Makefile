# Minimal Makefile for Slint PanelKit
PROJECT := slint_panelkit
TARGET := aarch64-unknown-linux-gnu

# Development dimensions - override with: make run WIDTH=800 HEIGHT=480
WIDTH ?= 480
HEIGHT ?= 640

# Build for local platform (macOS)
build:
	cargo build --release

# Run locally with dimensions
run:
	cargo run -- --width $(WIDTH) --height $(HEIGHT)

# Cross-compile for RPi using Docker with caching
pi-build:
	# Create docker volume for cargo registry caching (but not target)
	docker volume inspect cargo-registry >/dev/null 2>&1 || docker volume create cargo-registry
	# Build the image
	docker build -t slint-cross .
	# Run without --user to avoid permission issues
	docker run --rm \
		-v $(shell pwd):/project \
		-v cargo-registry:/usr/local/cargo/registry \
		slint-cross
	# Fix permissions on the output
	sudo chown -R $(shell whoami):$(shell id -gn) target/ || true
	# Verify the binary was created
	ls -la $(shell pwd)/target/$(TARGET)/release/$(PROJECT) || echo "Binary not found!"

# Deploy to Raspberry Pi (requires 'panelkit' SSH config)
deploy: pi-build
	# Test SSH connectivity first
	@echo "Testing SSH connection to panelkit..."
	ssh -o ConnectTimeout=5 panelkit "echo SSH connection successful"

	# Create remote directory structure
	ssh panelkit "mkdir -p panelkit/logs"

	# Print debug info before copying
	@echo "Copying from: $(shell pwd)/target/$(TARGET)/release/$(PROJECT)"
	@echo "Copying to: panelkit:panelkit/"
	@ls -la $(shell pwd)/target/$(TARGET)/release/$(PROJECT) || echo "WARNING: Binary not found!"

	# Copy binary with verbose flag to see progress
	scp -v $(shell pwd)/target/$(TARGET)/release/$(PROJECT) panelkit:panelkit/

	# Copy supporting files
	scp deploy/Makefile.target panelkit:panelkit/Makefile
	scp deploy/slint_panelkit.service panelkit:panelkit/$(PROJECT).service

	# Verify deployment
	ssh panelkit "ls -la panelkit/"
	@echo "Deployed. Run 'make setup' on the Pi"

# Clean builds
clean:
	cargo clean
	rm -rf target

.PHONY: build run pi-build deploy clean
