#include "localpdub/crypto.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <chrono>
#include <iomanip>

namespace localpdub {
namespace storage {

using json = nlohmann::json;
namespace fs = std::filesystem;

// File format constants
constexpr char MAGIC_BYTES[4] = {'L', 'P', 'D', 'V'};
constexpr uint16_t FILE_VERSION = 1;
constexpr size_t SALT_SIZE = 32;

struct FileHeader {
    char magic[4];
    uint16_t version;
    uint16_t flags;
    uint32_t header_size;
    uint32_t data_size;
};

class VaultStorage {
private:
    fs::path vault_path;
    std::vector<uint8_t> master_key;
    json vault_data;
    bool is_open = false;

public:
    VaultStorage() {
        // Default vault location
        const char* home = std::getenv("HOME");
        if (home) {
            vault_path = fs::path(home) / ".localpdub" / "vault.lpd";
        } else {
            vault_path = fs::current_path() / "vault.lpd";
        }
    }

    bool create_vault(const std::string& password) {
        // Initialize empty vault
        vault_data = {
            {"metadata", {
                {"version", FILE_VERSION},
                {"created_at", get_timestamp()},
                {"modified_at", get_timestamp()},
                {"entry_count", 0}
            }},
            {"entries", json::array()},
            {"categories", json::array()}
        };

        // Generate salt and derive key
        auto salt = crypto::generate_salt();
        master_key = crypto::derive_key_from_password(password, salt);

        // Save vault
        return save_vault(salt);
    }

    bool open_vault(const std::string& password) {
        if (!fs::exists(vault_path)) {
            return false;
        }

        std::ifstream file(vault_path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        // Read header
        FileHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));

        // Validate magic bytes
        if (std::memcmp(header.magic, MAGIC_BYTES, 4) != 0) {
            return false;
        }

        // Read salt
        std::vector<uint8_t> salt(SALT_SIZE);
        file.read(reinterpret_cast<char*>(salt.data()), SALT_SIZE);

        // Derive key from password
        master_key = crypto::derive_key_from_password(password, salt);

        // Read encrypted data
        std::vector<uint8_t> encrypted(header.data_size);
        file.read(reinterpret_cast<char*>(encrypted.data()), header.data_size);

        try {
            // Decrypt
            std::string decrypted = crypto::decrypt_data(encrypted, master_key);

            // Parse JSON
            vault_data = json::parse(decrypted);
            is_open = true;
            return true;
        } catch (const std::exception& e) {
            // Wrong password or corrupted data
            crypto::secure_clear(master_key);
            return false;
        }
    }

    bool save_vault() {
        if (!is_open) {
            return false;
        }

        // Read existing salt from file
        std::vector<uint8_t> salt(SALT_SIZE);
        if (fs::exists(vault_path)) {
            std::ifstream file(vault_path, std::ios::binary);
            file.seekg(sizeof(FileHeader));
            file.read(reinterpret_cast<char*>(salt.data()), SALT_SIZE);
        } else {
            salt = crypto::generate_salt();
        }

        return save_vault(salt);
    }

    void close_vault() {
        crypto::secure_clear(master_key);
        vault_data.clear();
        is_open = false;
    }

    // Entry management
    std::string add_entry(const json& entry) {
        if (!is_open) {
            throw std::runtime_error("Vault is not open");
        }

        // Generate UUID
        std::string id = generate_uuid();
        json new_entry = entry;
        new_entry["id"] = id;
        new_entry["created_at"] = get_timestamp();
        new_entry["modified_at"] = get_timestamp();

        vault_data["entries"].push_back(new_entry);
        vault_data["metadata"]["entry_count"] = vault_data["entries"].size();
        vault_data["metadata"]["modified_at"] = get_timestamp();

        return id;
    }

    bool update_entry(const std::string& id, const json& entry) {
        if (!is_open) {
            throw std::runtime_error("Vault is not open");
        }

        for (auto& e : vault_data["entries"]) {
            if (e["id"] == id) {
                // Preserve certain fields
                json updated = entry;
                updated["id"] = id;
                updated["created_at"] = e["created_at"];
                updated["modified_at"] = get_timestamp();
                e = updated;
                vault_data["metadata"]["modified_at"] = get_timestamp();
                return true;
            }
        }
        return false;
    }

    bool delete_entry(const std::string& id) {
        if (!is_open) {
            throw std::runtime_error("Vault is not open");
        }

        auto& entries = vault_data["entries"];
        for (auto it = entries.begin(); it != entries.end(); ++it) {
            if ((*it)["id"] == id) {
                entries.erase(it);
                vault_data["metadata"]["entry_count"] = entries.size();
                vault_data["metadata"]["modified_at"] = get_timestamp();
                return true;
            }
        }
        return false;
    }

