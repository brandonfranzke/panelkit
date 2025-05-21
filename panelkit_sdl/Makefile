# Makefile for PanelKit SDL

.PHONY: all clean build run cross deploy

all: build

build:
	@echo "Building PanelKit SDL..."
	@./build.sh build

run: build
	@echo "Running PanelKit SDL..."
	@./build.sh

clean:
	@echo "Cleaning build directory..."
	@./build.sh clean

cross:
	@echo "Cross-compiling for ARM64 target..."
	@./build_cross.sh

deploy: cross
	@echo "Creating deployment package..."
	@echo "Deployment files are in the ./deploy directory"