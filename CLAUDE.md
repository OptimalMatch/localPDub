# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Git Commit Conventions

- Do not include any credits, co-authorship, or attribution in commit messages
- Do not add "Generated with Claude Code" or similar attribution messages
- Do not include "Co-Authored-By" lines
- Keep commit messages focused solely on the changes made

## Project Overview

LocalPDub is a simple password manager with local file storage and straightforward synchronization options. It uses a C++ core library for performance and cross-platform support.

## Build Commands

### Core Library (C++)
```bash
cd core
mkdir build && cd build
cmake ..
make -j$(nproc)       # Build library
make test             # Run tests
```

### Desktop Application
```bash
cd desktop
mkdir build && cd build
cmake ..
make                  # Build desktop app
```

### Mobile Applications
```bash
# iOS
cd mobile/ios
xcodebuild            # Build iOS app

# Android
cd mobile/android
./gradlew build       # Build Android app
```

## Architecture

- **Core**: C++ library providing crypto, file storage, and sync logic
- **Desktop**: Qt or wxWidgets native application
- **Mobile**: Native iOS (Swift) and Android (Kotlin) with C++ bindings
- **Encryption**: AES-256-GCM with Argon2 key derivation
- **Storage**: Encrypted JSON files (.lpd format)
- **Sync**: User-initiated network sync on ports 51820-51829 (no persistent listeners)

## Key Files

- `ARCHITECTURE.md` - System design and component overview
- `SYNC_PROTOCOL.md` - P2P synchronization protocol specification
- `SECURITY.md` - Cryptographic implementation details
- `DATA_MODEL.md` - Database schema and storage format

## Repository Structure

```
localPDub/
├── core/               # C++ core library
│   ├── include/       # Public headers
│   ├── src/          # Implementation files
│   │   ├── crypto/   # Encryption (AES-GCM, Argon2)
│   │   ├── storage/  # File I/O and vault management
│   │   ├── sync/     # Sync protocols
│   │   └── models/   # Data structures
│   └── CMakeLists.txt
├── desktop/          # Desktop application
├── mobile/           # Mobile applications
│   ├── ios/         # iOS native app
│   └── android/     # Android native app
├── docs/            # Documentation
└── tests/           # Unit and integration tests
```