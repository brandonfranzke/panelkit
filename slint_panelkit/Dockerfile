FROM rust:latest

# Install dependencies for cross-compilation
RUN apt-get update && apt-get install -y \
    pkg-config \
    libssl-dev \
    libfontconfig1-dev \
    libudev-dev \
    libxkbcommon-dev \
    libinput-dev \
    gcc-aarch64-linux-gnu \
    libc6-dev-arm64-cross

# Install cross-architecture libraries
RUN dpkg --add-architecture arm64 && \
    apt-get update && \
    apt-get install -y \
    libxkbcommon-dev:arm64 \
    libinput-dev:arm64 \
    libudev-dev:arm64

# Install Rust for aarch64
RUN rustup target add aarch64-unknown-linux-gnu

# Set cross-compilation environment variables
ENV CARGO_TARGET_AARCH64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/aarch64-linux-gnu-gcc \
    PKG_CONFIG_ALLOW_CROSS=1

# Create working directory
WORKDIR /project

# The actual build command will be run when the container starts
COPY scripts/fix_docker_build.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/fix_docker_build.sh

CMD ["/usr/local/bin/fix_docker_build.sh"]