#include "sync/sync_manager.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

namespace localpdub {
namespace sync {

SyncManager::SyncManager(const std::string& vault_path)
    : vault_path_(vault_path)
    , server_running_(false)
    , server_socket_(-1) {
}

SyncManager::~SyncManager() {
    stop_sync_server();
}

bool SyncManager::start_sync_server(int port) {
    if (server_running_) {
        return false;
    }

    // Create TCP socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        return false;
    }

    // Allow socket reuse
    int reuse = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        close(server_socket_);
        server_socket_ = -1;
        return false;
    }

    // Bind to port
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(server_socket_);
        server_socket_ = -1;
        return false;
    }

    // Start listening
    if (listen(server_socket_, MAX_SIMULTANEOUS_CONNECTIONS) < 0) {
        close(server_socket_);
        server_socket_ = -1;
        return false;
    }

    server_running_ = true;

    // Start server thread
    server_thread_ = std::make_unique<std::thread>([this]() {
        accept_clients();
    });

    return true;
}

void SyncManager::stop_sync_server() {
    if (!server_running_) {
        return;
    }

    server_running_ = false;

    if (server_socket_ >= 0) {
        shutdown(server_socket_, SHUT_RDWR);
        close(server_socket_);
        server_socket_ = -1;
    }

    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }
}

void SyncManager::accept_clients() {
    while (server_running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        // Set accept timeout
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        setsockopt(server_socket_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);

        if (client_socket >= 0) {
            // Handle client in separate thread
            std::thread client_thread([this, client_socket]() {
                handle_sync_client(client_socket);
                close(client_socket);
            });
            client_thread.detach();
        }
    }
}

void SyncManager::handle_sync_client(int client_socket) {
    // Set socket timeout
    struct timeval tv;
    tv.tv_sec = SOCKET_TIMEOUT_SECONDS;
    tv.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    try {
        // Receive sync request
        char buffer[4096];
        int received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            return;
        }
        buffer[received] = '\0';

        json request = json::parse(buffer);

        // Authenticate if required
        if (!passphrase_.empty()) {
            if (!authenticate_connection(client_socket, AuthMethod::PASSPHRASE, passphrase_)) {
                return;
            }
        }

        // Compute local vault digest
        auto local_digest = compute_vault_digest();

        // Send local digest
        json digest_msg = {
            {"type", "DIGEST"},
            {"entries", json::array()}
        };

        for (const auto& entry : local_digest) {
            auto time_t = std::chrono::system_clock::to_time_t(entry.modified);
            digest_msg["entries"].push_back({
                {"id", entry.id},
                {"modified", time_t},
                {"hash", entry.hash}
            });
        }

        std::string digest_str = digest_msg.dump();
        send(client_socket, digest_str.c_str(), digest_str.length(), 0);

        // Receive remote digest
        received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            return;
        }
        buffer[received] = '\0';

        json remote_digest_msg = json::parse(buffer);
        std::vector<EntryDigest> remote_digest;

        for (const auto& entry : remote_digest_msg["entries"]) {
            EntryDigest digest;
            digest.id = entry["id"];
            digest.modified = std::chrono::system_clock::from_time_t(entry["modified"]);
            digest.hash = entry["hash"];
            remote_digest.push_back(digest);
        }

        // Determine what to send
        auto entries_to_send = find_entries_to_send(local_digest, remote_digest);

        // Send entries
        json entries_msg = {
            {"type", "ENTRIES"},
            {"entries", entries_to_send}
        };

        std::string entries_str = entries_msg.dump();
        send(client_socket, entries_str.c_str(), entries_str.length(), 0);

        // Receive entries from client
        received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (received > 0) {
            buffer[received] = '\0';
            json received_entries_msg = json::parse(buffer);

            if (received_entries_msg["type"] == "ENTRIES") {
                // Apply received entries
                apply_changes(received_entries_msg["entries"], SyncStrategy::NEWEST_WINS);
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error handling sync client: " << e.what() << std::endl;
    }
}

