# User-Initiated Network Sync Protocol

## Overview

LocalPDub uses an on-demand, user-initiated synchronization model. Clients remain invisible on the network until the user explicitly initiates sync mode. Once activated, clients broadcast their availability and can discover other LocalPDub instances on the same network.

## Sync Workflow

### User Perspective

1. User opens sync dialog in LocalPDub
2. Client starts broadcasting and listening for other clients
3. Available clients appear in a list
4. User selects specific clients or "Sync All"
5. Sync proceeds with progress indication
6. Client stops broadcasting when sync completes or user cancels

## Network Protocol

### Port Range

```
Primary port: 51820
Fallback range: 51821-51829
```

Clients attempt to bind to the primary port first, then try fallback ports if occupied.

### Discovery Phase

#### 1. Activation

When user initiates sync mode:

```cpp
class SyncSession {
public:
    void start_discovery() {
        // 1. Bind to available port
        int port = bind_available_port(51820, 51829);

        // 2. Start UDP broadcast listener
        start_broadcast_listener(port);

        // 3. Start TCP sync server
        start_sync_server(port);

        // 4. Begin broadcasting presence
        broadcast_presence(port);

        // 5. Set timeout (default 5 minutes)
        set_discovery_timeout(300);
    }
};
```

#### 2. Broadcast Message

UDP broadcast on port 51820 (255.255.255.255):

```json
{
    "type": "LOCALPDUB_ANNOUNCE",
    "version": 1,
    "device": {
        "id": "device-uuid",
        "name": "John's Laptop",
        "ip": "192.168.1.100",
        "port": 51820,
        "vault_id": "vault-uuid",
        "last_modified": "2024-01-01T12:00:00Z"
    },
    "auth": {
        "challenge": "random-nonce",
        "public_key": "base64-public-key"
    }
}
```

#### 3. Discovery Response

When receiving a broadcast, respond directly via UDP:

```json
{
    "type": "LOCALPDUB_RESPONSE",
    "version": 1,
    "device": {
        "id": "device-uuid",
        "name": "John's Desktop",
        "ip": "192.168.1.101",
        "port": 51821,
        "vault_id": "vault-uuid",
        "last_modified": "2024-01-01T11:30:00Z"
    },
    "auth": {
        "response": "signed-challenge",
        "challenge": "new-random-nonce",
        "public_key": "base64-public-key"
    }
}
```

### Client Discovery UI

```
┌─────────────────────────────────────┐
│  Sync - Available Clients           │
├─────────────────────────────────────┤
│ ☑ John's Desktop  (192.168.1.101)  │
│    Last sync: 2 hours ago           │
│                                     │
│ ☑ iPad            (192.168.1.105)  │
│    Last sync: Yesterday             │
│                                     │
│ ☐ Work Laptop     (192.168.1.110)  │
│    Last sync: Never                 │
├─────────────────────────────────────┤
│ [Select All] [Sync Selected] [Cancel]│
└─────────────────────────────────────┘
```

### Synchronization Phase

#### 1. Connection Establishment

After user selects clients to sync with:

```cpp
struct SyncRequest {
    std::string device_id;
    std::string vault_id;
    std::string last_sync_timestamp;
    std::vector<EntryDigest> entry_digests;
};

void initiate_sync(const std::vector<Device>& selected_devices) {
    // Create version backup before sync
    version_manager.save_current_version("pre-sync");

    // Begin sync session for history tracking
    auto session_id = history_manager.begin_sync_session();

    for (const auto& device : selected_devices) {
        try {
            // Establish TCP connection
            auto connection = connect_tcp(device.ip, device.port);

            // Authenticate
            authenticate(connection, device.public_key);

            // Exchange vault digests
            exchange_digests(connection);

            // Perform sync
            sync_changes(connection);

            // Record success
            history_manager.record_sync_success(session_id, device);
        } catch (const std::exception& e) {
            // Record failure - rollback available
            history_manager.record_sync_failure(session_id, device, e.what());
        }
    }

    // End session
    history_manager.end_sync_session(session_id);
}
```

#### 2. Authentication

Simple challenge-response with pre-shared key:

```cpp
bool authenticate(Connection& conn, const std::string& passphrase) {
    // 1. Send challenge
    auto challenge = generate_random_bytes(32);
    conn.send(challenge);

    // 2. Receive response
    auto response = conn.receive();

    // 3. Verify HMAC
    auto expected = hmac_sha256(challenge, passphrase);
    return timing_safe_compare(response, expected);
}
```

#### 3. Data Exchange

```
Client A                     Client B
    │                           │
    ├─ Send Entry List Digest ─>│
    │  (id, modified_timestamp) │
    │                           │
    │<─ Send Entry List Digest ─┤
    │                           │
    ├─ Request Missing/Newer ──>│
    │                           │
    │<─ Request Missing/Newer ──┤
    │                           │
    ├─ Send Requested Entries ─>│
    │                           │
    │<─ Send Requested Entries ─┤
    │                           │
    ├─ Confirm & Close ────────>│
    │                           │
```

### Security

#### Session Security

1. **No Persistent Listening**: Clients only listen during active sync sessions
2. **Timeout**: Automatic shutdown after 5 minutes of inactivity
3. **Local Network Only**: No internet-facing services
4. **Encrypted Transport**: TLS 1.3 for TCP connections

