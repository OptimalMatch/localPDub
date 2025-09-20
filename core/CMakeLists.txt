cmake_minimum_required(VERSION 3.16)
project(localpdub_core VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_TESTS "Build unit tests" ON)
option(USE_OPENSSL "Use OpenSSL for cryptography" ON)

# Find packages
find_package(Threads REQUIRED)

if(USE_OPENSSL)
    find_package(OpenSSL REQUIRED)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CRYPTOPP REQUIRED libcrypto++)
endif()

# Find nlohmann/json
find_package(nlohmann_json 3.10 REQUIRED)

# Source files
set(SOURCES
    src/crypto/crypto.cpp
    src/crypto/key_derivation.cpp
    src/crypto/aes_gcm.cpp
    src/storage/vault_reader.cpp
    src/storage/vault_writer.cpp
    src/storage/file_lock.cpp
    src/sync/sync_manager.cpp
    src/sync/network_transfer.cpp
    src/models/vault.cpp
    src/models/password_entry.cpp
    src/utils/uuid.cpp
    src/utils/secure_string.cpp
)

# Header files
set(HEADERS
    include/localpdub/localpdub.h
    include/localpdub/crypto.h
    include/localpdub/storage.h
    include/localpdub/sync.h
    include/localpdub/models.h
    include/localpdub/utils.h
)

# Create library
add_library(localpdub_core ${SOURCES})

# Include directories
target_include_directories(localpdub_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(localpdub_core
    PUBLIC
        Threads::Threads
        nlohmann_json::nlohmann_json
)

if(USE_OPENSSL)
    target_link_libraries(localpdub_core PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(localpdub_core PRIVATE USE_OPENSSL)
else()
    target_link_libraries(localpdub_core PUBLIC ${CRYPTOPP_LIBRARIES})
    target_include_directories(localpdub_core PUBLIC ${CRYPTOPP_INCLUDE_DIRS})
endif()

# Platform-specific settings
if(WIN32)
    target_compile_definitions(localpdub_core PRIVATE LOCALPDUB_WINDOWS)
elseif(APPLE)
    target_compile_definitions(localpdub_core PRIVATE LOCALPDUB_MACOS)
elseif(UNIX)
    target_compile_definitions(localpdub_core PRIVATE LOCALPDUB_LINUX)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(localpdub_core PRIVATE /W4 /WX)
else()
    target_compile_options(localpdub_core PRIVATE -Wall -Wextra -Werror)
endif()

# Export targets
export(TARGETS localpdub_core FILE localpdub_coreTargets.cmake)

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(TARGETS localpdub_core
    EXPORT localpdub_coreTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/localpdub
    DESTINATION include
)

install(EXPORT localpdub_coreTargets
    FILE localpdub_coreTargets.cmake
    DESTINATION lib/cmake/localpdub_core
)