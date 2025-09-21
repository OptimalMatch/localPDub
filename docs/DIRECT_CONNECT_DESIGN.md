# Direct Connect Feature Design

## Overview
Add ability to connect directly to a LocalPDub instance by IP address, bypassing UDP discovery.

## Use Cases
1. **WSL2 NAT Issues** - Connect across NAT boundaries
2. **Firewall Restrictions** - When UDP broadcast is blocked
3. **Remote Sync** - Connect over internet with port forwarding
4. **Known Hosts** - Faster connection to frequently used devices

## Implementation Plan

### 1. CLI Arguments
```cpp
// In main.cpp
struct CLIArgs {
    std::string sync_host;  // Direct IP:port
    bool auto_sync = false;  // Auto-sync on startup
};

CLIArgs parse_args(int argc, char* argv[]) {
    CLIArgs args;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--sync-host" && i + 1 < argc) {
            args.sync_host = argv[++i];
        } else if (arg == "--auto-sync") {
            args.auto_sync = true;
        }
    }
    return args;
}
```

### 2. Menu Option
```cpp
void LocalPDubCLI::direct_sync() {
    std::cout << "\n" << ui::AnsiUI::color(ui::ansi::BRIGHT_CYAN);
    std::cout << "Direct Sync Connection\n";
    std::cout << ui::AnsiUI::color(ui::ansi::RESET);

    std::cout << "Enter IP address or hostname: ";
    std::string host;
    std::getline(std::cin, host);

    std::cout << "Enter port [51820]: ";
    std::string port_str;
    std::getline(std::cin, port_str);
    int port = port_str.empty() ? 51820 : std::stoi(port_str);

    // Create a fake Device entry for direct connection
    sync::Device direct_device;
    direct_device.name = "Direct-" + host;
    direct_device.ip = host;
    direct_device.port = port;

    // Use existing sync mechanism
    sync_with_device(direct_device);
}
```

### 3. Saved Hosts File
```json
// ~/.localpdub/known_hosts.json
{
  "hosts": [
    {
      "name": "Home Desktop",
      "address": "192.168.5.51",
      "port": 51820,
      "last_sync": "2025-09-20T22:45:00Z",
      "fingerprint": "sha256:..."
    },
    {
      "name": "Work Laptop",
      "address": "work.example.com",
      "port": 51820,
      "last_sync": "2025-09-19T10:30:00Z"
    }
  ]
}
```

### 4. Menu Integration
```
[3] Sync with devices
    → [1] Discover on network
    → [2] Connect to saved host
    → [3] Enter IP manually
    → [4] Manage saved hosts
```

### 5. Connection Method in SyncManager
```cpp
// In sync_manager.cpp
bool SyncManager::connect_direct(const std::string& host, int port) {
    // Resolve hostname if needed
    struct hostent* server = gethostbyname(host.c_str());
    if (!server) {
        // Try as IP address
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
            return false;
        }
    }

    // Create socket and connect
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        return false;
    }

    // Continue with normal sync protocol
    return perform_sync(sock);
}
```

## Security Considerations

1. **Fingerprint Verification** - Store and verify device fingerprints
2. **Certificate Pinning** - Optional TLS for internet connections
3. **Passphrase Required** - Never allow unauthenticated direct connections
4. **Rate Limiting** - Prevent brute force attempts

## Configuration Options
```ini
# ~/.localpdub/config.ini
[sync]
allow_direct_connect = true
require_passphrase = true
save_known_hosts = true
auto_trust_local = true  # Auto-trust 192.168.x.x and 10.x.x.x

[network]
sync_timeout = 30
prefer_ipv4 = true
```

## Future Enhancements

1. **Tunnel Support** - SSH tunnel integration
2. **Relay Server** - Optional relay for NAT traversal
3. **mDNS/Bonjour** - Alternative discovery mechanism
4. **QR Code** - Scan QR code with IP and auth token
5. **Bluetooth** - Sync over Bluetooth for mobile devices