# File-Based Storage Model

## Overview

LocalPDub uses encrypted file storage instead of a database. All password data is stored in a single encrypted vault file that can be easily backed up, transferred, and synchronized.

## File Structure

### Primary Vault File

Location: `~/.localpdub/vault.lpd`

The vault file contains all password entries, categories, and settings in an encrypted format.

### Directory Layout

```
~/.localpdub/
├── vault.lpd              # Primary encrypted vault
├── vault.lpd.lock         # Lock file for concurrent access
├── vault.key              # Encrypted key file (optional)
├── versions/              # Previous vault versions for rollback
│   ├── vault.lpd.v1       # Most recent previous version
│   ├── vault.lpd.v2       # Second most recent version
│   └── metadata.json      # Version metadata and history
├── history/               # Change history
│   ├── history.lph        # Encrypted binary history file
│   └── exports/           # Exported history reports
├── backups/
│   ├── auto/             # Automatic backups
│   │   └── vault-[timestamp].lpd
│   └── manual/           # User-initiated backups
│       └── vault-[name].lpd
├── config.json           # Application settings (non-sensitive)
├── sync/
│   ├── devices.json      # Known devices
│   └── conflicts/        # Unresolved conflicts
└── temp/                 # Temporary files
```

## Data Structures

### Vault Structure (Decrypted)

```cpp
struct Vault {
    VaultMetadata metadata;
    std::vector<PasswordEntry> entries;
    std::vector<Category> categories;
    std::map<std::string, std::string> settings;
};

struct VaultMetadata {
    uint32_t version;
    std::string vault_id;
    std::string device_id;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point modified_at;
    uint32_t entry_count;
};
```

### Password Entry

```cpp
struct PasswordEntry {
    // Core Fields
    std::string id;           // UUID
    std::string title;        // Display name (e.g., "Gmail Account")
    std::string username;     // Username or email
    std::string password;     // Encrypted password

    // Additional Standard Fields
    std::string url;          // Website URL
    std::string notes;        // Free-form encrypted notes
    std::string email;        // Separate email field (if different from username)
    std::string totp_secret;  // For 2FA/TOTP codes

    // Organization
    std::vector<std::string> tags;      // Multiple tags for categorization
    std::string category_id;            // Link to category
    bool favorite;                       // Quick access flag

    // Custom Fields (key-value pairs for flexibility)
    std::map<std::string, std::string> custom_fields;
    // Examples:
    // - "Security Question" -> "What's your pet's name?"
    // - "Account Number" -> "123456789"
    // - "PIN" -> "1234"
    // - "Recovery Codes" -> "ABC123, DEF456, GHI789"
    // - "License Key" -> "XXXX-XXXX-XXXX-XXXX"

    // Metadata
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point modified_at;
    std::chrono::system_clock::time_point accessed_at;
    std::chrono::system_clock::time_point password_changed_at;
    std::chrono::system_clock::time_point expires_at;  // Optional expiration

    // Statistics
    uint32_t access_count;
    uint32_t password_strength_score;  // 0-100 strength indicator
};
```

### Entry Types

Beyond standard password entries, the system supports specialized entry types:

```cpp
enum class EntryType {
    PASSWORD,      // Standard login credentials
    SECURE_NOTE,   // Encrypted text note
    CREDIT_CARD,   // Credit card information
    IDENTITY,      // Personal identity info (passport, license, etc.)
    WIFI,          // WiFi network credentials
    SERVER,        // Server/SSH credentials
    API_KEY,       // API keys and tokens
    DATABASE,      // Database connections
    CRYPTO_WALLET  // Cryptocurrency wallet info
};

// Specialized structures for different types
struct CreditCardEntry : PasswordEntry {
    std::string card_number;
    std::string card_holder_name;
    std::string expiry_month;
    std::string expiry_year;
    std::string cvv;
    std::string billing_address;
    std::string bank_name;
};

struct IdentityEntry : PasswordEntry {
    std::string full_name;
    std::string document_number;
    std::string document_type;  // passport, license, etc.
    std::string issue_date;
    std::string expiry_date;
    std::string country;
    std::string additional_info;
};

struct ServerEntry : PasswordEntry {
    std::string hostname;
    std::string port;
    std::string ssh_key;
    std::string connection_type;  // SSH, FTP, RDP, etc.
};
```

