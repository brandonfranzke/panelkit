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
FONT_GEN_DIR    = $(FONTS_DIR)/generated
FONT_HEADER     = $(FONT_GEN_DIR)/embedded_font.h
FONT_SOURCE     = $(FONTS_DIR)/$(DEFAULT_FONT)

# Deployment Configuration
TARGET_HOST     = panelkit
TARGET_USER     = 
TARGET_PATH     = /tmp/panelkit

# Display Configuration for host testing
DISPLAY_WIDTH   = 
DISPLAY_HEIGHT  = 
PORTRAIT        = 1
DISPLAY_BACKEND = 

.PHONY: help clean host target deploy run

# Default target shows help
help:
	@echo "PanelKit SDL Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  help    - Show this help message (default)"
	@echo "  host    - Build for host development/testing"
	@echo "  target  - Cross-compile for target device ($(TARGET_ARCH))"
	@echo "  run     - Build and run on host"
	@echo "  deploy  - Deploy to target device ($(TARGET_USER)@$(TARGET_HOST))"
	@echo "  clean   - Clean all build artifacts"
	@echo ""
	@echo "Configuration:"
	@echo "  PROJECT_NAME = $(PROJECT_NAME)"
	@echo "  TARGET_HOST  = $(TARGET_HOST)"
	@echo "  TARGET_USER  = $(TARGET_USER)"
	@echo "  TARGET_ARCH  = $(TARGET_ARCH)"
	@echo "  HOST_ARCH    = $(HOST_ARCH)"
	@echo ""
	@echo "Display Configuration (for 'make run'):"
	@echo "  DISPLAY_WIDTH   = $(DISPLAY_WIDTH)"
	@echo "  DISPLAY_HEIGHT  = $(DISPLAY_HEIGHT)"
	@echo "  PORTRAIT        = $(PORTRAIT)"
	@echo "  DISPLAY_BACKEND = $(DISPLAY_BACKEND)"
	@echo ""
	@echo "Build outputs:"
	@echo "  Host:   ./$(BUILD_DIR)/host/$(PROJECT_NAME)"
	@echo "  Target: ./$(BUILD_DIR)/target/$(PROJECT_NAME)"
	@echo ""
	@echo "Display testing examples:"
	@echo "  make run                                    # Default (640x480)"
	@echo "  make run PORTRAIT=1                         # Portrait mode (480x640)"
	@echo "  make run DISPLAY_WIDTH=800 DISPLAY_HEIGHT=600  # Custom size"
	@echo "  make run DISPLAY_BACKEND=sdl                # Force SDL backend"
	@echo ""
	@echo "Other examples:"
	@echo "  make font DEFAULT_FONT=font-sans-dejavu.ttf"
	@echo "  make target TARGET_HOST=192.168.1.100"
	@echo "  make deploy TARGET_USER=brandon"

# Create font generation directory
$(FONT_GEN_DIR):
	@mkdir -p $@

# Generate embedded font header only if source font is newer
$(FONT_HEADER): $(FONT_SOURCE) | $(FONT_GEN_DIR)
	@echo "Embedding font: $(DEFAULT_FONT)"
	@cd $(FONTS_DIR) && ./embed_font.sh $(DEFAULT_FONT) generated

host: $(FONT_HEADER)
	@./$(SCRIPTS_DIR)/build_host.sh

target: $(FONT_HEADER)
	@./$(SCRIPTS_DIR)/build_target.sh

run: host
	@if [ ! -f "$(BUILD_DIR)/host/$(PROJECT_NAME)" ]; then \
		echo "Error: Host binary not found. Run 'make host' first."; \
		exit 1; \
	fi
	$(eval RUN_ARGS := )
	$(if $(DISPLAY_BACKEND),$(eval RUN_ARGS += --display-backend $(DISPLAY_BACKEND)))
	$(if $(DISPLAY_WIDTH),$(eval RUN_ARGS += --width $(DISPLAY_WIDTH)))
	$(if $(DISPLAY_HEIGHT),$(eval RUN_ARGS += --height $(DISPLAY_HEIGHT)))
	$(if $(PORTRAIT),$(eval RUN_ARGS += --portrait))
	@echo "Running $(PROJECT_NAME) on host$(if $(RUN_ARGS), with args:$(RUN_ARGS))..."
	@./$(BUILD_DIR)/host/$(PROJECT_NAME) $(RUN_ARGS)

deploy: target
	@if [ ! -f "$(BUILD_DIR)/target/$(PROJECT_NAME)" ]; then \
		echo "Error: Target binary not found. Run 'make target' first."; \
		exit 1; \
	fi
	@$(SCRIPTS_DIR)/deploy.sh --host $(TARGET_HOST) \
		$(if $(TARGET_USER),--user $(TARGET_USER)) \
		--target-dir $(TARGET_PATH)


clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)/
	@rm -rf $(FONT_GEN_DIR)/
	@echo "Build artifacts cleaned"
