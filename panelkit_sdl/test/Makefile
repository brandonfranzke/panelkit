# PanelKit Test Makefile

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -g
CFLAGS += -I../src -I../src/core -I. -Iunity
CFLAGS += -DTEST_BUILD

# SDL flags (needed for some tests)
SDL_CFLAGS = $(shell pkg-config --cflags sdl2 SDL2_ttf 2>/dev/null || echo "-I/opt/homebrew/include/SDL2")
SDL_LIBS = $(shell pkg-config --libs sdl2 SDL2_ttf 2>/dev/null || echo "-L/opt/homebrew/lib -lSDL2 -lSDL2_ttf")

# Zlog flags (for logger)
ZLOG_CFLAGS = $(shell pkg-config --cflags zlog 2>/dev/null || echo "")
ZLOG_LIBS = $(shell pkg-config --libs zlog 2>/dev/null || echo "-lzlog")
CFLAGS += $(ZLOG_CFLAGS)
LIBS = $(SDL_LIBS) $(ZLOG_LIBS)

# Output directory
BUILD_DIR = build
TEST_EXEC = $(BUILD_DIR)/test_panelkit

# Object files - list them explicitly
TEST_OBJS = \
    $(BUILD_DIR)/unity.o \
    $(BUILD_DIR)/test_layout_core.o \
    $(BUILD_DIR)/test_absolute_layout.o \
    $(BUILD_DIR)/test_flex_layout.o \
    $(BUILD_DIR)/test_style_resolution.o \
    $(BUILD_DIR)/error.o \
    $(BUILD_DIR)/test_logger.o \
    $(BUILD_DIR)/layout_core.o \
    $(BUILD_DIR)/layout_absolute.o \
    $(BUILD_DIR)/layout_flex.o \
    $(BUILD_DIR)/widget_layout_adapter.o

# Default target
.PHONY: all
all: $(TEST_EXEC)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Build test executable
$(TEST_EXEC): $(BUILD_DIR) $(TEST_OBJS)
	@echo "Linking test executable..."
	@$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ test_runner.c $(TEST_OBJS) $(LIBS)

# Build Unity
$(BUILD_DIR)/unity.o: unity/unity.c unity/unity.h | $(BUILD_DIR)
	@echo "Building Unity..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Build test files
$(BUILD_DIR)/test_layout_core.o: ui/layout/test_layout_core.c | $(BUILD_DIR)
	@echo "Building $<..."
	@$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_absolute_layout.o: ui/layout/test_absolute_layout.c | $(BUILD_DIR)
	@echo "Building $<..."
	@$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_flex_layout.o: ui/layout/test_flex_layout.c | $(BUILD_DIR)
	@echo "Building $<..."
	@$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_style_resolution.o: ui/style/test_style_resolution.c | $(BUILD_DIR)
	@echo "Building $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Build project source files
$(BUILD_DIR)/error.o: ../src/core/error.c | $(BUILD_DIR)
	@echo "Building $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_logger.o: test_logger.c | $(BUILD_DIR)
	@echo "Building test logger..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/layout_core.o: ../src/ui/layout/layout_core.c | $(BUILD_DIR)
	@echo "Building $<..."
	@$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

$(BUILD_DIR)/layout_absolute.o: ../src/ui/layout/layout_absolute.c | $(BUILD_DIR)
	@echo "Building $<..."
	@$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

$(BUILD_DIR)/layout_flex.o: ../src/ui/layout/layout_flex.c | $(BUILD_DIR)
	@echo "Building $<..."
	@$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

$(BUILD_DIR)/widget_layout_adapter.o: ../src/ui/layout/widget_layout_adapter.c | $(BUILD_DIR)
	@echo "Building $<..."
	@$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

# Run tests
.PHONY: test
test: $(TEST_EXEC)
	@echo "Running tests..."
	@./$(TEST_EXEC)

# Run specific test suites
.PHONY: test-layout
test-layout: $(TEST_EXEC)
	@echo "Running layout tests..."
	@./$(TEST_EXEC) layout

.PHONY: test-style
test-style: $(TEST_EXEC)
	@echo "Running style tests..."
	@./$(TEST_EXEC) style

# Clean
.PHONY: clean
clean:
	@echo "Cleaning test build..."
	@rm -rf $(BUILD_DIR)

# Verbose mode
.PHONY: verbose
verbose: CFLAGS += -DUNITY_VERBOSE
verbose: test

# Help
.PHONY: help
help:
	@echo "PanelKit Test Suite"
	@echo ""
	@echo "Targets:"
	@echo "  all         - Build test executable"
	@echo "  test        - Run all tests"
	@echo "  test-layout - Run layout tests only"
	@echo "  test-style  - Run style tests only"
	@echo "  clean       - Remove build artifacts"
	@echo "  verbose     - Run tests with verbose output"
	@echo "  help        - Show this help"