### Category

```cpp
struct Category {
    std::string id;
    std::string name;
    std::string icon;
    std::string color;
    uint32_t sort_order;
};
```

## JSON Serialization Format

### Vault JSON (before encryption)

```json
{
  "metadata": {
    "version": 1,
    "vault_id": "550e8400-e29b-41d4-a716-446655440000",
    "device_id": "device-uuid",
    "created_at": "2024-01-01T00:00:00Z",
    "modified_at": "2024-01-01T12:00:00Z",
    "entry_count": 42
  },
  "entries": [
    {
      "id": "entry-uuid-1",
      "type": "password",
      "title": "Gmail Account",
      "username": "john.doe@gmail.com",
      "password": "MySecurePassword123!",
      "email": "john.doe@gmail.com",
      "url": "https://mail.google.com",
      "notes": "Primary email account\nRecovery phone: 555-0123\nBackup email: john.backup@yahoo.com",
      "totp_secret": "JBSWY3DPEHPK3PXP",
      "tags": ["email", "personal", "important"],
      "category_id": "personal-cat",
      "custom_fields": {
        "Security Question 1": "First pet's name",
        "Answer 1": "Fluffy",
        "Security Question 2": "Mother's maiden name",
        "Answer 2": "Smith",
        "Recovery Codes": "ABC123, DEF456, GHI789, JKL012, MNO345"
      },
      "favorite": true,
      "created_at": "2024-01-01T00:00:00Z",
      "modified_at": "2024-01-01T00:00:00Z",
      "accessed_at": "2024-01-01T00:00:00Z",
      "password_changed_at": "2024-01-01T00:00:00Z",
      "expires_at": null,
      "access_count": 142,
      "password_strength_score": 85
    },
    {
      "id": "entry-uuid-2",
      "type": "credit_card",
      "title": "Chase Visa Card",
      "card_number": "4111-1111-1111-1111",
      "card_holder_name": "John Doe",
      "expiry_month": "12",
      "expiry_year": "2025",
      "cvv": "123",
      "bank_name": "Chase Bank",
      "billing_address": "123 Main St, City, ST 12345",
      "notes": "Primary credit card for online purchases",
      "tags": ["financial", "credit-card"],
      "category_id": "financial-cat",
      "custom_fields": {
        "PIN": "1234",
        "Customer Service": "1-800-555-0123",
        "Account Number": "987654321"
      },
      "favorite": false
    },
    {
      "id": "entry-uuid-3",
      "type": "server",
      "title": "Production Server",
      "hostname": "prod.example.com",
      "port": "22",
      "username": "admin",
      "password": "ServerPassword456!",
      "connection_type": "SSH",
      "url": "ssh://prod.example.com:22",
      "notes": "Main production server\nIP: 192.168.1.100\nOS: Ubuntu 22.04",
      "ssh_key": "-----BEGIN RSA PRIVATE KEY-----\n[key data]\n-----END RSA PRIVATE KEY-----",
      "tags": ["work", "server", "production"],
      "category_id": "work-cat",
      "custom_fields": {
        "Root Password": "RootPass789!",
        "Database Password": "DbPass321!",
        "Backup Location": "/backup/daily/",
        "Monitoring URL": "https://monitor.example.com"
      },
      "favorite": true
    },
    {
      "id": "entry-uuid-4",
      "type": "secure_note",
      "title": "WiFi Passwords",
      "notes": "Home Network:\nSSID: HomeWiFi\nPassword: MyHomeNetwork123\nRouter IP: 192.168.1.1\nAdmin: admin/admin123\n\nGuest Network:\nSSID: GuestWiFi\nPassword: Welcome2024",
      "tags": ["network", "wifi"],
      "category_id": "personal-cat"
    }
  ],
  "categories": [
    {
      "id": "category-uuid",
      "name": "Work",
      "icon": "briefcase",
      "color": "#2196F3",
      "sort_order": 1
    }
  ],
  "settings": {
    "auto_lock_minutes": "5",
    "clipboard_timeout": "30",
    "password_generator_length": "20"
  }
}
```

