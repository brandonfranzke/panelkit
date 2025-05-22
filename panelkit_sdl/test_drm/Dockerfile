FROM debian:bookworm

# Add arm64 architecture and install build tools + libraries
RUN dpkg --add-architecture arm64 && \
    apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    crossbuild-essential-arm64 \
    libdrm-dev:arm64 \
    pkg-config

# Set up cross-compilation
ENV CC=aarch64-linux-gnu-gcc
ENV CXX=aarch64-linux-gnu-g++
ENV AR=aarch64-linux-gnu-ar
ENV PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig

# Create CMake toolchain file
RUN mkdir -p /build && \
    echo 'set(CMAKE_SYSTEM_NAME Linux)' > /build/arm64-toolchain.cmake && \
    echo 'set(CMAKE_SYSTEM_PROCESSOR aarch64)' >> /build/arm64-toolchain.cmake && \
    echo 'set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)' >> /build/arm64-toolchain.cmake && \
    echo 'set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)' >> /build/arm64-toolchain.cmake && \
    echo 'set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu /usr/local/aarch64)' >> /build/arm64-toolchain.cmake && \
    echo 'set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)' >> /build/arm64-toolchain.cmake && \
    echo 'set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)' >> /build/arm64-toolchain.cmake && \
    echo 'set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)' >> /build/arm64-toolchain.cmake

# Download and build minimal SDL2
WORKDIR /build
RUN git clone --depth 1 --branch release-2.30.x https://github.com/libsdl-org/SDL.git SDL2

# Build minimal SDL2 - only offscreen driver, no dynamic loading
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
    -DSDL_KMSDRM=OFF \
    -DSDL_FBDEV=OFF \
    -DSDL_DUMMYVIDEO=ON \
    -DSDL_OFFSCREEN=ON \
    -DSDL_LOADSO=OFF \
    -DSDL_OPENGL=OFF \
    -DSDL_OPENGLES=OFF \
    -DSDL_VULKAN=OFF \
    && make -j$(nproc) \
    && make install

WORKDIR /build