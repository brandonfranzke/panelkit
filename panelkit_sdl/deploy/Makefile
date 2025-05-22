# Target device Makefile for PanelKit

# Configuration
APP         = panelkit
TARGET_DIR  = /tmp/panelkit
LOG_DIR     = /var/log/panelkit
SERVICE_DIR = /etc/systemd/system

.PHONY: help setup install start stop restart status logs clean

# Default target shows help
help:
	@echo "PanelKit Target Device Setup"
	@echo ""
	@echo "Configuration:"
	@echo "  APP        = $(APP)"
	@echo "  TARGET_DIR = $(TARGET_DIR)"
	@echo "  LOG_DIR    = $(LOG_DIR)"
	@echo ""
	@echo "Available targets:"
	@echo "  help    - Show this help message (default)"
	@echo "  setup   - Setup permissions and user service"
	@echo "  install - Install binary and service files"
	@echo "  start   - Start the service"
	@echo "  stop    - Stop the service"
	@echo "  restart - Restart the service"
	@echo "  status  - Show service status"
	@echo "  logs    - Follow service logs"
	@echo "  clean   - Remove installed files and service"

setup:
	@echo "Setting up system permissions and directories..."
	sudo usermod -a -G input,video $(USER)
	sudo chmod a+rw /dev/dri/card0 /dev/fb0 || true
	sudo mkdir -p $(LOG_DIR)
	mkdir -p $(TARGET_DIR)
	@echo "Setup completed"

install: setup
	@echo "Installing $(APP) service..."
	@if [ ! -f "$(TARGET_DIR)/$(APP)" ]; then \
		echo "Error: Binary not found at $(TARGET_DIR)/$(APP)"; \
		echo "Make sure files are deployed first"; \
		exit 1; \
	fi
	@if [ ! -f "$(TARGET_DIR)/$(APP).service" ]; then \
		echo "Error: Service file not found at $(TARGET_DIR)/$(APP).service"; \
		exit 1; \
	fi
	sudo cp $(TARGET_DIR)/$(APP).service $(SERVICE_DIR)/
	chmod +x $(TARGET_DIR)/$(APP)
	sudo systemctl daemon-reload
	sudo systemctl enable $(APP)
	@echo "Service installed and enabled"

start:
	sudo systemctl start $(APP)

stop:
	sudo systemctl stop $(APP)

restart:
	sudo systemctl restart $(APP)

status:
	sudo systemctl status $(APP) --no-pager

logs:
	tail -f $(LOG_DIR)/$(APP).log

clean:
	@echo "Cleaning $(APP) service..."
	sudo systemctl stop $(APP) || true
	sudo systemctl disable $(APP) || true
	sudo rm -f $(SERVICE_DIR)/$(APP).service
	sudo systemctl daemon-reload
	sudo rm -rf $(LOG_DIR)
	@echo "Service cleanup completed"