## Binary File Format

### File Header

```cpp
struct FileHeader {
    char magic[4];          // "LPDV"
    uint16_t version;       // File format version
    uint16_t flags;         // Compression, encryption type
    uint32_t header_size;   // Size of header
    uint32_t data_size;     // Size of encrypted data
};
```

### Encrypted File Structure

```
[Header (16 bytes)]
[Salt (32 bytes)]       // For key derivation
[Nonce (12 bytes)]      // For AES-GCM
[Encrypted Data (N bytes)]
[MAC (16 bytes)]        // Authentication tag
```

## Encryption Details

### Key Derivation

```cpp
class KeyDerivation {
public:
    static std::vector<uint8_t> derive_key(
        const std::string& password,
        const std::vector<uint8_t>& salt
    ) {
        // Using Argon2id
        uint32_t t_cost = 3;        // iterations
        uint32_t m_cost = 65536;    // 64MB memory
        uint32_t parallelism = 4;   // threads

        return argon2id_hash(password, salt, t_cost, m_cost, parallelism);
    }
};
```

### Encryption/Decryption

```cpp
class VaultCrypto {
private:
    std::vector<uint8_t> key;

public:
    std::vector<uint8_t> encrypt(const std::string& plaintext) {
        // Generate random nonce
        auto nonce = generate_random_bytes(12);

        // Encrypt using AES-256-GCM
        auto ciphertext = aes_gcm_encrypt(plaintext, key, nonce);

        // Combine nonce + ciphertext + tag
        return combine(nonce, ciphertext);
    }

    std::string decrypt(const std::vector<uint8_t>& encrypted) {
        // Extract nonce, ciphertext, and tag
        auto [nonce, ciphertext, tag] = split(encrypted);

        // Decrypt and verify
        return aes_gcm_decrypt(ciphertext, key, nonce, tag);
    }
};
```

## File Operations

### Reading Vault

```cpp
class VaultReader {
public:
    Vault read_vault(const std::filesystem::path& path,
                     const std::string& password) {
        // 1. Read file header
        auto header = read_header(path);

        // 2. Validate magic and version
        validate_header(header);

        // 3. Read salt and derive key
        auto salt = read_salt(path);
        auto key = derive_key(password, salt);

        // 4. Read and decrypt data
        auto encrypted = read_encrypted_data(path);
        auto json = decrypt(encrypted, key);

        // 5. Parse JSON to Vault object
        return parse_vault_json(json);
    }
};
```

### Writing Vault

```cpp
class VaultWriter {
public:
    void write_vault(const Vault& vault,
                     const std::filesystem::path& path,
                     const std::string& password) {
        // 1. Create backup
        create_backup(path);

        // 2. Serialize vault to JSON
        auto json = serialize_vault(vault);

        // 3. Generate salt and derive key
        auto salt = generate_salt();
        auto key = derive_key(password, salt);

        // 4. Encrypt data
        auto encrypted = encrypt(json, key);

        // 5. Write to temporary file
        auto temp_path = write_temp_file(encrypted, salt);

        // 6. Atomic rename
        std::filesystem::rename(temp_path, path);
    }
};
```

## Version Control & Rollback

### Version Management

The system maintains 2 previous versions for quick rollback:

```cpp
class VersionManager {
public:
    void save_version() {
        // Rotate versions: v2 -> (delete), v1 -> v2, current -> v1
        if (exists("vault.lpd.v2")) {
            remove("vault.lpd.v2");
        }
        if (exists("vault.lpd.v1")) {
            rename("vault.lpd.v1", "vault.lpd.v2");
        }
        copy("vault.lpd", "vault.lpd.v1");

        // Update metadata
        update_version_metadata();
    }

    bool rollback(int version_back = 1) {
        if (version_back == 1 && exists("vault.lpd.v1")) {
            // Save current as temporary
            rename("vault.lpd", "vault.lpd.tmp");
            // Restore version 1
            copy("vault.lpd.v1", "vault.lpd");
            // Make temp the new v1
            rename("vault.lpd.tmp", "vault.lpd.v1");
            return true;
        }
        // Similar for version 2
        return false;
    }
};
```

