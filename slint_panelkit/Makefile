# Simple Makefile for PanelKit LVGL

# Default build directory
BUILD_DIR = build

# Default to SDL build
SDL_BUILD ?= 1

# Detect OS
ifeq ($(shell uname), Darwin)
	# macOS specific settings
	CMAKE_EXTRA = -DCMAKE_PREFIX_PATH=$(shell brew --prefix sdl2)
else
	# Linux specific settings
	CMAKE_EXTRA =
endif

# Targets
.PHONY: all clean build run prepare rebuild

all: build

prepare:
	@mkdir -p $(BUILD_DIR)

build: prepare
	@echo "Building PanelKit with LVGL..."
	@cd $(BUILD_DIR) && cmake $(CMAKE_EXTRA) .. && make -j

run: build
	@echo "Running PanelKit..."
	@cd $(BUILD_DIR) && ./panelkit

clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)

rebuild: clean build

# Docker targets for cross-compilation
.PHONY: docker-build docker-image docker-run

# Docker image name
DOCKER_IMAGE = panelkit-build:latest

docker-image:
	@echo "Building Docker image for cross-compilation..."
	@docker build -t $(DOCKER_IMAGE) -f Dockerfile.build .

docker-build: docker-image
	@echo "Cross-compiling with Docker..."
	@docker run --rm -v $(PWD):/project $(DOCKER_IMAGE) make BUILD_DIR=build-arm64 SDL_BUILD=0

# Add a Dockerfile for cross-compilation
Dockerfile.build:
	@echo "Creating Dockerfile for cross-compilation..."
	@echo "FROM ubuntu:22.04" > Dockerfile.build
	@echo "" >> Dockerfile.build
	@echo "RUN apt-get update && apt-get install -y \\" >> Dockerfile.build
	@echo "    build-essential \\" >> Dockerfile.build
	@echo "    cmake \\" >> Dockerfile.build
	@echo "    git \\" >> Dockerfile.build
	@echo "    crossbuild-essential-arm64 \\" >> Dockerfile.build
	@echo "    pkg-config" >> Dockerfile.build
	@echo "" >> Dockerfile.build
	@echo "WORKDIR /project" >> Dockerfile.build
	@echo "" >> Dockerfile.build
	@echo "# Use ARM64 toolchain" >> Dockerfile.build
	@echo "ENV TOOLCHAIN_FILE=/project/cmake/arm64-toolchain.cmake" >> Dockerfile.build
	@echo "" >> Dockerfile.build
	@echo "CMD ["make", "BUILD_DIR=build-arm64", "SDL_BUILD=0"]" >> Dockerfile.build