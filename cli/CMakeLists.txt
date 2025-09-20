cmake_minimum_required(VERSION 3.16)
project(localpdub_cli VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# Compiler flags
target_compile_options(localpdub PRIVATE
    -Wall -Wextra -O2
    ${ARGON2_CFLAGS_OTHER}
)

# Installation
install(TARGETS localpdub
    RUNTIME DESTINATION bin
)