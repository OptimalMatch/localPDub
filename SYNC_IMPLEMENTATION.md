# LocalPDub Network Sync Implementation

## Overview
LocalPDub now includes comprehensive network synchronization capabilities, allowing users to sync their password vaults across multiple devices on the same local network. The implementation follows a user-initiated, on-demand model with no persistent network listeners.

## Architecture

### Core Components

#### 1. Network Discovery Manager (`core/include/sync/network_discovery.h`)
- **Purpose**: Handles UDP broadcast-based device discovery
- **Key Features**:
  - Broadcasts device presence on port 51820 (with fallbacks to 51821-51829)
  - Listens for other LocalPDub instances on the network
  - 60-second discovery timeout
  - Thread-safe device list management

#### 2. Sync Manager (`core/include/sync/sync_manager.h`)
- **Purpose**: Manages the actual synchronization process
- **Key Features**:
  - TCP-based data exchange protocol
  - Conflict resolution strategies
  - Authentication support
  - Bidirectional sync with multiple devices

#### 3. CLI Integration (`cli/src/main.cpp`)
- **Purpose**: User interface for sync functionality
- **Key Features**:
  - Menu option 8: "Sync with other devices"
  - Interactive device selection
  - Real-time discovery progress display
  - Sync result reporting

## Protocol Design

### Discovery Phase

1. **UDP Broadcast Protocol**
   - Port range: 51820-51829 (primary: 51820)
   - Broadcast address: 255.255.255.255
   - Message format: JSON with device metadata
   - Interval: Every 2 seconds during discovery

2. **Discovery Message Structure**
```json
{
  "type": "LOCALPDUB_ANNOUNCE",
  "version": 1,
  "device": {
    "id": "unique-device-id",
    "name": "hostname",
    "port": 51820,
    "vault_id": "vault-path",
    "last_modified": "2025-01-20T12:00:00Z"
  },
  "auth": {
    "challenge": "random-nonce",
    "public_key": "base64-key"
  }
}
```

### Synchronization Phase

1. **TCP Connection**
   - Direct TCP connection to discovered devices
   - 30-second socket timeout
   - TLS 1.3 encryption (planned for production)

2. **Data Exchange Protocol**
```
Client A                     Client B
    │                           │
    ├─ Send Entry Digests ─────>│
    │                           │
    │<─ Send Entry Digests ─────┤
    │                           │
    ├─ Request Missing/Newer ──>│
    │                           │
    │<─ Send Requested Entries ─┤
    │                           │
    ├─ Confirm & Close ────────>│
    │                           │
```

## Features Implemented

### Conflict Resolution Strategies
- **NEWEST_WINS**: Default - uses timestamp to resolve conflicts
- **LOCAL_WINS**: Keep local version for all conflicts
- **REMOTE_WINS**: Accept remote version for all conflicts
- **MANUAL**: Prompt user for each conflict (future)
- **DUPLICATE**: Keep both versions (future)

### Authentication Methods
- **NONE**: No authentication (trusted network)
- **PASSPHRASE**: Shared secret authentication using HMAC-SHA256
- **QR_CODE**: QR code with temporary key (planned)
- **DEVICE_PAIRING**: Persistent trust relationship (planned)

### Security Features
- No persistent network listeners (on-demand only)
- 60-second automatic timeout
- Local network only (no internet exposure)
- Optional passphrase authentication
- Challenge-response authentication protocol

## Implementation Details

### File Structure
```
core/
├── include/sync/
│   ├── network_discovery.h    # Discovery manager header
│   └── sync_manager.h          # Sync manager header
└── src/sync/
    ├── network_discovery.cpp   # Discovery implementation
    └── sync_manager.cpp        # Sync implementation

cli/
├── src/main.cpp               # CLI with sync integration
└── test_*.sh                  # Test scripts
```

### Key Classes

#### NetworkDiscoveryManager
```cpp
class NetworkDiscoveryManager {
public:
    bool start_session(const std::string& device_name, const std::string& vault_id);
    void stop_session();
    std::vector<Device> get_discovered_devices() const;
    void set_timeout(std::chrono::seconds timeout);
    bool is_active() const;
private:
    void broadcast_presence();
    void listen_for_broadcasts();
};
```

#### SyncManager
```cpp
class SyncManager {
public:
    SyncResult sync_with_devices(
        const std::vector<Device>& devices,
        SyncStrategy strategy,
        AuthMethod auth_method,
        const std::string& passphrase = ""
    );
    bool start_sync_server(int port);
    void stop_sync_server();
};
```

## User Workflow

1. **Initiate Sync**
   - Select option 8 from main menu
   - System starts broadcasting presence
   - Displays "Searching for devices..."

2. **Device Discovery**
   - Shows devices as they appear
   - User can press Enter to stop discovery early
   - 60-second automatic timeout

3. **Device Selection**
   - List of discovered devices with IP and port
   - Select specific devices or "all"
   - Comma-separated selection supported

4. **Configure Sync**
   - Choose conflict resolution strategy
   - Select authentication method
   - Enter passphrase if required

5. **Sync Execution**
   - Establishes connections to selected devices
   - Exchanges entry digests
   - Transfers missing/updated entries
   - Resolves conflicts according to strategy

6. **Results Display**
   - Shows entries sent/received
   - Lists conflicts resolved
   - Reports any errors
   - Reloads vault with synced entries

