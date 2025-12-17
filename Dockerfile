# Dockerfile for cross-compiling LED Matrix Clock for Raspberry Pi Zero 2 W
# ARM64v8 (aarch64) - ARM Cortex-A53 64-bit CPU

FROM --platform=linux/arm64/v8 debian:bullseye

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    cmake \
    libgraphicsmagick++-dev \
    libwebp-dev \
    && rm -rf /var/lib/apt/lists/*

# Create working directory
WORKDIR /workspace

# Clone and build rpi-rgb-led-matrix library
RUN git clone https://github.com/hzeller/rpi-rgb-led-matrix.git /root/rpi-rgb-led-matrix && \
    cd /root/rpi-rgb-led-matrix && \
    make -C lib

# Display architecture info
RUN echo "=== Build Architecture ===" && \
    uname -m && \
    getconf LONG_BIT && \
    echo "=========================="

# Set default command
CMD ["/bin/bash"]
