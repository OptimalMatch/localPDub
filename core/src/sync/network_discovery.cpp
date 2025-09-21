#include "sync/network_discovery.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <random>

namespace localpdub {
namespace sync {

NetworkDiscoveryManager::NetworkDiscoveryManager()
    : active_(false)
    , bound_port_(-1)
    , broadcast_socket_(-1)
    , listener_socket_(-1)
    , timeout_(std::chrono::seconds(300)) {

    // Generate unique device ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    std::stringstream ss;
    for (int i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            ss << "-";
        }
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    device_id_ = ss.str();
}

NetworkDiscoveryManager::~NetworkDiscoveryManager() {
    stop_session();
}

bool NetworkDiscoveryManager::start_session(const std::string& device_name, const std::string& vault_id) {
    if (active_) {
        return false; // Session already active
    }

    device_name_ = device_name;
    vault_id_ = vault_id;
    session_start_time_ = std::chrono::system_clock::now();

    // Try to bind to available port
    if (!bind_to_port(PRIMARY_PORT, FALLBACK_END_PORT)) {
        return false;
    }

    active_ = true;

    // Start broadcast thread
    broadcast_thread_ = std::make_unique<std::thread>([this]() {
        while (active_) {
            broadcast_presence();
            std::this_thread::sleep_for(std::chrono::seconds(BROADCAST_INTERVAL_SECONDS));

            // Check timeout
            auto now = std::chrono::system_clock::now();
            if (now - session_start_time_ > timeout_) {
                // Don't call stop_session from within the thread - just set active to false
                active_ = false;
                break;
            }
        }
    });

    // Start listener thread
    listener_thread_ = std::make_unique<std::thread>([this]() {
        listen_for_broadcasts();
    });

    return true;
}

void NetworkDiscoveryManager::stop_session() {
    // Set active to false to signal threads to stop
    active_ = false;

    // Close sockets
    if (broadcast_socket_ >= 0) {
        close(broadcast_socket_);
        broadcast_socket_ = -1;
    }

    if (listener_socket_ >= 0) {
        close(listener_socket_);
        listener_socket_ = -1;
    }

    // Always wait for threads to finish, even if active was already false
    if (broadcast_thread_ && broadcast_thread_->joinable()) {
        broadcast_thread_->join();
    }

    if (listener_thread_ && listener_thread_->joinable()) {
        listener_thread_->join();
    }

    // Clear discovered devices
    {
        std::lock_guard<std::mutex> lock(devices_mutex_);
        discovered_devices_.clear();
    }
}

bool NetworkDiscoveryManager::bind_to_port(int start_port, int end_port) {
    // Create UDP socket for broadcasting
    broadcast_socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (broadcast_socket_ < 0) {
        return false;
    }

    // Enable broadcast option
    int broadcast_enable = 1;
    if (setsockopt(broadcast_socket_, SOL_SOCKET, SO_BROADCAST,
                   &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        close(broadcast_socket_);
        broadcast_socket_ = -1;
        return false;
    }

    // Create UDP socket for listening
    listener_socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (listener_socket_ < 0) {
        close(broadcast_socket_);
        broadcast_socket_ = -1;
        return false;
    }

    // Try to bind listener socket to available port
    for (int port = start_port; port <= end_port; ++port) {
        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(listener_socket_, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            bound_port_ = port;
            return true;
        }
    }

    // Failed to bind to any port
    close(broadcast_socket_);
    close(listener_socket_);
    broadcast_socket_ = -1;
    listener_socket_ = -1;
    return false;
}

void NetworkDiscoveryManager::broadcast_presence() {
    if (broadcast_socket_ < 0 || !active_) {
        return;
    }

    json message = create_announce_message();
    std::string msg_str = message.dump();

    struct sockaddr_in broadcast_addr;
    std::memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    broadcast_addr.sin_port = htons(PRIMARY_PORT);

    sendto(broadcast_socket_, msg_str.c_str(), msg_str.length(), 0,
           (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
}

void NetworkDiscoveryManager::listen_for_broadcasts() {
    if (listener_socket_ < 0) {
        return;
    }

    // Set socket timeout
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(listener_socket_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    char buffer[4096];
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    while (active_) {
        int received = recvfrom(listener_socket_, buffer, sizeof(buffer) - 1, 0,
                               (struct sockaddr*)&sender_addr, &sender_len);

        if (received > 0) {
            buffer[received] = '\0';
            std::string sender_ip = inet_ntoa(sender_addr.sin_addr);

            try {
                json message = json::parse(buffer);
                if (message["type"] == "LOCALPDUB_ANNOUNCE" ||
                    message["type"] == "LOCALPDUB_RESPONSE") {
                    handle_announce_message(message, sender_ip);
                }
            } catch (const std::exception& e) {
                // Invalid JSON, ignore
            }
        }
    }
}

json NetworkDiscoveryManager::create_announce_message() const {
    auto now = std::chrono::system_clock::now();
    auto now_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream timestamp;
    timestamp << std::put_time(std::gmtime(&now_t), "%Y-%m-%dT%H:%M:%SZ");

    json message = {
        {"type", "LOCALPDUB_ANNOUNCE"},
        {"version", 1},
        {"device", {
            {"id", device_id_},
            {"name", device_name_},
            {"port", bound_port_},
            {"vault_id", vault_id_},
            {"last_modified", timestamp.str()}
        }},
        {"auth", {
            {"challenge", "not-implemented"},
            {"public_key", "not-implemented"}
        }}
    };

    return message;
}

void NetworkDiscoveryManager::handle_announce_message(const json& message, const std::string& sender_ip) {
    try {
        const auto& device_info = message["device"];

        // Ignore our own broadcasts
        if (device_info["id"] == device_id_) {
            return;
        }

        Device device;
        device.id = device_info["id"];
        device.name = device_info["name"];
        device.ip_address = sender_ip;
        device.port = device_info["port"];
        device.vault_id = device_info["vault_id"];

        // Parse timestamp
        std::string timestamp_str = device_info["last_modified"];
        std::tm tm = {};
        std::istringstream ss(timestamp_str);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
        device.last_modified = std::chrono::system_clock::from_time_t(std::mktime(&tm));

        device.public_key = message["auth"]["public_key"];

        // Add to discovered devices
        {
            std::lock_guard<std::mutex> lock(devices_mutex_);

            // Check if device already exists
            auto it = std::find_if(discovered_devices_.begin(), discovered_devices_.end(),
                                  [&device](const Device& d) { return d.id == device.id; });

            if (it == discovered_devices_.end()) {
                discovered_devices_.push_back(device);

                // Notify callback if set
                if (device_found_callback_) {
                    device_found_callback_(device);
                }
            } else {
                // Update existing device info
                *it = device;
            }
        }

        // Send response if this was an announcement
        if (message["type"] == "LOCALPDUB_ANNOUNCE") {
            json response = create_announce_message();
            response["type"] = "LOCALPDUB_RESPONSE";
            std::string response_str = response.dump();

            struct sockaddr_in response_addr;
            std::memset(&response_addr, 0, sizeof(response_addr));
            response_addr.sin_family = AF_INET;
            response_addr.sin_addr.s_addr = inet_addr(sender_ip.c_str());
            response_addr.sin_port = htons(device.port);

            sendto(broadcast_socket_, response_str.c_str(), response_str.length(), 0,
                   (struct sockaddr*)&response_addr, sizeof(response_addr));
        }
    } catch (const std::exception& e) {
        // Error handling message, ignore
    }
}

void NetworkDiscoveryManager::set_device_found_callback(DeviceFoundCallback callback) {
    device_found_callback_ = callback;
}

std::vector<Device> NetworkDiscoveryManager::get_discovered_devices() const {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    return discovered_devices_;
}

bool NetworkDiscoveryManager::is_active() const {
    return active_;
}

void NetworkDiscoveryManager::set_timeout(std::chrono::seconds timeout) {
    timeout_ = timeout;
}

} // namespace sync
} // namespace localpdub