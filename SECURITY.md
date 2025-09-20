# Security Architecture & Encryption Strategy

## Security Principles

1. **Zero-Knowledge**: Service never has access to unencrypted data
2. **Defense in Depth**: Multiple layers of encryption
3. **Principle of Least Privilege**: Minimal permissions, segmented access
4. **Forward Secrecy**: Compromise of long-term keys doesn't affect past sessions
5. **Cryptographic Agility**: Ability to upgrade algorithms as needed

## Cryptographic Primitives

### Algorithm Selection

| Purpose | Algorithm | Parameters | Rationale |
|---------|-----------|------------|-----------|
| Master Key Derivation | Argon2id | Time=3, Mem=64MB, Parallel=4 | Memory-hard, resistant to GPU/ASIC attacks |
| Symmetric Encryption | XChaCha20-Poly1305 | 256-bit key, 192-bit nonce | Fast, secure, larger nonce space than AES-GCM |
| Key Exchange | X25519 | - | Fast, constant-time, widely supported |
| Digital Signatures | Ed25519 | - | Fast, small signatures, deterministic |
| Hash Function | BLAKE3 | - | Faster than SHA-3, parallelizable |
| MAC | HMAC-SHA256 | - | Proven security, hardware acceleration |
| Random Generation | OS CSPRNG | - | /dev/urandom, CryptGenRandom, etc. |

### Fallback Algorithms

For compatibility with older systems:
- AES-256-GCM (FIPS compliance)
- RSA-4096 (legacy system integration)
- SHA-256 (wide hardware support)

## Key Management

### Key Hierarchy

```
User Password
      │
      ▼ (Argon2id)
Master Password Key (MPK)
      │
      ├──► Account Key (AK)
      │      └──► Vault Encryption Keys (VEK)
      │
      ├──► Authentication Key (AuthK)
      │      └──► Device Authentication
      │
      └──► Recovery Key (RK)
             └──► Encrypted backup of MPK
```

### Key Derivation Details

```rust
// Master Password Key derivation
fn derive_master_key(password: &str, email: &str) -> MasterKey {
    let salt = blake3::hash(email.as_bytes());
    let key = argon2id::derive(
        password.as_bytes(),
        &salt,
        iterations: 3,
        memory: 65536, // 64 MB
        parallelism: 4,
        key_length: 32
    );
    MasterKey(key)
}

// Sub-key derivation
fn derive_subkey(master: &MasterKey, context: &str) -> SubKey {
    let kdf = hkdf::Hkdf::<Sha256>::new(None, master.0);
    let mut key = [0u8; 32];
    kdf.expand(context.as_bytes(), &mut key);
    SubKey(key)
}
```

### Key Rotation

- **Device Keys**: Rotate every 90 days
- **Session Keys**: Rotate every 24 hours
- **Master Key**: User-initiated only
- **Emergency Re-encryption**: Support for algorithm migration

## Encryption Layers

### Layer 1: Database Encryption (SQLCipher)

```sql
PRAGMA key = 'derived-database-key';
PRAGMA cipher_page_size = 4096;
PRAGMA kdf_iter = 256000;
PRAGMA cipher_hmac_algorithm = HMAC_SHA256;
PRAGMA cipher_kdf_algorithm = PBKDF2_HMAC_SHA256;
```

### Layer 2: Field-Level Encryption

```rust
struct EncryptedField {
    ciphertext: Vec<u8>,
    nonce: [u8; 24],
    version: u8,
}

fn encrypt_field(plaintext: &[u8], key: &Key) -> EncryptedField {
    let nonce = generate_nonce();
    let cipher = XChaCha20Poly1305::new(key);
    let ciphertext = cipher.encrypt(&nonce, plaintext);

    EncryptedField {
        ciphertext,
        nonce,
        version: 1,
    }
}
```

### Layer 3: Transport Encryption (Noise Protocol)

Using Noise_IK_25519_ChaChaPoly_BLAKE2s pattern:
- I: Initiator's static key is immediately transmitted
- K: Responder's static key is known in advance

```
Initiator                    Responder
  -> e, es, s, ss
  <- e, ee, se
  -> payload
  <- payload
```

## Authentication & Authorization

### Multi-Factor Authentication

1. **Primary**: Master password
2. **Secondary Options**:
   - TOTP (RFC 6238)
   - WebAuthn/FIDO2
   - Biometrics (via platform APIs)

### Device Authentication Flow

```rust
struct DeviceAuth {
    device_id: Uuid,
    device_key: Ed25519KeyPair,
    user_approval: Signature,
    capabilities: Vec<Capability>,
    expires: Timestamp,
}

// Initial pairing
fn pair_device(master_key: &MasterKey, new_device: DeviceInfo) -> DeviceAuth {
    // 1. Generate device keypair
    let device_key = Ed25519KeyPair::generate();

    // 2. Create capability grant
    let grant = CapabilityGrant {
        device_id: new_device.id,
        public_key: device_key.public,
        capabilities: vec![Capability::Read, Capability::Write],
        granted_at: now(),
    };

    // 3. Sign with user's auth key
    let auth_key = derive_subkey(master_key, "auth");
    let signature = auth_key.sign(&grant);

    DeviceAuth { ... }
}
```

