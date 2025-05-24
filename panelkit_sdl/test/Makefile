# PanelKit Test Suite Makefile
# Builds test utilities for both host and target, with deployment capability

# Configuration
PROJECT_ROOT = ..
TARGET_HOST = panelkit
TARGET_USER = brandon
TARGET_PATH = /tmp/panelkit-tests

# Compiler Configuration (build everything on host)
CC = gcc
CFLAGS = -Wall -Wextra -g -I$(PROJECT_ROOT)/src/core -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lzlog -lpthread

# For simple utilities, static linking helps with deployment
STATIC_CFLAGS = -Wall -Wextra -g -static

# Test Categories and Binaries
CORE_TESTS = test_logger
INPUT_TESTS = test_touch_raw test_sdl_touch test_touch_minimal test_sdl_dummy test_sdl_hints test_manual_inject test_kmsdrm_touch
DISPLAY_TESTS = 
INTEGRATION_TESTS = 

ALL_TESTS = $(CORE_TESTS) $(INPUT_TESTS) $(DISPLAY_TESTS) $(INTEGRATION_TESTS)

# Build directory
BUILD_DIR = build

.PHONY: help clean build deploy-all deploy-input deploy-core deploy-input-dummy run-setup list-devices

# Default target shows help
help:
	@echo "PanelKit Test Suite"
	@echo "==================="
	@echo ""
	@echo "Build targets:"
	@echo "  build             - Build all tests"
	@echo "  build-core        - Build core tests"
	@echo "  build-input       - Build input tests"
	@echo "  build-display     - Build display tests"
	@echo "  build-integration - Build integration tests"
	@echo ""
	@echo "Deployment targets:"
	@echo "  deploy-all        - Deploy all target tests to $(TARGET_USER)@$(TARGET_HOST)"
	@echo "  deploy-core       - Deploy core tests to target"
	@echo "  deploy-input      - Deploy input tests to target"
	@echo "  deploy-input-dummy - Deploy and build SDL dummy driver test"
	@echo "  run-setup         - Setup test environment on target"
	@echo ""
	@echo "Utility targets:"
	@echo "  list-devices      - List input devices on target"
	@echo "  clean             - Clean all build artifacts"
	@echo ""
	@echo "Individual tests:"
	@echo "  Core tests:"
	@echo "    test_logger     - Test logging system"
	@echo "  Input tests:"
	@echo "    test_touch_raw    - Test raw touch input (no SDL)"
	@echo "    test_sdl_touch    - Test SDL touch input handling"
	@echo "    test_touch_minimal - Test touch with PanelKit's exact SDL setup"
	@echo ""
	@echo "Usage examples:"
	@echo "  make build-input && make deploy-input"
	@echo "  make build-input && ./build/test_touch_raw"
	@echo "  make list-devices"
	@echo "  make touch-test    # Build and deploy touch test in one command"
	@echo ""
	@echo "Configuration:"
	@echo "  TARGET_HOST = $(TARGET_HOST)"
	@echo "  TARGET_USER = $(TARGET_USER)"
	@echo "  TARGET_PATH = $(TARGET_PATH)"

# Create build directory
$(BUILD_DIR):
	@mkdir -p $@

# Build targets
build: build-input build-display build-integration

build-core: $(BUILD_DIR)
	@echo "Building core tests..."
	@$(CC) $(CFLAGS) -o $(BUILD_DIR)/test_logger \
		core/test_logger.c $(PROJECT_ROOT)/src/core/logger.c $(LDFLAGS)
	@echo "Core tests built"

build-input: $(BUILD_DIR)
	@echo "Input tests require Linux headers - deploy source and build on target"
	@echo "Use 'make deploy-input-source' to deploy source code to target"

build-display: $(BUILD_DIR)
	@echo "No display tests yet"

build-integration: $(BUILD_DIR)
	@echo "No integration tests yet"

