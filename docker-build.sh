#!/bin/bash
# Docker-based build script for LED Matrix Clock
# ARM64v8 only (Raspberry Pi Zero 2 W with ARM Cortex-A53 64-bit)

set -e

LOCALE="${1:-it_IT}"
IMAGE_NAME="rpi-matrix-clock-builder"
PLATFORM="linux/arm64/v8"

echo "════════════════════════════════════════════════════════"
echo "  LED Matrix Clock - Docker Build (ARM64)"
echo "════════════════════════════════════════════════════════"
echo "  Architecture: ARM64v8 (aarch64)"
echo "  Locale: $LOCALE"
echo "════════════════════════════════════════════════════════"
echo ""

# Check if Docker is running
if ! docker info > /dev/null 2>&1; then
    echo "❌ Docker is not running. Please start Docker and try again."
    exit 1
fi

# Step 1: Build Docker image (only if it doesn't exist)
if ! docker image inspect $IMAGE_NAME > /dev/null 2>&1; then
    echo "🐳 Building Docker image (this may take a few minutes)..."
    docker build --platform $PLATFORM -t $IMAGE_NAME .
    echo "✓ Docker image built"
    echo ""
else
    echo "✓ Docker image already exists"
    echo ""
fi

# Step 2: Build the project inside Docker
echo "🔨 Building project in Docker container..."
docker run --rm \
    --platform $PLATFORM \
    -v "$(pwd):/workspace" \
    -w /workspace \
    $IMAGE_NAME \
    bash -c "make clean && make LOCALE=$LOCALE"

if [ $? -ne 0 ]; then
    echo "❌ Build failed"
    exit 1
fi

echo ""
echo "✓ Build successful!"
echo ""
echo "Binary location: build/led-clock"
echo "Architecture:"
file build/led-clock 2>/dev/null || echo "ARM64 (aarch64) ELF binary"
echo ""
echo "════════════════════════════════════════════════════════"
echo "  Next steps:"
echo "════════════════════════════════════════════════════════"
echo ""
echo "To deploy to Raspberry Pi, run:"
echo "  ./deploy-binary.sh"
echo ""
