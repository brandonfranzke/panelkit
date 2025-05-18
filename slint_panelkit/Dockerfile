FROM rust:slim

ENV RUST_TARGET=aarch64-unknown-linux-gnu

# Essential cross-compilation dependencies only
RUN apt-get update && apt-get install -y \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    libc6-dev-arm64-cross \
    pkg-config \
    libdrm-dev \
    libudev-dev \
    libxkbcommon-dev \
    libinput-dev \
    libgbm-dev \
    && rm -rf /var/lib/apt/lists/*

# Set environment for cross-compilation
ENV PKG_CONFIG_ALLOW_CROSS=1 \
    CARGO_TARGET_AARCH64_UNKNOWN_LINUX_GNU_LINKER=aarch64-linux-gnu-gcc \
    CC_aarch64_unknown_linux_gnu=aarch64-linux-gnu-gcc \
    CXX_aarch64_unknown_linux_gnu=aarch64-linux-gnu-g++ \
    SLINT_FORCE_NOSEAT=1

# Add target and set working directory
RUN rustup target add ${RUST_TARGET}
WORKDIR /project

# Build command with explicit output path
CMD cargo build --release --target=${RUST_TARGET} --features=embedded && \
    # Ensure the binary exists and has proper permissions
    ls -la /project/target/${RUST_TARGET}/release/slint_panelkit && \
    chmod +x /project/target/${RUST_TARGET}/release/slint_panelkit