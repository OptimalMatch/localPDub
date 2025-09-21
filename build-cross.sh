#!/bin/bash
# Cross-compilation build script for LocalPDub
# Builds for multiple architectures

set -e

echo "==================================="
echo "LocalPDub Cross-Compilation Builder"
echo "==================================="

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to build for a specific architecture
build_arch() {
    local arch=$1
    local compiler_prefix=$2
    local cmake_processor=$3
    local output_name=$4

    echo -e "\n${YELLOW}Building for ${arch}...${NC}"

    # Create build directory
    rm -rf build-${arch}
    mkdir -p build-${arch}
    cd build-${arch}

    # Configure with CMake
    cmake ../cli \
        -DCMAKE_SYSTEM_NAME=Linux \
        -DCMAKE_SYSTEM_PROCESSOR=${cmake_processor} \
        -DCMAKE_C_COMPILER=${compiler_prefix}-gcc \
        -DCMAKE_CXX_COMPILER=${compiler_prefix}-g++ \
        -DCMAKE_FIND_ROOT_PATH=/usr/${compiler_prefix} \
        -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
        -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY

    # Build
    make -j$(nproc)

    # Strip binary
    ${compiler_prefix}-strip ${output_name}

    # Move to release directory
    mkdir -p ../release
    cp ${output_name} ../release/

    # Create tarball
    cd ../release
    tar -czf ${output_name}.tar.gz ${output_name}
    sha256sum ${output_name}.tar.gz > ${output_name}.tar.gz.sha256

    echo -e "${GREEN}✓ Built ${output_name}${NC}"
    cd ..
}

# Check for required tools
check_tool() {
    if ! command -v $1 &> /dev/null; then
        echo "Error: $1 is not installed"
        echo "Install with: sudo apt-get install $2"
        exit 1
    fi
}

# Main build process
main() {
    echo "Checking dependencies..."

    # Check native build tools
    check_tool cmake cmake
    check_tool make build-essential

    # Build native x86_64 first (if on x86_64)
    if [[ $(uname -m) == "x86_64" ]]; then
        echo -e "\n${YELLOW}Building native x86_64...${NC}"
        rm -rf build
        mkdir -p build
        cd build
        cmake ../cli
        make -j$(nproc)
        strip localpdub-linux-x86_64
        mkdir -p ../release
        cp localpdub-linux-x86_64 ../release/
        cd ../release
        tar -czf localpdub-linux-x86_64.tar.gz localpdub-linux-x86_64
        sha256sum localpdub-linux-x86_64.tar.gz > localpdub-linux-x86_64.tar.gz.sha256
        cd ..
        echo -e "${GREEN}✓ Built localpdub-linux-x86_64${NC}"
    fi

    # Check for ARM64 cross-compiler
    if command -v aarch64-linux-gnu-gcc &> /dev/null; then
        # Build dependencies if not already built
        if [ ! -d "deps/arm64/lib" ]; then
            echo -e "${YELLOW}Building ARM64 dependencies first...${NC}"
            ./scripts/build-arm64-deps.sh
        fi

        # Build ARM64 using our custom toolchain
        echo -e "\n${YELLOW}Building for ARM64...${NC}"
        rm -rf build-arm64
        mkdir -p build-arm64
        cd build-arm64

        cmake ../cli -DCMAKE_TOOLCHAIN_FILE=../deps/arm64/toolchain-arm64-with-deps.cmake
        make -j$(nproc)

        aarch64-linux-gnu-strip localpdub-linux-arm64
        mkdir -p ../release
        cp localpdub-linux-arm64 ../release/
        cd ../release
        tar -czf localpdub-linux-arm64.tar.gz localpdub-linux-arm64
        sha256sum localpdub-linux-arm64.tar.gz > localpdub-linux-arm64.tar.gz.sha256
        cd ..
        echo -e "${GREEN}✓ Built localpdub-linux-arm64${NC}"
    else
        echo -e "${YELLOW}Skipping ARM64 build (install with: sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu)${NC}"
    fi

    # Check for ARMv7 cross-compiler (32-bit ARM)
    if command -v arm-linux-gnueabihf-gcc &> /dev/null; then
        build_arch "armhf" "arm-linux-gnueabihf" "arm" "localpdub-linux-armhf"
    else
        echo -e "${YELLOW}Skipping ARMv7 build (install with: sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf)${NC}"
    fi

    echo -e "\n${GREEN}==================================="
    echo "Build complete! Binaries in release/"
    echo "===================================${NC}"
    ls -la release/
}

# Run main function
main "$@"