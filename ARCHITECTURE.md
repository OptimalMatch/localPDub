# LocalPDub - Simple Password Manager Architecture

## Overview

LocalPDub is a secure, cross-platform password manager that stores encrypted passwords locally and provides simple synchronization options between devices.

## Core Principles

1. **Local-First Storage**: All data stored in encrypted files on device
2. **Simple Sync**: Direct file synchronization without complex P2P protocols
3. **Zero-Knowledge**: All encryption/decryption happens client-side
4. **Cross-Platform**: Native apps for all major platforms
5. **Minimal Dependencies**: Lightweight with few external dependencies

## System Architecture

### 1. Application Layers

```
┌─────────────────────────────────────────────┐
│           Platform UI Layer                 │
│  (Native UI for each platform)              │
├─────────────────────────────────────────────┤
│           Core Library (C++)                │
│  (Password management, crypto, file I/O)    │
├─────────────────────────────────────────────┤
│         Cryptography Module                 │
│  (AES-256-GCM, Argon2, SHA-256)            │
├─────────────────────────────────────────────┤
│        File Storage Layer                   │
│  (Encrypted JSON/Binary files)              │
├─────────────────────────────────────────────┤
│          Simple Sync Module                 │
│  (File sync via USB/Network/Cloud folder)   │
└─────────────────────────────────────────────┘
```

### 2. Technology Stack

#### Core Library (Shared across all platforms)
- **Language**: C++17/20
- **Why C++**: Performance, cross-platform support, mature ecosystem
- **Key Libraries**:
  - `OpenSSL` or `Crypto++` - Cryptography
  - `nlohmann/json` - JSON parsing
  - `spdlog` - Logging
  - Standard library for file I/O

#### Platform-Specific UI

**Desktop (Windows/macOS/Linux)**
- **Framework**: Qt 6 or wxWidgets
- **UI**: Native widgets for each platform
- **Alternative**: Electron with Node.js C++ bindings

**Mobile (iOS/Android)**
- **iOS**: Swift UI with C++ bindings via Objective-C++
- **Android**: Kotlin/Java with JNI bindings to C++

### 3. User-Initiated Network Synchronization

#### Sync Philosophy

- **On-Demand Only**: No background services or persistent listeners
- **User-Controlled**: Sync only happens when explicitly initiated
- **Network Visibility**: Clients remain invisible until user starts sync
- **Direct Communication**: Peer-to-peer sync on local network

#### Sync Workflow

1. **User Initiates Sync**
   - Opens sync dialog in application
   - Client begins broadcasting presence on LAN

2. **Device Discovery**
   - UDP broadcast on ports 51820-51829
   - Other LocalPDub clients respond if also in sync mode
   - Available devices shown in list

3. **User Selects Targets**
   - Choose specific devices or "Sync All"
   - Configure conflict resolution strategy

4. **Synchronization**
   - TCP connection established with selected devices
   - Exchange password entry digests
   - Transfer only changed/new entries
   - Apply conflict resolution

5. **Session Ends**
   - Broadcasting stops automatically
   - Network listeners close
   - Clients become invisible again

#### Network Architecture

```
┌──────────────┐     UDP Broadcast      ┌──────────────┐
│   Client A   │ ◄──────────────────────► │   Client B   │
│  (Sync Mode) │                          │  (Sync Mode) │
└──────────────┘                          └──────────────┘
       │              TCP Connection              │
       └──────────────────────────────────────────┘
                    (Data Exchange)
```

### 4. Security Architecture

#### Master Key Derivation
```
User Password → Argon2id → Master Key
                    ↓
         ┌──────────┴──────────┐
         ↓                     ↓
    Encryption Key        Authentication Key
    (AES-256-GCM)         (HMAC-SHA256)
```

#### Multi-Layer Encryption

1. **Database Encryption**: SQLCipher for SQLite
2. **Field-Level Encryption**: Individual password entries
3. **Transport Encryption**: TLS 1.3 / Noise Protocol
4. **Backup Encryption**: Additional passphrase option

#### Device Authentication

- **Initial Pairing**: PAKE (Password Authenticated Key Exchange)
- **Subsequent Connections**: Ed25519 keypairs
- **Device Revocation**: Distributed revocation list

### 5. File Storage Format

#### Vault File Structure

```json
{
  "version": 1,
  "metadata": {
    "created": "2024-01-01T00:00:00Z",
    "modified": "2024-01-01T00:00:00Z",
    "device_id": "unique-device-id"
  },
  "encrypted_data": "base64-encoded-encrypted-json"
}
```

#### Decrypted Data Structure

```json
{
  "entries": [
    {
      "id": "unique-id",
      "title": "Website Name",
      "username": "user@example.com",
      "password": "encrypted-password",
      "url": "https://example.com",
      "notes": "optional notes",
      "tags": ["tag1", "tag2"],
      "created": "timestamp",
      "modified": "timestamp"
    }
  ],
  "categories": [
    {
      "id": "category-id",
      "name": "Work",
      "color": "#FF5733"
    }
  ]
}
```

### 6. Version Control & Rollback

#### Automatic Versioning
- Keep 2 previous versions for quick rollback
- Versions saved before each sync operation
- Rotate versions automatically (oldest deleted)

#### Change Tracking
- All changes recorded in encrypted history file (history.lph)
- Custom binary format (not SQLite) for security
- Track who changed what and when
- Group changes by sync session

#### Rollback Capability
- One-click rollback to previous version
- Rollback entire sync session if corrupted
- View detailed change history before rollback
- Automatic backup before any rollback operation

### 7. Conflict Resolution

Simple timestamp-based resolution:
- Most recent modification wins
- Manual merge option for conflicts
- All conflicts logged in history
- Rollback available if merge goes wrong

### 8. Features Roadmap

#### Phase 1: MVP
- Basic password storage and retrieval
- Master password protection
- File-based encrypted storage
- Manual export/import

#### Phase 2: Basic Sync
- Shared folder monitoring
- Simple LAN transfer
- Timestamp-based conflict resolution

#### Phase 3: Enhanced Features
- Password generation
- Password strength analysis
- Categories and tags
- Search functionality
- Auto-lock timer

#### Phase 4: Advanced Features
- Browser extensions
- TOTP/2FA support
- Biometric authentication
- Password breach checking

## Development Approach

### Repository Structure
```
localPDub/
├── core/                # C++ core library
│   ├── include/        # Public headers
│   ├── src/           # Implementation
│   │   ├── crypto/    # Encryption
│   │   ├── storage/   # File I/O
│   │   ├── sync/      # Sync logic
│   │   └── models/    # Data structures
│   └── CMakeLists.txt
├── desktop/           # Desktop application
│   ├── src/          # Platform-specific code
│   ├── ui/           # UI resources
│   └── CMakeLists.txt
├── mobile/           # Mobile applications
│   ├── ios/         # iOS app
│   ├── android/     # Android app
│   └── shared/      # Shared mobile code
├── docs/            # Documentation
├── tests/           # Unit and integration tests
└── scripts/         # Build scripts
```

## Next Steps

1. Set up C++ core library with CMake
2. Implement basic encryption using OpenSSL/Crypto++
3. Create file-based storage system
4. Build desktop app with Qt or wxWidgets
5. Implement simple file sync
6. Add mobile applications