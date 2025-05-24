# Dockerfile for building input tests with static SDL
FROM debian:bookworm

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    crossbuild-essential-arm64 \
    pkg-config \
    wget \
    libdrm-dev:arm64 \
    libfreetype6-dev:arm64 \
    zlib1g-dev:arm64

# Create toolchain file for cross-compilation
RUN mkdir -p /build && \
    echo 'set(CMAKE_SYSTEM_NAME Linux)' > /build/arm64-toolchain.cmake && \
    echo 'set(CMAKE_SYSTEM_PROCESSOR aarch64)' >> /build/arm64-toolchain.cmake && \
    echo 'set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)' >> /build/arm64-toolchain.cmake && \
    echo 'set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)' >> /build/arm64-toolchain.cmake && \
    echo 'set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu /usr/local/aarch64)' >> /build/arm64-toolchain.cmake && \
    echo 'set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)' >> /build/arm64-toolchain.cmake && \
    echo 'set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)' >> /build/arm64-toolchain.cmake && \
    echo 'set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)' >> /build/arm64-toolchain.cmake

# Build minimal SDL2 from source (static, with offscreen and dummy drivers only)
WORKDIR /build
RUN git clone --depth 1 --branch release-2.30.x https://github.com/libsdl-org/SDL.git SDL2

WORKDIR /build/SDL2
RUN mkdir build && cd build && \
    cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=/build/arm64-toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local/aarch64 \
    -DSDL_SHARED=OFF \
    -DSDL_STATIC=ON \
    -DSDL_AUDIO=OFF \
    -DSDL_VIDEO=ON \
    -DSDL_RENDER=ON \
    -DSDL_EVENTS=ON \
    -DSDL_JOYSTICK=OFF \
    -DSDL_HAPTIC=OFF \
    -DSDL_SENSOR=OFF \
    -DSDL_X11=OFF \
    -DSDL_WAYLAND=OFF \
    -DSDL_COCOA=OFF \
    -DSDL_DIRECTX=OFF \
    -DSDL_RENDER_D3D=OFF \
    -DSDL_DUMMYVIDEO=ON \
    -DSDL_OFFSCREEN=ON \
    -DSDL_KMSDRM=OFF \
    -DSDL_FBDEV=OFF \
    -DSDL_OPENGL=OFF \
    -DSDL_OPENGLES=OFF \
    -DSDL_VULKAN=OFF \
    -DSDL_LOADSO=OFF \
    -DSDL_DLOPEN=OFF \
    -DSDL_THREADS=ON \
    -DSDL_TIMERS=ON \
    -DSDL_FILE=ON \
    -DSDL_FILESYSTEM=ON \
    -DSDL_POWER=OFF \
    && make -j$(nproc) \
    && make install

WORKDIR /workspace