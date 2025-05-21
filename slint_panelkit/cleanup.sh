#!/bin/bash
# Cleanup script for removing old Rust implementation

# Define the base path
BASE_PATH="/Users/brandon/ProjectRoot/Code/panelkit"

# Items to preserve (relative to the base path)
PRESERVE=(
  "panelkit/panelkit_sdl"
)

# Function to check if path should be preserved
should_preserve() {
  local path=$1
  for item in "${PRESERVE[@]}"; do
    if [[ "$path" == *"$item"* ]]; then
      return 0 # True, should preserve
    fi
  done
  return 1 # False, should not preserve
}

# Function to safely delete a file or directory
safe_delete() {
  local path=$1
  if should_preserve "$path"; then
    echo "Preserving: $path"
  else
    echo "Deleting: $path"
    rm -rf "$path"
  fi
}

# List of items to delete
DELETE_ITEMS=(
  "$BASE_PATH/Cargo.lock"
  "$BASE_PATH/Cargo.toml"
  "$BASE_PATH/DESIGN_PLAN_SLINT.md"
  "$BASE_PATH/Makefile"
  "$BASE_PATH/README.md"
  "$BASE_PATH/assets"
  "$BASE_PATH/build"
  "$BASE_PATH/config"
  "$BASE_PATH/containers"
  "$BASE_PATH/docs"
  "$BASE_PATH/scripts"
  "$BASE_PATH/src"
  "$BASE_PATH/target"
  "$BASE_PATH/slint_panelkit/panelkit"
)

# Delete specific items
for item in "${DELETE_ITEMS[@]}"; do
  safe_delete "$item"
done

echo "Cleanup complete!"