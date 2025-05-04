FROM debian:bullseye-slim

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies 
# - Only include what's actually needed for cross-compilation
RUN apt-get update && apt-get install -y \
    curl \
    build-essential \
    pkg-config \
    gcc-arm-linux-gnueabihf \
    g++-arm-linux-gnueabihf \
    libc6-dev-armhf-cross \
    git \
    ca-certificates \
    --no-install-recommends \
    && rm -rf /var/lib/apt/lists/*

# Install Rust with a specific version for reproducibility
ARG RUST_VERSION=1.73.0
RUN curl -sSf https://sh.rustup.rs -o rustup.sh \
    && chmod +x rustup.sh \
    && ./rustup.sh -y --default-toolchain ${RUST_VERSION} --profile minimal \
    && rm rustup.sh

# Add cargo bin to PATH permanently
ENV PATH="/root/.cargo/bin:${PATH}"

# Add ARM target
RUN rustup target add armv7-unknown-linux-gnueabihf

# Configure Cargo for cross compilation
RUN mkdir -p ~/.cargo && \
    echo '[target.armv7-unknown-linux-gnueabihf]' > ~/.cargo/config && \
    echo 'linker = "arm-linux-gnueabihf-gcc"' >> ~/.cargo/config && \
    echo 'rustflags = ["-C", "link-arg=-march=armv7-a", "-C", "link-arg=-mfpu=neon-vfpv4"]' >> ~/.cargo/config

# Set up working directory
WORKDIR /src

# Default command
CMD ["bash"]