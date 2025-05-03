# PanelKit Architecture

This document describes the overall architecture of the PanelKit embedded UI system.

## Overview

PanelKit is designed as a self-contained UI application for embedded Linux environments with touch input. It follows a modular, event-driven architecture with clean separation of concerns.

## Core Components

### 1. UI System

The UI system is responsible for rendering components to the display and handling user interactions:

- **UI Manager**: Controls page navigation and widget hierarchy
- **Page**: Represents a full-screen interface with multiple widgets
- **Widget**: Individual UI elements (buttons, labels, etc.)

### 2. Event System

The event system handles communication between components using a pub/sub pattern:

- **Event Broker**: Central message dispatcher
- **Event Types**: Structured event data (Touch, State changes, etc.)
- **Event Subscribers**: Components that listen for specific events

### 3. State Management

The state management system maintains application state and configuration:

- **State Manager**: Handles state persistence and retrieval
- **State Types**: Different categories of state (UI, settings, etc.)
- **Persistence**: Optional persistence of state to storage

### 4. Platform Abstraction

The platform layer abstracts hardware-specific details:

- **Display Driver**: Handles rendering to the screen
- **Input Driver**: Processes touch and other input events
- **Platform Factory**: Creates appropriate driver implementations

## Data Flow

1. Input events are captured by the platform layer
2. Events are published to the event system
3. UI components subscribe to relevant events
4. State changes are persisted as needed
5. UI is re-rendered based on state changes

## Build System

The build system supports both local development and cross-compilation:

- **Containerized Builds**: All builds occur in Docker containers
- **Makefile**: Central orchestration for all build workflows
- **Feature Flags**: Conditional compilation for simulator vs. target

## Deployment

The application is designed to run automatically on boot:

- **Systemd Service**: Auto-starts the application
- **Fullscreen Mode**: Runs without a window manager
- **Error Handling**: Graceful recovery and logging