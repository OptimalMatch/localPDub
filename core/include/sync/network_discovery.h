#ifndef LOCALPDUB_NETWORK_DISCOVERY_H
#define LOCALPDUB_NETWORK_DISCOVERY_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>
#include <memory>
#include <nlohmann/json.hpp>

namespace localpdub {
namespace sync {

using json = nlohmann::json;

struct Device {
    std::string id;
    std::string name;
    std::string ip_address;
    int port;
    std::string vault_id;
    std::chrono::system_clock::time_point last_modified;
    std::string public_key;
};

class NetworkDiscoveryManager {
public:
    using DeviceFoundCallback = std::function<void(const Device&)>;

    NetworkDiscoveryManager();
    ~NetworkDiscoveryManager();

    // Start discovery session
    bool start_session(const std::string& device_name, const std::string& vault_id);

    // Stop discovery session
    void stop_session();

    // Set callback for when device is found
    void set_device_found_callback(DeviceFoundCallback callback);

    // Get list of discovered devices
    std::vector<Device> get_discovered_devices() const;

    // Check if session is active
    bool is_active() const;

    // Set discovery timeout (default 5 minutes)
    void set_timeout(std::chrono::seconds timeout);

private:
    // Network operations
    bool bind_to_port(int start_port, int end_port);
    void broadcast_presence();
    void listen_for_broadcasts();
    json create_announce_message() const;
    void handle_announce_message(const json& message, const std::string& sender_ip);

    // Threading
    std::atomic<bool> active_;
    std::unique_ptr<std::thread> broadcast_thread_;
    std::unique_ptr<std::thread> listener_thread_;

    // Network state
    int bound_port_;
    int broadcast_socket_;
    int listener_socket_;

    // Device information
    std::string device_id_;
    std::string device_name_;
    std::string vault_id_;
    std::vector<Device> discovered_devices_;
    mutable std::mutex devices_mutex_;

    // Callbacks
    DeviceFoundCallback device_found_callback_;

    // Configuration
    std::chrono::seconds timeout_;
    std::chrono::system_clock::time_point session_start_time_;

    // Constants
    static constexpr int PRIMARY_PORT = 51820;
    static constexpr int FALLBACK_START_PORT = 51821;
    static constexpr int FALLBACK_END_PORT = 51829;
    static constexpr int BROADCAST_INTERVAL_SECONDS = 2;
};

} // namespace sync
} // namespace localpdub

#endif // LOCALPDUB_NETWORK_DISCOVERY_H