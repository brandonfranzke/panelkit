# Vendored libyaml

This directory contains the libyaml library (version 0.2.5) vendored directly into the project.

## Source
- Official repository: https://github.com/yaml/libyaml
- Version: 0.2.5
- License: MIT

## Why Vendored?
- Ensures consistent behavior across all platforms
- Eliminates runtime dependencies
- Allows static linking on embedded targets
- Simplifies build process

## Files
- `yaml.h` - Main public API header
- `yaml_private.h` - Internal definitions
- `*.c` - Implementation files

## Usage
Include `yaml.h` and link all `.c` files in this directory.

## Modifications
No modifications have been made to the original source files.