## Data Protection

### Password Entry Encryption

```rust
struct PasswordEntry {
    id: Uuid,
    // Searchable (encrypted but with deterministic encryption for indexing)
    title_search: DeterministicEncrypted<String>,

    // Fully encrypted fields
    title: Encrypted<String>,
    username: Encrypted<String>,
    password: Encrypted<String>,
    notes: Encrypted<String>,

    // Metadata (not encrypted)
    created: Timestamp,
    modified: Timestamp,
    accessed: Timestamp,
}
```

### Secure Password Generation

```rust
fn generate_password(config: PasswordConfig) -> String {
    let mut rng = OsRng;
    let charset = build_charset(config);

    // Generate with excess length
    let mut password: Vec<char> = (0..config.length + 10)
        .map(|_| charset[rng.gen_range(0..charset.len())])
        .collect();

    // Ensure requirements are met
    ensure_requirements(&mut password, &config);

    // Truncate to requested length
    password.truncate(config.length);
    password.shuffle(&mut rng);

    password.into_iter().collect()
}
```

## Memory Protection

### Sensitive Data Handling

```rust
use zeroize::Zeroize;

struct SecureString(Vec<u8>);

impl Drop for SecureString {
    fn drop(&mut self) {
        self.0.zeroize();
    }
}

impl SecureString {
    fn mlock(&self) -> Result<()> {
        // Prevent swapping to disk
        unsafe {
            mlock(
                self.0.as_ptr() as *const c_void,
                self.0.len()
            )?;
        }
        Ok(())
    }
}
```

### Memory Encryption (Windows/macOS)

- Windows: Use `CryptProtectMemory`
- macOS: Use Keychain Services for sensitive data
- Linux: Use kernel keyring where available

## Threat Model

### Threats Addressed

1. **Network Attacks**
   - Man-in-the-middle: Mitigated by E2E encryption
   - Replay attacks: Nonces and timestamps
   - Traffic analysis: Padding and timing obfuscation

2. **Device Compromise**
   - Stolen device: Encrypted at rest, biometric locks
   - Malware: Process isolation, secure enclave usage
   - Memory dumps: Memory protection, key zeroization

3. **Cryptographic Attacks**
   - Brute force: Strong KDF parameters
   - Quantum computers: Post-quantum ready architecture
   - Side channels: Constant-time implementations

### Threats Out of Scope

1. Hardware keyloggers
2. Compromised OS kernel
3. Physical coercion
4. Supply chain attacks on dependencies

## Security Audit Trail

### Audit Events

```rust
enum AuditEvent {
    Login { device: DeviceId, success: bool },
    PasswordAccess { entry: Uuid, action: AccessType },
    DevicePaired { device: DeviceId },
    DeviceRevoked { device: DeviceId },
    MasterPasswordChanged,
    ExportPerformed { format: ExportFormat },
    SyncCompleted { peer: DeviceId, changes: u32 },
}
```

### Secure Logging

```rust
fn log_audit_event(event: AuditEvent) {
    let entry = AuditEntry {
        timestamp: SystemTime::now(),
        event,
        signature: sign_event(&event),
    };

    // Store locally (encrypted)
    store_local_audit(entry);

    // Optional: Send to external SIEM
    if config.external_audit {
        send_to_siem(entry);
    }
}
```

## Incident Response

### Key Compromise Response

1. **Immediate Actions**:
   - Revoke compromised device keys
   - Force re-authentication on all devices
   - Generate security alert

2. **Recovery**:
   - Re-encrypt all data with new keys
   - Audit access logs
   - Update recovery keys

### Emergency Access

```rust
struct EmergencyKit {
    recovery_key: EncryptedKey,
    backup_codes: Vec<BackupCode>,
    trusted_contact: Option<TrustedContact>,
}

// Shamir's Secret Sharing for recovery key
fn split_recovery_key(key: &RecoveryKey, threshold: u8, shares: u8) -> Vec<Share> {
    shamir::split_secret(key.as_bytes(), threshold, shares)
}
```

## Security Headers & Configuration

### API Security Headers

```
Strict-Transport-Security: max-age=31536000; includeSubDomains
Content-Security-Policy: default-src 'self'
X-Content-Type-Options: nosniff
X-Frame-Options: DENY
Permissions-Policy: geolocation=(), camera=(), microphone=()
```

### Platform-Specific Hardening

**iOS**:
- Keychain Services for key storage
- Data Protection API
- Face ID/Touch ID integration

**Android**:
- Android Keystore
- BiometricPrompt API
- StrongBox Keymaster (when available)

**Desktop**:
- OS credential managers
- TPM integration (Windows)
- Secure Enclave (macOS)