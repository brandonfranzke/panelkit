# PanelKit Makefile Configuration
PROJECT := slint_panelkit
TARGET := aarch64-unknown-linux-gnu

# Configuration Variables - Edit these by hand
WIDTH ?= 480
HEIGHT ?= 640
DEBUG ?= --debug

# Host commands
run:
	cargo run -- --width $(WIDTH) --height $(HEIGHT) $(DEBUG)

build:
	cargo build --release

# Target commands
target:
	docker volume inspect cargo-registry >/dev/null 2>&1 || docker volume create cargo-registry
	docker build -t slint-cross .
	docker run --rm \
		-v $(shell pwd):/project \
		-v cargo-registry:/usr/local/cargo/registry \
		slint-cross
	sudo chown -R $(shell whoami):$(shell id -gn) target/ || true

deploy: target
	ssh panelkit "mkdir -p panelkit/logs"
	scp target/$(TARGET)/release/$(PROJECT) panelkit:panelkit/
	scp deploy/Makefile.target panelkit:panelkit/Makefile
	scp deploy/slint_panelkit.service panelkit:panelkit/$(PROJECT).service
	@echo "Deployed. SSH to target and run 'make setup'"

clean:
	cargo clean

.PHONY: run build target deploy clean