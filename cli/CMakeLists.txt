cmake_minimum_required(VERSION 3.16)
project(localpdub_cli VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Detect architecture
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|ARM|aarch64|AARCH64")
    set(ARCH "arm")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCH_FULL "arm64")
        message(STATUS "Building for ARM64 architecture")
    else()
        set(ARCH_FULL "arm32")
        message(STATUS "Building for ARM32 architecture")
    endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
    set(ARCH "x86")
    set(ARCH_FULL "x86_64")
    message(STATUS "Building for x86_64 architecture")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686|x86")
    set(ARCH "x86")
    set(ARCH_FULL "x86")
    message(STATUS "Building for x86 architecture")
else()
    set(ARCH "unknown")
    set(ARCH_FULL "unknown")
    message(WARNING "Unknown architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Platform detection
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(PLATFORM "linux")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(PLATFORM "macos")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(PLATFORM "windows")
else()
    set(PLATFORM "unknown")
endif()

# Set output name with architecture
set(OUTPUT_NAME "localpdub-${PLATFORM}-${ARCH_FULL}")

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(PkgConfig REQUIRED)

# Find Argon2
pkg_check_modules(ARGON2 REQUIRED libargon2)

# Source files
set(SOURCES
    src/main.cpp
)

# Create executable
add_executable(localpdub ${SOURCES})

# Set output name
set_target_properties(localpdub PROPERTIES OUTPUT_NAME ${OUTPUT_NAME})

# Include directories
target_include_directories(localpdub PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
    ${ARGON2_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(localpdub
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    ${ARGON2_LIBRARIES}
    pthread
    stdc++fs
)

# Architecture-specific optimizations
if(ARCH STREQUAL "x86")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # x86/x86_64 optimizations
        target_compile_options(localpdub PRIVATE
            -march=native
            -mtune=native
        )

        # Enable AES-NI if available (for AES encryption acceleration)
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag("-maes" COMPILER_SUPPORTS_AES)
        if(COMPILER_SUPPORTS_AES)
            target_compile_options(localpdub PRIVATE -maes)
            message(STATUS "AES-NI hardware acceleration enabled")
        endif()
    endif()
elseif(ARCH STREQUAL "arm")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # ARM optimizations
        if(ARCH_FULL STREQUAL "arm64")
            # ARM64/AArch64 specific
            target_compile_options(localpdub PRIVATE
                -march=armv8-a+crypto
            )
            message(STATUS "ARM crypto extensions enabled")
        else()
            # ARM32 specific
            target_compile_options(localpdub PRIVATE
                -march=armv7-a
                -mfpu=neon
            )
        endif()
    endif()
endif()

# Get build timestamp
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S" UTC)

# Pass build timestamp as compile definition
target_compile_definitions(localpdub PRIVATE
    BUILD_TIMESTAMP="${BUILD_TIMESTAMP}"
    VERSION_STRING="${PROJECT_VERSION}"
)

# General compiler flags
target_compile_options(localpdub PRIVATE
    -Wall -Wextra -O2
    ${ARGON2_CFLAGS_OTHER}
)

# Installation
install(TARGETS localpdub
    RUNTIME DESTINATION bin
)

# Create symbolic link for convenience
install(CODE "
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${OUTPUT_NAME}
        localpdub
        WORKING_DIRECTORY \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin
    )
")

# Display build configuration
message(STATUS "=================================")
message(STATUS "LocalPDub Build Configuration:")
message(STATUS "  Platform: ${PLATFORM}")
message(STATUS "  Architecture: ${ARCH_FULL}")
message(STATUS "  Output binary: ${OUTPUT_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "=================================")