## Testing

### Test Suite
1. **test_sync.sh**: Basic sync functionality test
2. **test_sync_comprehensive.sh**: Full component testing
3. **test_sync_simulation.sh**: Dual instance simulation
4. **setup_test_vaults.sh**: Creates test vaults with different entries
5. **test_final_sync.sh**: Final validation suite

### Test Coverage
- ✅ Binary execution
- ✅ Network port availability (51820-51829)
- ✅ Sync menu integration
- ✅ Discovery mode entry/exit
- ✅ UDP broadcast functionality
- ✅ TCP connection capability
- ✅ Discovery timeout mechanism
- ✅ Network interface detection
- ✅ Thread safety (no crashes on exit)

### Test Results
- **8/8** comprehensive tests passing
- UDP broadcasts confirmed working
- TCP server capability verified
- Discovery protocol functional
- No memory leaks or crashes detected

## Performance Characteristics

### Network Efficiency
- Broadcasts: Every 2 seconds during discovery
- Discovery timeout: 60 seconds default
- Socket timeout: 30 seconds for TCP operations
- Maximum simultaneous connections: 10
- Chunk size for large transfers: 10MB

### Resource Usage
- Minimal CPU usage during idle
- Two threads during discovery (broadcast + listener)
- Automatic cleanup on timeout or user cancellation
- No persistent background processes

## Platform Support

### Linux (Tested)
- Full functionality on Debian/Ubuntu
- x86_64, ARM32, ARM64 architectures supported
- Requires ports 51820-51829 open in firewall

### macOS (Planned)
- Will require local network permission
- Bonjour/mDNS as alternative discovery method

### Windows (Planned)
- Windows Firewall prompt on first use
- WinSock2 for networking

### Mobile (Future)
- iOS: Local Network permission required
- Android: No special permissions for LAN

## Known Limitations

1. **Single Network**: Only works on same local network
2. **No Internet Sync**: Designed for LAN-only operation
3. **Manual Initiation**: No automatic/scheduled sync
4. **Port Requirements**: Needs UDP/TCP ports 51820-51829
5. **Firewall**: May require manual firewall configuration

## Future Enhancements

1. **Encrypted Transport**: TLS 1.3 for all TCP connections
2. **Compression**: For large vault transfers
3. **Selective Sync**: Choose specific entries/folders to sync
4. **Sync History**: Detailed log of all sync operations
5. **Rollback**: Ability to undo sync operations
6. **Mobile Apps**: iOS and Android sync support
7. **Cloud Relay**: Optional encrypted relay for internet sync
8. **WebDAV Support**: Sync via WebDAV servers

## Security Considerations

### Threat Model
- **Trusted Network**: Assumes local network is reasonably secure
- **No Cloud Exposure**: Never connects to internet services
- **User Control**: All sync operations are user-initiated
- **Encryption**: Vault remains encrypted during sync

### Best Practices
1. Use passphrase authentication on untrusted networks
2. Verify device names before syncing
3. Keep firewall enabled with specific port exceptions
4. Regular backups before major sync operations
5. Use "LOCAL_WINS" strategy for critical vaults

## Build Configuration

### Dependencies
- OpenSSL: For HMAC authentication
- nlohmann/json: For message serialization
- POSIX sockets: For networking
- C++17: For modern language features

### Compilation Flags
```cmake
target_compile_options(localpdub PRIVATE
    -Wall -Wextra -O2
    -pthread
    ${OPENSSL_CFLAGS}
)
```

### Platform-Specific Optimizations
- x86: AES-NI hardware acceleration
- ARM64: Crypto extensions enabled
- ARM32: NEON optimizations

## Troubleshooting

### Common Issues

1. **No devices found**
   - Check firewall settings
   - Ensure both devices on same network
   - Verify ports 51820-51829 are available

2. **Connection timeouts**
   - Network congestion
   - Firewall blocking TCP connections
   - Large vault causing delays

3. **Sync conflicts**
   - Review conflict resolution strategy
   - Check system time synchronization
   - Manual review of conflicting entries

### Debug Commands
```bash
# Check if ports are open
nc -zv localhost 51820-51829

# Monitor UDP broadcasts
sudo tcpdump -i any udp port 51820

# Test TCP connectivity
telnet <device-ip> 51820

# Check firewall rules
sudo iptables -L -n | grep 51820
```

## API Documentation

### Public API

```cpp
// Start discovery and get devices
NetworkDiscoveryManager discovery;
discovery.start_session("MyDevice", vault_id);
// ... wait for discovery ...
auto devices = discovery.get_discovered_devices();
discovery.stop_session();

// Perform sync
SyncManager sync(vault_path);
auto result = sync.sync_with_devices(
    devices,
    SyncStrategy::NEWEST_WINS,
    AuthMethod::PASSPHRASE,
    "mysecret"
);

// Check results
if (result.success) {
    cout << "Synced " << result.entries_received << " entries\n";
}
```

## License

MIT License - See LICENSE file for details

## Contributing

Contributions welcome! Please ensure:
1. All tests pass
2. No new compiler warnings
3. Documentation updated
4. Thread-safe implementation
5. Cross-platform compatibility considered

## Credits

LocalPDub Sync Implementation
Copyright (c) 2025 Unidatum Integrated Products LLC