SyncResult SyncManager::sync_with_devices(
    const std::vector<Device>& devices,
    SyncStrategy strategy,
    AuthMethod auth_method,
    const std::string& passphrase) {

    SyncResult total_result;
    total_result.success = true;

    for (const auto& device : devices) {
        try {
            // Connect to device
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                total_result.errors.push_back("Failed to create socket for " + device.name);
                continue;
            }

            // Set socket timeout
            struct timeval tv;
            tv.tv_sec = SOCKET_TIMEOUT_SECONDS;
            tv.tv_usec = 0;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

            // Connect
            struct sockaddr_in addr;
            std::memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = inet_addr(device.ip_address.c_str());
            addr.sin_port = htons(device.port);

            if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                close(sock);
                total_result.errors.push_back("Failed to connect to " + device.name);
                continue;
            }

            // Send sync request
            json request = {
                {"type", "SYNC_REQUEST"},
                {"device_id", device.id},
                {"vault_id", vault_path_}
            };
            std::string request_str = request.dump();
            send(sock, request_str.c_str(), request_str.length(), 0);

            // Authenticate
            if (auth_method != AuthMethod::NONE) {
                if (!authenticate_connection(sock, auth_method, passphrase)) {
                    close(sock);
                    total_result.errors.push_back("Authentication failed for " + device.name);
                    continue;
                }
            }

            // Exchange digests
            auto local_digest = compute_vault_digest();

            // Send our digest
            json digest_msg = {
                {"type", "DIGEST"},
                {"entries", json::array()}
            };

            for (const auto& entry : local_digest) {
                auto time_t = std::chrono::system_clock::to_time_t(entry.modified);
                digest_msg["entries"].push_back({
                    {"id", entry.id},
                    {"modified", time_t},
                    {"hash", entry.hash}
                });
            }

            std::string digest_str = digest_msg.dump();
            send(sock, digest_str.c_str(), digest_str.length(), 0);

            // Receive remote digest
            char buffer[4096];
            int received = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (received <= 0) {
                close(sock);
                total_result.errors.push_back("Failed to receive digest from " + device.name);
                continue;
            }
            buffer[received] = '\0';

            json remote_digest_msg = json::parse(buffer);
            std::vector<EntryDigest> remote_digest;

            for (const auto& entry : remote_digest_msg["entries"]) {
                EntryDigest digest;
                digest.id = entry["id"];
                digest.modified = std::chrono::system_clock::from_time_t(entry["modified"]);
                digest.hash = entry["hash"];
                remote_digest.push_back(digest);
            }

            // Send our entries
            auto entries_to_send = find_entries_to_send(local_digest, remote_digest);
            if (!entries_to_send.empty()) {
                if (send_entries(sock, entries_to_send)) {
                    total_result.entries_sent += entries_to_send.size();
                }
            }

            // Receive their entries
            auto received_entries = receive_entries(sock);
            if (!received_entries.empty()) {
                auto conflicts = apply_changes(received_entries, strategy);
                total_result.entries_received += received_entries.size();
                total_result.conflicts_resolved += conflicts.size();
            }

            close(sock);

        } catch (const std::exception& e) {
            total_result.errors.push_back("Error syncing with " + device.name + ": " + e.what());
            total_result.success = false;
        }
    }

    // Save sync result to history
    {
        std::lock_guard<std::mutex> lock(history_mutex_);
        sync_history_.push_back(total_result);
    }

    return total_result;
}

bool SyncManager::authenticate_connection(int socket, AuthMethod method, const std::string& passphrase) {
    if (method == AuthMethod::NONE) {
        return true;
    }

    if (method == AuthMethod::PASSPHRASE) {
        // Send challenge
        unsigned char challenge[32];
        RAND_bytes(challenge, sizeof(challenge));
        send(socket, challenge, sizeof(challenge), 0);

        // Receive response
        unsigned char response[32];
        int received = recv(socket, response, sizeof(response), 0);
        if (received != sizeof(response)) {
            return false;
        }

        // Compute expected HMAC
        unsigned char expected[32];
        unsigned int expected_len;
        HMAC(EVP_sha256(), passphrase.c_str(), passphrase.length(),
             challenge, sizeof(challenge), expected, &expected_len);

        // Compare
        return CRYPTO_memcmp(response, expected, sizeof(expected)) == 0;
    }

    // Other auth methods not implemented yet
    return false;
}