### Version Metadata

```json
{
  "versions": [
    {
      "version": 1,
      "file": "vault.lpd.v1",
      "created_at": "2024-01-01T12:00:00Z",
      "source": "sync",
      "device": "John's Desktop",
      "entries_count": 150,
      "size_bytes": 45678,
      "hash": "sha256-hash"
    },
    {
      "version": 2,
      "file": "vault.lpd.v2",
      "created_at": "2024-01-01T11:00:00Z",
      "source": "manual_save",
      "device": "Current Device",
      "entries_count": 148,
      "size_bytes": 45123,
      "hash": "sha256-hash"
    }
  ],
  "current": {
    "last_modified": "2024-01-01T13:00:00Z",
    "entries_count": 152
  }
}
```

## Change History

### History File Format

Change history is stored in an encrypted binary file with custom structure:

```cpp
// History file: history.lph (LocalPDub History)
struct HistoryFile {
    FileHeader header;
    std::vector<ChangeRecord> changes;
    std::vector<SyncSession> sessions;
};

struct FileHeader {
    char magic[4];          // "LPHX"
    uint16_t version;       // Format version
    uint32_t record_count;
    uint32_t session_count;
    uint64_t file_size;
    uint8_t compression;    // 0=none, 1=zlib
    uint8_t reserved[16];
};

struct ChangeRecord {
    uint64_t timestamp;     // Unix timestamp
    uint32_t record_id;
    uint8_t operation;      // 0=add, 1=update, 2=delete, 3=sync
    char entry_id[36];      // UUID
    uint16_t title_len;
    uint8_t* title;         // Encrypted
    uint8_t field_type;     // Which field changed
    uint32_t old_value_len;
    uint8_t* old_value;     // Encrypted
    uint32_t new_value_len;
    uint8_t* new_value;     // Encrypted
    uint8_t source;         // 0=local, 1=sync, 2=import
    char device_id[36];
    char session_id[36];    // Links to sync session
};

struct SyncSession {
    char id[36];
    uint64_t started_at;
    uint64_t completed_at;
    char device_name[64];   // Encrypted
    uint16_t entries_sent;
    uint16_t entries_received;
    uint16_t conflicts_resolved;
    uint8_t rollback_available;
};
```

### Binary Layout

```
[Magic: 4 bytes] "LPHX"
[Version: 2 bytes]
[Flags: 2 bytes]
[Salt: 32 bytes]
[Nonce: 12 bytes]
[Encrypted Block Start]
  [Record Count: 4 bytes]
  [Session Count: 4 bytes]
  [Change Records: Variable]
    [Record 1]
    [Record 2]
    ...
  [Sync Sessions: Variable]
    [Session 1]
    [Session 2]
    ...
[Encrypted Block End]
[MAC: 16 bytes]
```

### Change Tracking

