# Simple Makefile for PanelKit LVGL

.PHONY: all clean build run

all: build

build:
	@echo "Building PanelKit with LVGL..."
	@mkdir -p build
	@cd build && cmake .. && make

run: build
	@echo "Running PanelKit..."
	@cd build && ./panelkit

clean:
	@echo "Cleaning build directory..."
	@rm -rf build