#### Authentication Options

User can choose authentication method:

1. **No Authentication** (trusted network)
2. **Passphrase** (shared secret)
3. **QR Code** (includes temporary key)
4. **Device Pairing** (persistent trust)

### Implementation Details

#### Network Discovery Manager

```cpp
class NetworkDiscoveryManager {
private:
    bool is_active = false;
    std::thread broadcast_thread;
    std::thread listener_thread;
    std::vector<Device> discovered_devices;
    std::function<void(Device)> on_device_found;

public:
    void start_session(const std::string& device_name) {
        is_active = true;

        // Start broadcasting every 2 seconds
        broadcast_thread = std::thread([this]() {
            while (is_active) {
                broadcast_presence();
                std::this_thread::sleep_for(2s);
            }
        });

        // Start listening for other devices
        listener_thread = std::thread([this]() {
            listen_for_broadcasts();
        });
    }

    void stop_session() {
        is_active = false;
        close_sockets();
        broadcast_thread.join();
        listener_thread.join();
    }

    std::vector<Device> get_available_devices() {
        return discovered_devices;
    }
};
```

#### Sync Manager

```cpp
class SyncManager {
public:
    struct SyncResult {
        int entries_sent;
        int entries_received;
        int conflicts_resolved;
        std::vector<std::string> errors;
    };

    SyncResult sync_with_devices(
        const std::vector<Device>& devices,
        SyncStrategy strategy
    ) {
        SyncResult total_result;

        for (const auto& device : devices) {
            auto result = sync_with_device(device, strategy);
            merge_results(total_result, result);
        }

        return total_result;
    }

private:
    SyncResult sync_with_device(
        const Device& device,
        SyncStrategy strategy
    ) {
        // Connect
        auto conn = establish_connection(device);

        // Authenticate
        if (!authenticate(conn)) {
            return {0, 0, 0, {"Authentication failed"}};
        }

        // Exchange data
        auto local_digest = compute_vault_digest();
        auto remote_digest = exchange_digest(conn, local_digest);

        // Determine changes
        auto to_send = find_newer_entries(local_digest, remote_digest);
        auto to_receive = find_newer_entries(remote_digest, local_digest);

        // Transfer entries
        send_entries(conn, to_send);
        auto received = receive_entries(conn);

        // Apply changes
        auto conflicts = apply_changes(received, strategy);

        return {
            to_send.size(),
            received.size(),
            conflicts.size(),
            {}
        };
    }
};
```

### Sync Strategies

User-selectable conflict resolution:

```cpp
enum class SyncStrategy {
    LOCAL_WINS,      // Keep local version for conflicts
    REMOTE_WINS,     // Accept remote version for conflicts
    NEWEST_WINS,     // Use timestamp (default)
    MANUAL,          // Prompt user for each conflict
    DUPLICATE        // Keep both versions
};
```

### User Interface Flow

```
1. User Menu: Tools → Sync with Other Devices

2. Discovery Screen:
   "Searching for devices..."
   [Stop Searching]

3. Device List:
   "Found 3 devices"
   [List of devices with checkboxes]
   [Sync Settings...]
   [Start Sync] [Cancel]

4. Sync Progress:
   "Syncing with John's Desktop..."
   [Progress bar]
   "Sent: 5 entries"
   "Received: 12 entries"
   "Conflicts: 2 resolved"

5. Completion:
   "Sync completed successfully"
   [View Changes] [Done]
```

### Performance Considerations

#### Broadcast Frequency
- Every 2 seconds during discovery
- Stop after 5 minutes or when user cancels

#### Connection Limits
- Maximum 10 simultaneous sync connections
- 30-second timeout per sync operation
- Chunk large vaults (>1000 entries) into batches

#### Network Efficiency
- Only exchange changed entries (diff-based)
- Compress data if >10KB
- Use binary protocol for large transfers

### Error Handling

```cpp
enum class SyncError {
    NETWORK_UNAVAILABLE,
    DEVICE_UNREACHABLE,
    AUTHENTICATION_FAILED,
    VERSION_MISMATCH,
    VAULT_CORRUPTED,
    TIMEOUT,
    USER_CANCELLED
};

void handle_sync_error(SyncError error, const Device& device) {
    switch (error) {
        case SyncError::DEVICE_UNREACHABLE:
            // Retry with different port
            break;
        case SyncError::AUTHENTICATION_FAILED:
            // Prompt for passphrase
            break;
        case SyncError::VERSION_MISMATCH:
            // Suggest update
            break;
        default:
            // Show error to user
            break;
    }
}
```

### Platform-Specific Considerations

#### Windows
- Windows Firewall prompt on first use
- Use WinSock2 for networking

#### macOS
- Request local network permission (iOS 14+)
- Bonjour/mDNS as alternative discovery

#### Linux
- May require firewall configuration
- Support both iptables and ufw

#### Mobile
- iOS: Local Network permission required
- Android: No special permissions for LAN

### Testing

#### Test Scenarios
1. Discovery with 0, 1, 5+ devices
2. Sync with conflicting changes
3. Network interruption during sync
4. Large vault transfer (>10MB)
5. Different OS combinations
6. Firewall blocking scenarios