std::vector<EntryDigest> SyncManager::compute_vault_digest() {
    std::vector<EntryDigest> digest;

    try {
        // Read vault file
        std::ifstream file(vault_path_, std::ios::binary);
        if (!file) {
            return digest;
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        json vault_data = json::parse(content);

        for (const auto& entry : vault_data["entries"]) {
            EntryDigest ed;
            ed.id = entry["id"];

            // Parse timestamp
            if (entry.contains("modified")) {
                ed.modified = std::chrono::system_clock::from_time_t(entry["modified"]);
            } else {
                ed.modified = std::chrono::system_clock::now();
            }

            // Compute hash
            std::string entry_str = entry.dump();
            unsigned char hash[SHA256_DIGEST_LENGTH];
            SHA256((unsigned char*)entry_str.c_str(), entry_str.length(), hash);

            std::stringstream ss;
            for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
                ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
            }
            ed.hash = ss.str();

            digest.push_back(ed);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error computing vault digest: " << e.what() << std::endl;
    }

    return digest;
}

std::vector<json> SyncManager::find_entries_to_send(
    const std::vector<EntryDigest>& local,
    const std::vector<EntryDigest>& remote) {

    std::vector<json> entries_to_send;

    try {
        // Read vault file
        std::ifstream file(vault_path_, std::ios::binary);
        if (!file) {
            return entries_to_send;
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        json vault_data = json::parse(content);

        for (const auto& local_entry : local) {
            // Check if entry exists in remote
            auto remote_it = std::find_if(remote.begin(), remote.end(),
                [&local_entry](const EntryDigest& rd) { return rd.id == local_entry.id; });

            bool should_send = false;

            if (remote_it == remote.end()) {
                // Entry doesn't exist remotely
                should_send = true;
            } else if (local_entry.hash != remote_it->hash) {
                // Entry exists but is different
                if (local_entry.modified > remote_it->modified) {
                    should_send = true;
                }
            }

            if (should_send) {
                // Find the full entry in vault data
                for (const auto& entry : vault_data["entries"]) {
                    if (entry["id"] == local_entry.id) {
                        entries_to_send.push_back(entry);
                        break;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error finding entries to send: " << e.what() << std::endl;
    }

    return entries_to_send;
}

bool SyncManager::send_entries(int socket, const std::vector<json>& entries) {
    try {
        json msg = {
            {"type", "ENTRIES"},
            {"entries", entries}
        };

        std::string msg_str = msg.dump();
        size_t total_sent = 0;

        while (total_sent < msg_str.length()) {
            int sent = send(socket, msg_str.c_str() + total_sent,
                           msg_str.length() - total_sent, 0);
            if (sent <= 0) {
                return false;
            }
            total_sent += sent;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error sending entries: " << e.what() << std::endl;
        return false;
    }
}

std::vector<json> SyncManager::receive_entries(int socket) {
    std::vector<json> entries;

    try {
        char buffer[4096];
        int received = recv(socket, buffer, sizeof(buffer) - 1, 0);
        if (received > 0) {
            buffer[received] = '\0';
            json msg = json::parse(buffer);

            if (msg["type"] == "ENTRIES") {
                entries = msg["entries"].get<std::vector<json>>();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error receiving entries: " << e.what() << std::endl;
    }

    return entries;
}

std::vector<std::string> SyncManager::apply_changes(
    const std::vector<json>& entries,
    SyncStrategy strategy) {

    std::vector<std::string> conflicts;

    try {
        // Read current vault
        std::ifstream file(vault_path_, std::ios::binary);
        if (!file) {
            return conflicts;
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        json vault_data = json::parse(content);

        for (const auto& remote_entry : entries) {
            std::string entry_id = remote_entry["id"];

            // Find local entry if exists
            auto local_it = std::find_if(vault_data["entries"].begin(),
                                        vault_data["entries"].end(),
                [&entry_id](const json& e) { return e["id"] == entry_id; });

            if (local_it == vault_data["entries"].end()) {
                // New entry, add it
                vault_data["entries"].push_back(remote_entry);
            } else {
                // Existing entry, check for conflict
                json resolved = resolve_conflict(*local_it, remote_entry, strategy);
                *local_it = resolved;
                conflicts.push_back(entry_id);
            }
        }

        // Save updated vault
        std::ofstream out_file(vault_path_, std::ios::binary);
        if (out_file) {
            out_file << vault_data.dump(2);
            out_file.close();
        }

    } catch (const std::exception& e) {
        std::cerr << "Error applying changes: " << e.what() << std::endl;
    }

    return conflicts;
}

json SyncManager::resolve_conflict(
    const json& local_entry,
    const json& remote_entry,
    SyncStrategy strategy) {

    switch (strategy) {
        case SyncStrategy::LOCAL_WINS:
            return local_entry;

        case SyncStrategy::REMOTE_WINS:
            return remote_entry;

        case SyncStrategy::NEWEST_WINS:
        default:
            // Compare timestamps
            auto local_time = local_entry.contains("modified") ?
                local_entry["modified"].get<time_t>() : 0;
            auto remote_time = remote_entry.contains("modified") ?
                remote_entry["modified"].get<time_t>() : 0;

            return (local_time >= remote_time) ? local_entry : remote_entry;
    }
}

void SyncManager::set_passphrase(const std::string& passphrase) {
    passphrase_ = passphrase;
}

std::vector<SyncResult> SyncManager::get_sync_history() const {
    std::lock_guard<std::mutex> lock(history_mutex_);
    return sync_history_;
}

} // namespace sync
} // namespace localpdub