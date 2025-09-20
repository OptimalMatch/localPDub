#!/bin/bash

# LocalPDub Build Script for Debian/Ubuntu Linux

set -e

echo "═══════════════════════════════════════════"
echo "  LocalPDub Password Manager Build Script"
echo "═══════════════════════════════════════════"
echo ""

# Check for required dependencies
echo "Checking dependencies..."

check_dependency() {
    if ! command -v $1 &> /dev/null; then
        echo "❌ $1 is not installed"
        return 1
    else
        echo "✓ $1 found"
        return 0
    fi
}

missing_deps=false

check_dependency cmake || missing_deps=true
check_dependency g++ || missing_deps=true
check_dependency pkg-config || missing_deps=true

# Check for libraries
echo ""
echo "Checking libraries..."

if ! pkg-config --exists libssl; then
    echo "❌ OpenSSL development libraries not found"
    missing_deps=true
else
    echo "✓ OpenSSL found"
fi

if ! pkg-config --exists libargon2; then
    echo "❌ Argon2 library not found"
    missing_deps=true
else
    echo "✓ Argon2 found"
fi

# Check for nlohmann-json
if ! pkg-config --exists nlohmann_json 2>/dev/null && \
   ! [ -f /usr/include/nlohmann/json.hpp ] && \
   ! [ -f /usr/local/include/nlohmann/json.hpp ]; then
    echo "❌ nlohmann-json not found"
    missing_deps=true
else
    echo "✓ nlohmann-json found"
fi

if $missing_deps; then
    echo ""
    echo "═══════════════════════════════════════════"
    echo "Please install missing dependencies:"
    echo ""
    echo "sudo apt update"
    echo "sudo apt install -y \\"
    echo "    build-essential \\"
    echo "    cmake \\"
    echo "    libssl-dev \\"
    echo "    libargon2-dev \\"
    echo "    nlohmann-json3-dev \\"
    echo "    pkg-config"
    echo ""
    echo "═══════════════════════════════════════════"
    exit 1
fi

echo ""
echo "All dependencies satisfied!"
echo ""

# Create build directory
echo "Setting up build directory..."
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake ../cli

# Build
echo ""
echo "Building LocalPDub..."
make -j$(nproc)

echo ""
echo "═══════════════════════════════════════════"
echo "✓ Build completed successfully!"
echo ""
echo "Binary location: ./build/localpdub"
echo ""
echo "To install system-wide:"
echo "  sudo make install"
echo ""
echo "To run:"
echo "  ./build/localpdub"
echo "═══════════════════════════════════════════"