    json get_entry(const std::string& id) const {
        if (!is_open) {
            throw std::runtime_error("Vault is not open");
        }

        for (const auto& e : vault_data["entries"]) {
            if (e["id"] == id) {
                return e;
            }
        }
        return nullptr;
    }

    json get_all_entries() const {
        if (!is_open) {
            throw std::runtime_error("Vault is not open");
        }
        return vault_data["entries"];
    }

    json search_entries(const std::string& query) const {
        if (!is_open) {
            throw std::runtime_error("Vault is not open");
        }

        json results = json::array();
        std::string lower_query = to_lower(query);

        for (const auto& entry : vault_data["entries"]) {
            std::string title = entry.value("title", "");
            std::string username = entry.value("username", "");
            std::string url = entry.value("url", "");

            if (to_lower(title).find(lower_query) != std::string::npos ||
                to_lower(username).find(lower_query) != std::string::npos ||
                to_lower(url).find(lower_query) != std::string::npos) {
                results.push_back(entry);
            }
        }

        return results;
    }

    bool is_vault_open() const {
        return is_open;
    }

    void set_vault_path(const fs::path& path) {
        vault_path = path;
    }

    std::string get_vault_path() const {
        return vault_path.string();
    }

    bool set_all_entries(const json& new_entries) {
        if (!is_open) {
            return false;
        }

        // Replace all entries with the new set
        vault_data["entries"] = new_entries;
        return true;
    }

    bool reload_entries() {
        if (!is_open) {
            return false;
        }

        // Re-read vault file to get synced changes
        try {
            std::ifstream file(vault_path, std::ios::binary);
            if (!file) {
                return false;
            }

            // Read header
            FileHeader header;
            file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));

            // Validate magic bytes
            if (std::memcmp(header.magic, MAGIC_BYTES, 4) != 0) {
                std::cerr << "Invalid vault file format" << std::endl;
                return false;
            }

            // Skip salt (we already have the master key)
            file.seekg(SALT_SIZE, std::ios::cur);

            // Read encrypted data
            std::vector<uint8_t> encrypted(header.data_size);
            file.read(reinterpret_cast<char*>(encrypted.data()), header.data_size);
            file.close();

            // Decrypt with current key
            std::string decrypted = crypto::decrypt_data(encrypted, master_key);
            vault_data = json::parse(decrypted);

            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to reload vault: " << e.what() << std::endl;
            return false;
        }
    }

private:
    bool save_vault(const std::vector<uint8_t>& salt) {
        // Update modified time
        vault_data["metadata"]["modified_at"] = get_timestamp();

        // Serialize to JSON
        std::string json_str = vault_data.dump(2);

        // Encrypt
        auto encrypted = crypto::encrypt_data(json_str, master_key);

        // Create directory if needed
        fs::create_directories(vault_path.parent_path());

        // Create backup of existing vault
        if (fs::exists(vault_path)) {
            fs::path backup = vault_path.string() + ".bak";
            fs::copy_file(vault_path, backup, fs::copy_options::overwrite_existing);
        }

        // Write to temporary file first
        fs::path temp_path = vault_path.string() + ".tmp";
        std::ofstream file(temp_path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        // Write header
        FileHeader header;
        std::memcpy(header.magic, MAGIC_BYTES, 4);
        header.version = FILE_VERSION;
        header.flags = 0;
        header.header_size = sizeof(FileHeader);
        header.data_size = encrypted.size();

        file.write(reinterpret_cast<const char*>(&header), sizeof(FileHeader));

        // Write salt
        file.write(reinterpret_cast<const char*>(salt.data()), salt.size());

        // Write encrypted data
        file.write(reinterpret_cast<const char*>(encrypted.data()), encrypted.size());

        file.close();

        // Atomic rename
        fs::rename(temp_path, vault_path);

        return true;
    }

    std::string generate_uuid() const {
        // Simple UUID v4 generation
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);

        std::stringstream ss;
        for (int i = 0; i < 36; i++) {
            if (i == 8 || i == 13 || i == 18 || i == 23) {
                ss << "-";
            } else if (i == 14) {
                ss << "4";  // Version 4
            } else {
                ss << std::hex << dis(gen);
            }
        }
        return ss.str();
    }

    std::string get_timestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
        return ss.str();
    }

    std::string to_lower(const std::string& str) const {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
};

} // namespace storage
} // namespace localpdub