# Makefile for PanelKit SDL

# Configuration Variables
PROJECT_NAME    = panelkit
SCRIPTS_DIR     = scripts
BUILD_DIR       = build
DEPLOY_DIR      = deploy
FONTS_DIR       = fonts
SRC_DIR         = src
TARGET_ARCH     = arm64
HOST_ARCH       = $(shell uname -m | sed 's/x86_64/x86_64/' | sed 's/aarch64/arm64/')

# Font Configuration  
DEFAULT_FONT    = font-sans-regular.ttf

# Deployment Configuration
TARGET_HOST     = target
TARGET_USER     = pi
TARGET_PATH     = /usr/local/bin

.PHONY: help clean host target deploy font

# Default target shows help
help:
	@echo "PanelKit SDL Build System"
	@echo ""
	@echo "Configuration:"
	@echo "  PROJECT_NAME = $(PROJECT_NAME)"
	@echo "  TARGET_HOST  = $(TARGET_HOST)"
	@echo "  TARGET_USER  = $(TARGET_USER)"
	@echo "  TARGET_ARCH  = $(TARGET_ARCH)"
	@echo "  HOST_ARCH    = $(HOST_ARCH)"
	@echo ""
	@echo "Available targets:"
	@echo "  help    - Show this help message (default)"
	@echo "  font    - Embed font ($(DEFAULT_FONT)) into C header"
	@echo "  host    - Build for host development/testing"
	@echo "  target  - Cross-compile for target device ($(TARGET_ARCH))"
	@echo "  deploy  - Deploy to target device ($(TARGET_USER)@$(TARGET_HOST))"
	@echo "  clean   - Clean all build artifacts"
	@echo ""
	@echo "Build outputs:"
	@echo "  Host:   ./$(BUILD_DIR)/$(HOST_ARCH)/$(PROJECT_NAME)"
	@echo "  Target: ./$(BUILD_DIR)/$(TARGET_ARCH)/$(PROJECT_NAME)"
	@echo ""
	@echo "To customize variables, edit this Makefile or use:"
	@echo "  make font DEFAULT_FONT=font-sans-dejavu.ttf"
	@echo "  make target TARGET_HOST=192.168.1.100"
	@echo "  make deploy TARGET_USER=brandon"

font:
	@echo "Embedding font: $(DEFAULT_FONT)"
	@cd $(FONTS_DIR) && ./embed_font.sh $(DEFAULT_FONT) ../$(SRC_DIR)

host: font
	@./$(SCRIPTS_DIR)/build_host.sh

target: font
	@./$(SCRIPTS_DIR)/build_target.sh

deploy: target
	@if [ ! -f "$(BUILD_DIR)/$(TARGET_ARCH)/$(PROJECT_NAME)" ]; then \
		echo "Error: Target binary not found. Run 'make target' first."; \
		exit 1; \
	fi
	@./$(SCRIPTS_DIR)/deploy.sh $(TARGET_HOST) $(TARGET_USER)

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)/
	@echo "Build artifacts cleaned"