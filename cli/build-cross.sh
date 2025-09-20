#!/bin/bash

# Cross-compilation build script for LocalPDub
# Builds for multiple architectures

set -e

echo "═══════════════════════════════════════════════"
echo "  LocalPDub Cross-Compilation Build Script"
echo "═══════════════════════════════════════════════"
echo ""

# Function to build for specific architecture
build_arch() {
    local ARCH=$1
    local TOOLCHAIN=$2
    local BUILD_DIR="build-${ARCH}"

    echo "Building for ${ARCH}..."

    mkdir -p ${BUILD_DIR}
    cd ${BUILD_DIR}

    if [ -n "${TOOLCHAIN}" ]; then
        cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-${ARCH}.cmake
    else
        cmake ..
    fi

    make -j$(nproc)
    cd ..

    echo "✓ Build complete for ${ARCH}"
    echo ""
}

# Native build (auto-detect architecture)
echo "══════════════════════════════════════"
echo "Building native binary..."
echo "══════════════════════════════════════"
build_arch "native" ""

# Cross-compile for ARM64 (if cross-compiler available)
if command -v aarch64-linux-gnu-g++ &> /dev/null; then
    echo "══════════════════════════════════════"
    echo "Building for ARM64..."
    echo "══════════════════════════════════════"
    build_arch "arm64" "arm64"
else
    echo "⚠ ARM64 cross-compiler not found, skipping ARM64 build"
fi

# Cross-compile for ARM32 (if cross-compiler available)
if command -v arm-linux-gnueabihf-g++ &> /dev/null; then
    echo "══════════════════════════════════════"
    echo "Building for ARM32..."
    echo "══════════════════════════════════════"
    build_arch "arm32" "arm32"
else
    echo "⚠ ARM32 cross-compiler not found, skipping ARM32 build"
fi

# Summary
echo "═══════════════════════════════════════════════"
echo "Build Summary:"
echo "═══════════════════════════════════════════════"
echo ""
echo "Binaries created:"

if [ -f build-native/localpdub-* ]; then
    ls -lh build-native/localpdub-* | awk '{print "  Native:  " $9 " (" $5 ")"}'
fi

if [ -f build-arm64/localpdub-* ]; then
    ls -lh build-arm64/localpdub-* | awk '{print "  ARM64:   " $9 " (" $5 ")"}'
fi

if [ -f build-arm32/localpdub-* ]; then
    ls -lh build-arm32/localpdub-* | awk '{print "  ARM32:   " $9 " (" $5 ")"}'
fi

echo ""
echo "To install cross-compilation tools on Debian/Ubuntu:"
echo "  sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu  # For ARM64"
echo "  sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf  # For ARM32"
echo ""