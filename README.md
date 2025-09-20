# LocalPDub - Simple Password Manager

A secure, cross-platform password manager with local file storage and simple synchronization between devices.

## Features

- 🔐 **Local File Storage**: All data stored in encrypted files on your device
- 🔄 **User-Initiated Sync**: Privacy-first sync - only active when you initiate
- 🛡️ **Strong Encryption**: AES-256-GCM encryption with Argon2 key derivation
- 📱 **Cross-Platform**: Works on Windows, macOS, Linux, iOS, and Android
- 📝 **Comprehensive Fields**: Store passwords, URLs, notes, TOTP, custom fields
- ↩️ **Version Control**: Rollback to previous versions if sync corrupts data
- 📊 **Change History**: Track all modifications with encrypted audit log
- 🚀 **Fast & Lightweight**: C++ core for maximum performance

## Project Structure

```
localPDub/
├── core/               # C++ core library
│   ├── include/       # Public headers
│   ├── src/          # Implementation
│   └── CMakeLists.txt
├── desktop/          # Desktop application
├── mobile/           # Mobile applications
│   ├── ios/         # iOS app
│   └── android/     # Android app
├── docs/            # Documentation
└── tests/           # Unit tests
```

## Building from Source

### Prerequisites

- C++17 compatible compiler
- CMake 3.16+
- OpenSSL or Crypto++ library
- nlohmann/json library

### Core Library

```bash
cd core
mkdir build && cd build
cmake ..
make
make test
```

### Desktop Application

```bash
cd desktop
mkdir build && cd build
cmake ..
make
```

## Quick Start

### Creating a New Vault

```cpp
#include <localpdub/localpdub.h>

localpdub::LocalPDub pdub;
pdub.create_vault("master_password");

// Standard password entry with all fields
localpdub::PasswordEntry entry;
entry.title = "Gmail Account";
entry.username = "user@gmail.com";
entry.password = "SecurePassword123!";
entry.email = "user@gmail.com";
entry.url = "https://mail.google.com";
entry.notes = "Primary email account\nRecovery: backup@yahoo.com";
entry.totp_secret = "JBSWY3DPEHPK3PXP";  // For 2FA

// Add custom fields for security questions, etc.
entry.add_custom_field("Security Question", "First pet's name?");
entry.add_custom_field("Answer", "Fluffy");
entry.add_custom_field("Recovery Codes", "ABC123, DEF456");

// Organize with tags and categories
entry.tags = {"email", "personal", "important"};
entry.favorite = true;

pdub.add_entry(entry);

// Credit card example
localpdub::CreditCardEntry card;
card.title = "Visa Card";
card.card_number = "4111-1111-1111-1111";
card.card_holder_name = "John Doe";
card.expiry_month = "12";
card.expiry_year = "2025";
card.cvv = "123";
card.pin = "1234";
card.bank_name = "Chase";

pdub.add_entry(card);
pdub.save_vault();
```

### Synchronization

LocalPDub uses a user-initiated sync model - devices remain invisible on the network until you explicitly start a sync session.

#### How It Works

1. **Start Sync Mode**: Click "Sync with Other Devices" in the menu
2. **Discovery**: Your device broadcasts and discovers other LocalPDub clients in sync mode
3. **Select Devices**: Choose which devices to sync with from the list
4. **Sync**: Exchange password entries with selected devices
5. **Done**: Sync mode automatically ends, devices become invisible again

#### Key Features

- **Privacy First**: No network activity unless you initiate sync
- **Local Network Only**: Sync happens directly between devices on same network
- **No Background Services**: Application doesn't listen for connections when not syncing
- **User Control**: You decide when and with which devices to sync

## File Format

LocalPDub uses the `.lpd` extension for encrypted vault files:
- Binary format with AES-256-GCM encryption
- Argon2 for key derivation
- JSON structure internally for flexibility

## Security

- **Encryption**: AES-256-GCM
- **Key Derivation**: Argon2id (64MB memory, 3 iterations)
- **No Cloud Storage**: Your passwords never leave your devices
- **Zero-Knowledge**: All encryption happens locally

## Documentation

- [Architecture Overview](ARCHITECTURE.md)
- [Sync Protocol](SYNC_PROTOCOL.md)
- [Security Model](SECURITY.md)
- [Data Format](DATA_MODEL.md)

## Contributing

This project is in early development. Contributions are welcome! Please feel free to submit pull requests, report bugs, or suggest features.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Disclaimer

This software is under active development. Use at your own risk.