# Deployment targets
deploy-all: build
	@echo "Deploying all tests to $(TARGET_USER)@$(TARGET_HOST):$(TARGET_PATH)..."
	@ssh $(TARGET_USER)@$(TARGET_HOST) "mkdir -p $(TARGET_PATH)"
	@scp $(BUILD_DIR)/* $(TARGET_USER)@$(TARGET_HOST):$(TARGET_PATH)/
	@scp README.md $(TARGET_USER)@$(TARGET_HOST):$(TARGET_PATH)/
	@echo "All tests deployed successfully"
	@echo ""
	@echo "Run tests on target:"
	@echo "  ssh $(TARGET_USER)@$(TARGET_HOST)"
	@echo "  cd $(TARGET_PATH)"
	@echo "  sudo ./test_touch_raw"

deploy-core: build-core
	@echo "Deploying core tests to $(TARGET_USER)@$(TARGET_HOST):$(TARGET_PATH)..."
	@ssh $(TARGET_USER)@$(TARGET_HOST) "mkdir -p $(TARGET_PATH)"
	@scp $(BUILD_DIR)/test_logger $(TARGET_USER)@$(TARGET_HOST):$(TARGET_PATH)/
	@echo "Core tests deployed"

deploy-input-source:
	@echo "Deploying input test sources to $(TARGET_USER)@$(TARGET_HOST):$(TARGET_PATH)..."
	@ssh $(TARGET_USER)@$(TARGET_HOST) "mkdir -p $(TARGET_PATH)"
	@scp input/test_touch_raw.c input/test_touch_minimal.c input/test_sdl_dummy.c input/test_sdl_hints.c input/test_manual_inject.c input/test_kmsdrm_touch.c $(TARGET_USER)@$(TARGET_HOST):$(TARGET_PATH)/
	@ssh $(TARGET_USER)@$(TARGET_HOST) "cd $(TARGET_PATH) && gcc -Wall -Wextra -g -static -o test_touch_raw test_touch_raw.c"
	@echo "Checking for SDL2 development packages on target..."
	@ssh $(TARGET_USER)@$(TARGET_HOST) "pkg-config --exists sdl2 && echo 'SDL2 found' || echo 'SDL2 dev packages not found - installing...'"
	@ssh $(TARGET_USER)@$(TARGET_HOST) "command -v apt >/dev/null && sudo apt update && sudo apt install -y libsdl2-dev || echo 'Could not install SDL2 dev packages'"
	@ssh $(TARGET_USER)@$(TARGET_HOST) "cd $(TARGET_PATH) && gcc -Wall -Wextra -g -o test_touch_minimal test_touch_minimal.c \$$(pkg-config --cflags --libs sdl2) || echo 'Failed to build SDL test - SDL2 dev packages may not be available'"
	@ssh $(TARGET_USER)@$(TARGET_HOST) "cd $(TARGET_PATH) && gcc -Wall -Wextra -g -o test_sdl_dummy test_sdl_dummy.c \$$(pkg-config --cflags --libs sdl2) || echo 'Failed to build SDL dummy test'"
	@ssh $(TARGET_USER)@$(TARGET_HOST) "cd $(TARGET_PATH) && gcc -Wall -Wextra -g -o test_sdl_hints test_sdl_hints.c \$$(pkg-config --cflags --libs sdl2) || echo 'Failed to build SDL hints test'"
	@ssh $(TARGET_USER)@$(TARGET_HOST) "cd $(TARGET_PATH) && gcc -Wall -Wextra -g -o test_manual_inject test_manual_inject.c \$$(pkg-config --cflags --libs sdl2) -lpthread || echo 'Failed to build manual inject test'"
	@ssh $(TARGET_USER)@$(TARGET_HOST) "cd $(TARGET_PATH) && gcc -Wall -Wextra -g -o test_kmsdrm_touch test_kmsdrm_touch.c \$$(pkg-config --cflags --libs sdl2) || echo 'Failed to build KMSDRM touch test'"
	@echo "Input tests built and deployed on target"
	@echo ""
	@echo "Test touch input on target:"
	@echo "  ssh $(TARGET_USER)@$(TARGET_HOST)"
	@echo "  cd $(TARGET_PATH)"
	@echo "  sudo ./test_touch_raw      # Raw Linux input events (confirmed working)"
	@echo "  ./test_touch_minimal       # SDL with PanelKit's exact setup"
	@echo "  ./test_sdl_dummy           # SDL with dummy video driver"
	@echo "  ./test_sdl_hints           # SDL with various driver configurations"
	@echo "  sudo ./test_manual_inject  # Manual input injection into SDL"
	@echo "  ./test_kmsdrm_touch        # SDL with KMSDRM driver (WORKING!)"

deploy-input: deploy-input-source

ensure-target-sdl:
	@echo "Checking for SDL2 development packages on target..."
	@ssh $(TARGET_USER)@$(TARGET_HOST) "pkg-config --exists sdl2 && echo 'SDL2 found' || (echo 'SDL2 dev packages not found - installing...' && command -v apt >/dev/null && sudo apt update && sudo apt install -y libsdl2-dev)"

deploy-input-dummy: ensure-target-sdl
	@echo "Deploying SDL dummy driver test source to target..."
	@ssh $(TARGET_USER)@$(TARGET_HOST) "mkdir -p $(TARGET_PATH)"
	@scp input/test_sdl_dummy.c $(TARGET_USER)@$(TARGET_HOST):$(TARGET_PATH)/
	@echo "Building test_sdl_dummy on target..."
	@ssh $(TARGET_USER)@$(TARGET_HOST) "cd $(TARGET_PATH) && gcc -Wall -Wextra -g -o test_sdl_dummy test_sdl_dummy.c $$(pkg-config --cflags --libs sdl2)"
	@echo "SDL dummy driver test deployed!"
	@echo ""
	@echo "Run test on target:"
	@echo "  ssh $(TARGET_USER)@$(TARGET_HOST)"
	@echo "  cd $(TARGET_PATH)"
	@echo "  ./test_sdl_dummy"

# Utility targets
run-setup:
	@echo "Setting up test environment on $(TARGET_USER)@$(TARGET_HOST)..."
	@ssh $(TARGET_USER)@$(TARGET_HOST) "mkdir -p $(TARGET_PATH)"
	@ssh $(TARGET_USER)@$(TARGET_HOST) "sudo chmod 755 $(TARGET_PATH)"
	@echo "Test environment ready"

list-devices:
	@echo "Listing input devices on $(TARGET_USER)@$(TARGET_HOST)..."
	@ssh $(TARGET_USER)@$(TARGET_HOST) "ls -la /dev/input/event* 2>/dev/null || echo 'No event devices found'"
	@echo ""
	@echo "Checking for touch devices..."
	@ssh $(TARGET_USER)@$(TARGET_HOST) "for dev in /dev/input/event*; do [ -e \"\$$dev\" ] && echo \"Device: \$$dev\" && sudo timeout 1 hexdump -C \"\$$dev\" 2>/dev/null | head -1 || echo \"  (no data or permission denied)\"; done"

# Individual test targets
test_logger: build-core

test_touch_raw: build-input

# Clean build artifacts
clean:
	@echo "Cleaning test build artifacts..."
	@rm -rf build/
	@echo "Test artifacts cleaned"

# Quick deployment shortcuts
deploy-touch: deploy-input
	@echo "Touch test deployed. To run:"
	@echo "  ssh $(TARGET_USER)@$(TARGET_HOST) 'cd $(TARGET_PATH) && sudo ./test_touch_raw'"

touch-test: deploy-input
	@echo "Touch test deployed and built on target!"
	@echo "Connect to target and run: sudo $(TARGET_PATH)/test_touch_raw"