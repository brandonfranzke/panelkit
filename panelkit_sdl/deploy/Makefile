# Target device Makefile for PanelKit

# Configuration
APP         = panelkit
TARGET_DIR  = /tmp/panelkit
INSTALL_DIR = /usr/local/bin
LOG_DIR     = /var/log/panelkit
CONFIG_DIR  = /etc/panelkit
SERVICE_DIR = /etc/systemd/system

.PHONY: help setup install uninstall start stop restart status logs clean

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
	@echo "  help      - Show this help message (default)"
	@echo "  setup     - Setup permissions and user service"
	@echo "  install   - Install binary and service files"
	@echo "  uninstall - Remove installed files and service"
	@echo "  start     - Start the service"
	@echo "  stop      - Stop the service"
	@echo "  restart   - Restart the service"
	@echo "  status    - Show service status"
	@echo "  logs      - Follow service logs"
	@echo "  clean     - Remove deployed files from $(TARGET_DIR)"

setup:
	@echo "Setting up system permissions and directories..."
	sudo usermod -a -G input,video $(USER)
	sudo chmod a+rw /dev/dri/card0 /dev/fb0 || true
	sudo mkdir -p $(LOG_DIR)
	sudo mkdir -p $(CONFIG_DIR)
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
	sudo systemctl stop $(APP) || true
	sudo cp $(TARGET_DIR)/$(APP) $(INSTALL_DIR)/
	sudo chmod +x $(INSTALL_DIR)/$(APP)
	sudo cp $(TARGET_DIR)/$(APP).service $(SERVICE_DIR)/
	@if [ -d "$(TARGET_DIR)/config" ]; then \
		sudo cp -r $(TARGET_DIR)/config/* $(CONFIG_DIR)/; \
		echo "Configuration files installed to $(CONFIG_DIR)"; \
	fi
	sudo systemctl daemon-reload
	sudo systemctl enable $(APP)
	@echo "Service installed and enabled"

start: install
	sudo systemctl start $(APP)

stop:
	sudo systemctl stop $(APP)

restart: install
	sudo systemctl restart $(APP)

status:
	sudo systemctl status $(APP) --no-pager

logs:
	tail -f $(LOG_DIR)/$(APP).log

uninstall:
	@echo "Uninstalling $(APP) service..."
	sudo systemctl stop $(APP) || true
	sudo systemctl disable $(APP) || true
	sudo rm -f $(SERVICE_DIR)/$(APP).service
	sudo rm -f $(INSTALL_DIR)/$(APP)
	sudo rm -rf $(CONFIG_DIR)
	sudo rm -rf $(LOG_DIR)
	sudo systemctl daemon-reload
	@echo "Service uninstalled"

clean:
	@echo "Cleaning deployed files from $(TARGET_DIR)..."
	rm -rf $(TARGET_DIR)
	@echo "Deployed files cleaned"