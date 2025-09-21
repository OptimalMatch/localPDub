#!/bin/bash
# Script to build ARM64 dependencies for cross-compilation

set -e

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Setup directories
DEPS_DIR="$(pwd)/deps/arm64"
BUILD_DIR="$(pwd)/deps/build"
PREFIX="$DEPS_DIR"

# Cross-compiler settings
export CC=aarch64-linux-gnu-gcc
export CXX=aarch64-linux-gnu-g++
export AR=aarch64-linux-gnu-ar
export RANLIB=aarch64-linux-gnu-ranlib
export STRIP=aarch64-linux-gnu-strip
export HOST=aarch64-linux-gnu

echo -e "${YELLOW}Building ARM64 dependencies in $DEPS_DIR${NC}"

# Create directories
mkdir -p "$DEPS_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Build OpenSSL for ARM64
build_openssl() {
    echo -e "${YELLOW}Building OpenSSL for ARM64...${NC}"

    if [ ! -f openssl-1.1.1w.tar.gz ]; then
        wget https://www.openssl.org/source/openssl-1.1.1w.tar.gz
    fi

    rm -rf openssl-1.1.1w
    tar -xzf openssl-1.1.1w.tar.gz
    cd openssl-1.1.1w

    # Save current environment and unset for OpenSSL
    SAVED_CC=$CC
    SAVED_CXX=$CXX
    SAVED_AR=$AR
    SAVED_RANLIB=$RANLIB
    unset CC CXX AR RANLIB

    ./Configure linux-aarch64 \
        --prefix="$PREFIX" \
        --cross-compile-prefix=aarch64-linux-gnu- \
        no-shared \
        no-tests

    make -j$(nproc)
    make install_sw

    # Restore environment for other builds
    export CC=$SAVED_CC
    export CXX=$SAVED_CXX
    export AR=$SAVED_AR
    export RANLIB=$SAVED_RANLIB

    cd ..
    echo -e "${GREEN}✓ OpenSSL built successfully${NC}"
}

# Build Argon2 for ARM64
build_argon2() {
    echo -e "${YELLOW}Building Argon2 for ARM64...${NC}"

    if [ ! -d argon2 ]; then
        git clone https://github.com/P-H-C/phc-winner-argon2.git argon2
    fi

    cd argon2
    git checkout 20190702  # Use stable version

    # Build with cross-compiler
    make clean
    make CC=$CC AR=$AR RANLIB=$RANLIB \
        OPTTARGET=generic \
        LIBRARY_REL="$PREFIX/lib" \
        INCLUDE_REL="$PREFIX/include" \
        DESTDIR="" \
        PREFIX="$PREFIX" \
        -j$(nproc)

    # Manual install since make install might not work correctly
    mkdir -p "$PREFIX/lib" "$PREFIX/include"
    cp libargon2.a "$PREFIX/lib/"
    cp include/argon2.h "$PREFIX/include/"

    # Create pkg-config file
    mkdir -p "$PREFIX/lib/pkgconfig"
    cat > "$PREFIX/lib/pkgconfig/libargon2.pc" <<EOF
prefix=$PREFIX
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include

Name: libargon2
Description: Argon2 password hashing library
Version: 20190702
Libs: -L\${libdir} -largon2
Cflags: -I\${includedir}
EOF

    cd ..
    echo -e "${GREEN}✓ Argon2 built successfully${NC}"
}

# Build nlohmann-json (header-only, just copy)
install_nlohmann_json() {
    echo -e "${YELLOW}Installing nlohmann-json...${NC}"

    if [ ! -d json ]; then
        git clone https://github.com/nlohmann/json.git
    fi

    cd json
    git checkout v3.11.2  # Use stable version

    # Just copy headers since it's header-only
    mkdir -p "$PREFIX/include/nlohmann"
    cp single_include/nlohmann/json.hpp "$PREFIX/include/nlohmann/"

    # Create cmake config
    mkdir -p "$PREFIX/lib/cmake/nlohmann_json"
    cat > "$PREFIX/lib/cmake/nlohmann_json/nlohmann_jsonConfig.cmake" <<EOF
set(nlohmann_json_INCLUDE_DIRS "$PREFIX/include")
set(nlohmann_json_FOUND TRUE)
EOF

    cd ..
    echo -e "${GREEN}✓ nlohmann-json installed successfully${NC}"
}

# Main build process
main() {
    # Check for cross-compiler
    if ! command -v aarch64-linux-gnu-gcc &> /dev/null; then
        echo -e "${RED}Error: ARM64 cross-compiler not found${NC}"
        echo "Install with: sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu"
        exit 1
    fi

    # Build dependencies
    build_openssl
    build_argon2
    install_nlohmann_json

    echo -e "${GREEN}==================================="
    echo "ARM64 dependencies built successfully!"
    echo "Location: $DEPS_DIR"
    echo "===================================${NC}"

    # Show what was built
    echo -e "\n${YELLOW}Installed files:${NC}"
    ls -la "$PREFIX/lib/"

    # Create a CMake toolchain file that uses these dependencies
    cat > "$DEPS_DIR/toolchain-arm64-with-deps.cmake" <<EOF
# Toolchain file for ARM64 with custom dependencies

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Cross compiler
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Where our custom dependencies are
set(ARM64_DEPS_PREFIX "$PREFIX")

# Tell CMake where to find our libraries
set(CMAKE_PREFIX_PATH \${ARM64_DEPS_PREFIX})
set(CMAKE_FIND_ROOT_PATH \${ARM64_DEPS_PREFIX} /usr/aarch64-linux-gnu)

# Pkg-config settings
set(PKG_CONFIG_EXECUTABLE /usr/bin/pkg-config)
set(ENV{PKG_CONFIG_PATH} "\${ARM64_DEPS_PREFIX}/lib/pkgconfig")
set(ENV{PKG_CONFIG_LIBDIR} "\${ARM64_DEPS_PREFIX}/lib/pkgconfig")
set(ENV{PKG_CONFIG_SYSROOT_DIR} \${ARM64_DEPS_PREFIX})

# Search settings
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Static linking
set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++" CACHE STRING "" FORCE)

# Include and library directories
include_directories(SYSTEM \${ARM64_DEPS_PREFIX}/include)
link_directories(\${ARM64_DEPS_PREFIX}/lib)
EOF

    echo -e "\n${GREEN}Toolchain file created: $DEPS_DIR/toolchain-arm64-with-deps.cmake${NC}"
}

# Run main
main "$@"