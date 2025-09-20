#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <optional>

namespace localpdub {

// Forward declarations
class Vault;

// Device information for sync
struct SyncDevice {
    std::string id;
    std::string name;
    std::string ip_address;
    int port;
    std::chrono::system_clock::time_point last_modified;
    std::chrono::system_clock::time_point last_sync;
    bool is_trusted;
};

// Sync conflict resolution strategies
enum class SyncStrategy {
    LOCAL_WINS,      // Keep local version for conflicts
    REMOTE_WINS,     // Accept remote version for conflicts
    NEWEST_WINS,     // Use timestamp (default)
    MANUAL,          // Prompt user for each conflict
    DUPLICATE        // Keep both versions
};

// Result of a sync operation
struct SyncResult {
    bool success;
    int entries_sent;
    int entries_received;
    int conflicts_resolved;
    std::vector<std::string> errors;
    std::chrono::milliseconds duration;
};

// Sync session manager
class SyncSession {
public:
    SyncSession();
    ~SyncSession();

    // Start discovery mode (begins broadcasting)
    bool start_discovery(const std::string& device_name,
                        int timeout_seconds = 300);

    // Stop discovery mode (stops broadcasting)
    void stop_discovery();

    // Get list of discovered devices
    std::vector<SyncDevice> get_available_devices() const;

    // Sync with selected devices
    SyncResult sync_with_devices(const std::vector<SyncDevice>& devices,
                                 SyncStrategy strategy = SyncStrategy::NEWEST_WINS);

    // Sync with all available devices
    SyncResult sync_with_all(SyncStrategy strategy = SyncStrategy::NEWEST_WINS);

    // Set callbacks
    void on_device_discovered(std::function<void(const SyncDevice&)> callback);
    void on_sync_progress(std::function<void(int percent, const std::string& status)> callback);

    // Authentication
    void set_passphrase(const std::string& passphrase);
    void enable_authentication(bool require_auth);

    // Check if currently in discovery mode
    bool is_discovering() const;

    // Get sync statistics
    struct SyncStats {
        int total_syncs;
        int successful_syncs;
        int failed_syncs;
        std::chrono::system_clock::time_point last_sync_time;
    };
    SyncStats get_statistics() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

// Network utilities
namespace network {

    // Check if port is available
    bool is_port_available(int port);

    // Get local network interfaces
    std::vector<std::string> get_local_ips();

    // Test connectivity to a device
    bool can_reach_device(const std::string& ip, int port,
                         int timeout_ms = 1000);
}

// Sync-specific exceptions
class SyncException : public std::exception {
public:
    enum Type {
        NETWORK_ERROR,
        AUTHENTICATION_FAILED,
        VERSION_MISMATCH,
        TIMEOUT,
        CANCELLED
    };

    SyncException(Type type, const std::string& message);
    const char* what() const noexcept override;
    Type type() const { return type_; }

private:
    Type type_;
    std::string message_;
};

} // namespace localpdub