```cpp
class ChangeTracker {
private:
    struct HistoryCache {
        std::vector<ChangeRecord> changes;
        std::vector<SyncSession> sessions;
        bool dirty = false;
    } cache;

    std::filesystem::path history_file{"history.lph"};

public:
    void record_change(const ChangeRecord& change) {
        // Load existing history
        load_history();

        // Add new change
        cache.changes.push_back(change);
        cache.dirty = true;

        // Keep only last 1000 changes for performance
        if (cache.changes.size() > 1000) {
            cache.changes.erase(
                cache.changes.begin(),
                cache.changes.begin() + (cache.changes.size() - 1000)
            );
        }

        // Save encrypted
        save_history();
    }

    std::vector<ChangeRecord> get_recent_changes(int limit = 100) {
        load_history();

        std::vector<ChangeRecord> recent;
        int start = std::max(0, (int)cache.changes.size() - limit);

        for (int i = start; i < cache.changes.size(); i++) {
            recent.push_back(cache.changes[i]);
        }

        return recent;
    }

private:
    void save_history() {
        // Generate encryption key from master password
        auto key = derive_history_key();

        // Serialize to binary
        std::vector<uint8_t> data;
        serialize_history(data);

        // Encrypt entire block
        auto encrypted = encrypt_aes_gcm(data, key);

        // Write with header
        std::ofstream file(history_file, std::ios::binary);
        write_header(file);
        file.write((char*)encrypted.data(), encrypted.size());
    }

    void load_history() {
        if (!std::filesystem::exists(history_file)) {
            return;
        }

        std::ifstream file(history_file, std::ios::binary);
        auto header = read_header(file);

        // Read encrypted data
        std::vector<uint8_t> encrypted(header.file_size);
        file.read((char*)encrypted.data(), encrypted.size());

        // Decrypt
        auto key = derive_history_key();
        auto decrypted = decrypt_aes_gcm(encrypted, key);

        // Deserialize
        deserialize_history(decrypted);
    }
};
```

### History View UI

```
┌────────────────────────────────────────────────┐
│  Change History                                │
├────────────────────────────────────────────────┤
│ Today                                          │
│   14:32 - Synced with John's Desktop          │
│           • Added: 5 entries                   │
│           • Updated: 3 entries                 │
│           • Conflicts resolved: 1              │
│           [View Details] [Rollback]            │
│                                                │
│   10:15 - Updated: Gmail Password             │
│           • Password changed                   │
│                                                │
│ Yesterday                                      │
│   16:45 - Added: Banking Login                │
│   09:30 - Deleted: Old Forum Account          │
│                                                │
│ [Load More] [Export History] [Clear History]   │
└────────────────────────────────────────────────┘
```

## Backup Strategy

### Automatic Backups

- Before each sync operation (mandatory)
- Before version rotation
- Daily backups kept for 7 days
- Weekly backups kept for 4 weeks

### Backup File Naming

```
vault-2024-01-01T12:00:00.lpd    # Automatic
vault-before-sync-2024-01-01.lpd # Before sync
vault-manual-description.lpd      # Manual
```

### Rollback Safety

Before any potentially destructive operation:

1. Save current version to versions/
2. Create backup in backups/auto/
3. Record operation in change history
4. Perform operation
5. Verify integrity
6. If failed, automatic rollback available

## Performance Considerations

### Memory Management

```cpp
class SecureString {
private:
    std::vector<char> data;

public:
    ~SecureString() {
        // Overwrite memory before deallocation
        std::fill(data.begin(), data.end(), 0);
    }
};
```

### File Locking

```cpp
class FileLock {
public:
    bool try_lock(const std::filesystem::path& path) {
        lock_path = path.string() + ".lock";

        // Try to create lock file exclusively
        lock_file.open(lock_path, std::ios::out | std::ios::excl);

        if (lock_file.is_open()) {
            // Write PID and timestamp
            lock_file << std::this_thread::get_id() << "\n";
            lock_file << std::chrono::system_clock::now();
            return true;
        }

        return false;
    }
};
```

### Caching

```cpp
class VaultCache {
private:
    Vault cached_vault;
    std::string cached_password_hash;
    std::chrono::time_point last_access;
    std::chrono::minutes timeout{5};

public:
    std::optional<Vault> get_cached(const std::string& password_hash) {
        if (password_hash == cached_password_hash &&
            std::chrono::steady_clock::now() - last_access < timeout) {
            last_access = std::chrono::steady_clock::now();
            return cached_vault;
        }
        return std::nullopt;
    }
};
```

## Migration from Other Formats

### Import Formats

- KeePass XML (.xml)
- 1Password (.1pif, .opvault)
- LastPass CSV (.csv)
- Bitwarden JSON (.json)
- Chrome/Firefox CSV

### Export Formats

- Encrypted LocalPDub (.lpd)
- Unencrypted JSON (with warning)
- CSV (with warning)
- KeePass XML compatible