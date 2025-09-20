# Building LocalPDub

## Prerequisites (Debian/Ubuntu)

Install required dependencies:

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    libssl-dev \
    libargon2-dev \
    nlohmann-json3-dev \
    pkg-config
```

## Building

### Quick Build

```bash
# Run the build script
./build.sh

# The binary will be at: ./build/localpdub
```

### Manual Build

```bash
# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake ../cli

# Build
make -j$(nproc)

# Run
./localpdub
```

## Installation

To install system-wide:

```bash
cd build
sudo make install

# Now you can run from anywhere:
localpdub
```

## First Run

When you first run LocalPDub:

1. **Create Master Password**: You'll be prompted to create a master password (minimum 8 characters)
2. **Vault Location**: Your vault is stored at `~/.localpdub/vault.lpd`
3. **Security**: All data is encrypted with AES-256-GCM

## Usage

### Main Menu Options

1. **List all entries** - View all password entries
2. **Search entries** - Find entries by title, username, or URL
3. **Add new entry** - Create a new password entry
4. **View entry details** - See full details of an entry
5. **Edit entry** - Modify existing entry
6. **Delete entry** - Remove an entry
7. **Generate password** - Create strong passwords
8. **Save and exit** - Save changes and quit
9. **Exit without saving** - Quit without saving changes

### Password Entry Fields

Each entry can store:
- Title (required)
- Username
- Password
- URL
- Email
- Notes
- Custom fields (unlimited key-value pairs)

### Security Features

- Master password protected
- Argon2id key derivation (64MB memory, 3 iterations)
- AES-256-GCM encryption
- Secure password generation
- Passwords masked by default in view

## Troubleshooting

### Build Errors

If you get "library not found" errors:
```bash
# Check if all dependencies are installed
pkg-config --list-all | grep -E "argon2|openssl|nlohmann"
```

### Runtime Errors

If the application won't start:
```bash
# Check library dependencies
ldd ./build/localpdub

# Create vault directory manually if needed
mkdir -p ~/.localpdub
```

## Development

### Project Structure
```
localPDub/
├── core/               # Core crypto and storage
│   ├── include/       # Header files
│   └── src/          # Implementation
├── cli/              # Command-line interface
│   ├── src/          # CLI source
│   └── CMakeLists.txt
└── build/            # Build output
    └── localpdub     # Executable
```

### Testing

Run the application with a test vault:
```bash
# Create test directory
mkdir -p ~/test_vault

# Run with custom vault location
./build/localpdub --vault ~/test_vault/test.lpd
```

## License

MIT License - See LICENSE file for details