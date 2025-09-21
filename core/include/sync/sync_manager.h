#ifndef LOCALPDUB_SYNC_MANAGER_H
#define LOCALPDUB_SYNC_MANAGER_H

#include "network_discovery.h"
#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace localpdub {
namespace sync {

using json = nlohmann::json;

enum class SyncStrategy {
    LOCAL_WINS,      // Keep local version for conflicts
    REMOTE_WINS,     // Accept remote version for conflicts
    NEWEST_WINS,     // Use timestamp (default)
    MANUAL,          // Prompt user for each conflict
    DUPLICATE        // Keep both versions
};

enum class AuthMethod {
    NONE,            // No authentication (trusted network)
    PASSPHRASE,      // Shared secret
    QR_CODE,         // QR code with temporary key
    DEVICE_PAIRING   // Persistent trust
};

struct SyncResult {
    int entries_sent = 0;
    int entries_received = 0;
    int conflicts_resolved = 0;
    std::vector<std::string> errors;
    bool success = false;
};

struct EntryDigest {
    std::string id;
    std::chrono::system_clock::time_point modified;
    std::string hash;
};

class SyncManager {
public:
    SyncManager(const std::string& vault_path);
    ~SyncManager();

    // Start sync server on specified port
    bool start_sync_server(int port);

    // Stop sync server
    void stop_sync_server();

    // Sync with specific devices
    SyncResult sync_with_devices(
        const std::vector<Device>& devices,
        SyncStrategy strategy,
        AuthMethod auth_method,
        const std::string& passphrase = ""
    );

    // Set authentication passphrase
    void set_passphrase(const std::string& passphrase);

    // Set vault entries (for computing digest without needing to decrypt)
    void set_vault_entries(const json& entries);

    // Get updated vault entries after sync
    json get_vault_entries() const { return vault_entries_; }

    // Get sync history
    std::vector<SyncResult> get_sync_history() const;

    // Set callback for when sync connection is received
    using ConnectionCallback = std::function<void()>;
    void set_connection_callback(ConnectionCallback callback);

private:
    // Connection management
    bool establish_connection(const Device& device);
    bool authenticate_connection(int socket, AuthMethod method, const std::string& passphrase);

    // Data exchange
    std::vector<EntryDigest> compute_vault_digest();
    std::vector<EntryDigest> exchange_digests(int socket, const std::vector<EntryDigest>& local_digest);
    std::vector<json> find_entries_to_send(const std::vector<EntryDigest>& local, const std::vector<EntryDigest>& remote);
    std::vector<json> find_entries_to_receive(const std::vector<EntryDigest>& local, const std::vector<EntryDigest>& remote);

    // Data transfer
    bool send_entries(int socket, const std::vector<json>& entries);
    std::vector<json> receive_entries(int socket);

    // Conflict resolution
    std::vector<std::string> apply_changes(const std::vector<json>& entries, SyncStrategy strategy);
    json resolve_conflict(const json& local_entry, const json& remote_entry, SyncStrategy strategy);

    // Server operations
    void handle_sync_client(int client_socket);
    void accept_clients();

    // State
    std::string vault_path_;
    std::string passphrase_;
    json vault_entries_;  // Decrypted vault entries
    std::atomic<bool> server_running_;
    int server_socket_;
    std::unique_ptr<std::thread> server_thread_;
    std::vector<SyncResult> sync_history_;
    mutable std::mutex history_mutex_;
    ConnectionCallback connection_callback_;

    // Constants
    static constexpr int SOCKET_TIMEOUT_SECONDS = 30;
    static constexpr size_t MAX_CHUNK_SIZE = 10 * 1024 * 1024; // 10MB
    static constexpr int MAX_SIMULTANEOUS_CONNECTIONS = 10;
};

} // namespace sync
} // namespace localpdub

#endif // LOCALPDUB_SYNC_MANAGER_H