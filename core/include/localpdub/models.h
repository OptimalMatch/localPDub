#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <optional>

namespace localpdub {

enum class EntryType {
    PASSWORD,      // Standard login credentials
    SECURE_NOTE,   // Encrypted text note
    CREDIT_CARD,   // Credit card information
    IDENTITY,      // Personal identity info
    WIFI,          // WiFi network credentials
    SERVER,        // Server/SSH credentials
    API_KEY,       // API keys and tokens
    DATABASE,      // Database connections
    CRYPTO_WALLET  // Cryptocurrency wallet info
};

struct PasswordEntry {
    // Core fields
    std::string id;
    EntryType type = EntryType::PASSWORD;
    std::string title;        // Display name
    std::string username;     // Username or email
    std::string password;     // Encrypted password

    // Additional standard fields
    std::string email;        // Email (if different from username)
    std::string url;          // Website URL
    std::string notes;        // Free-form notes (multi-line)
    std::string totp_secret;  // For 2FA/TOTP codes

    // Organization
    std::vector<std::string> tags;
    std::string category_id;
    bool favorite = false;

    // Custom fields for flexibility
    std::map<std::string, std::string> custom_fields;
    // Common custom fields:
    // - Security questions and answers
    // - Account numbers
    // - PINs
    // - Recovery codes
    // - License keys
    // - API tokens
    // - Connection strings

    // Timestamps
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point modified_at;
    std::chrono::system_clock::time_point accessed_at;
    std::chrono::system_clock::time_point password_changed_at;
    std::chrono::system_clock::time_point expires_at;  // Optional

    // Statistics
    uint32_t access_count = 0;
    uint32_t password_strength_score = 0;  // 0-100

    // Constructors
    PasswordEntry();
    explicit PasswordEntry(const std::string& title);

    // Helper methods
    void add_custom_field(const std::string& key, const std::string& value);
    std::string get_custom_field(const std::string& key) const;
    bool has_custom_field(const std::string& key) const;

    // Serialization
    std::string to_json() const;
    static PasswordEntry from_json(const std::string& json);
};

// Specialized entry types with additional fields
struct CreditCardEntry : PasswordEntry {
    std::string card_number;
    std::string card_holder_name;
    std::string expiry_month;
    std::string expiry_year;
    std::string cvv;
    std::string billing_address;
    std::string bank_name;
    std::string pin;

    CreditCardEntry() { type = EntryType::CREDIT_CARD; }
};

struct ServerEntry : PasswordEntry {
    std::string hostname;
    std::string port;
    std::string ssh_key;
    std::string connection_type;  // SSH, FTP, RDP, VNC
    std::string root_password;    // Optional root/admin password

    ServerEntry() { type = EntryType::SERVER; }
};

struct IdentityEntry : PasswordEntry {
    std::string full_name;
    std::string document_type;    // passport, license, SSN, etc.
    std::string document_number;
    std::string issue_date;
    std::string expiry_date;
    std::string country;
    std::string date_of_birth;

    IdentityEntry() { type = EntryType::IDENTITY; }
};

struct Category {
    std::string id;
    std::string name;
    std::string icon;
    std::string color;
    uint32_t sort_order = 0;

    // Constructors
    Category();
    explicit Category(const std::string& name);

    // Serialization
    std::string to_json() const;
    static Category from_json(const std::string& json);
};

struct VaultMetadata {
    uint32_t version = 1;
    std::string vault_id;
    std::string device_id;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point modified_at;
    uint32_t entry_count = 0;
};

class Vault {
public:
    Vault();
    ~Vault();

    // Entry management
    void add_entry(const PasswordEntry& entry);
    void update_entry(const PasswordEntry& entry);
    void remove_entry(const std::string& id);
    std::optional<PasswordEntry> get_entry(const std::string& id) const;
    std::vector<PasswordEntry> get_all_entries() const;
    std::vector<PasswordEntry> search_entries(const std::string& query) const;

    // Category management
    void add_category(const Category& category);
    void update_category(const Category& category);
    void remove_category(const std::string& id);
    std::optional<Category> get_category(const std::string& id) const;
    std::vector<Category> get_all_categories() const;

    // Metadata
    const VaultMetadata& get_metadata() const;
    void update_metadata();

    // Settings
    void set_setting(const std::string& key, const std::string& value);
    std::string get_setting(const std::string& key, const std::string& default_value = "") const;

    // Serialization
    std::string to_json() const;
    static Vault from_json(const std::string& json);

    // Merge operations
    void merge_with(const Vault& other);
    std::vector<std::pair<PasswordEntry, PasswordEntry>> find_conflicts(const Vault& other) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

// Password strength calculation
enum class PasswordStrength {
    VERY_WEAK = 0,
    WEAK = 1,
    FAIR = 2,
    GOOD = 3,
    STRONG = 4,
    VERY_STRONG = 5
};

PasswordStrength calculate_password_strength(const std::string& password);

// UUID generation
std::string generate_uuid();

} // namespace localpdub