#!/bin/bash
# Utility script to downgrade Cargo.lock version for compatibility with older Docker images
# Only needed if you encounter lock file version errors during cross-compilation

sed -i '' 's/^version = 4/version = 3/' ../Cargo.lock
echo "Cargo.lock downgraded to version 3 for Docker compatibility"