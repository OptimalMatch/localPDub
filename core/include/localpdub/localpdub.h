#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>

namespace localpdub {

// Forward declarations
class Vault;
class PasswordEntry;
class Category;
class SyncManager;

// Version information
constexpr int VERSION_MAJOR = 0;
constexpr int VERSION_MINOR = 1;
constexpr int VERSION_PATCH = 0;

// Main interface class
class LocalPDub {
public:
    LocalPDub();
    ~LocalPDub();

    // Vault operations
    bool create_vault(const std::string& password);
    bool open_vault(const std::string& vault_path, const std::string& password);
    bool save_vault();
    bool close_vault();
    bool is_vault_open() const;

    // Password entry operations
    std::string add_entry(const PasswordEntry& entry);
    bool update_entry(const std::string& id, const PasswordEntry& entry);
    bool delete_entry(const std::string& id);
    std::optional<PasswordEntry> get_entry(const std::string& id);
    std::vector<PasswordEntry> get_all_entries();
    std::vector<PasswordEntry> search_entries(const std::string& query);

    // Category operations
    std::string add_category(const Category& category);
    bool update_category(const std::string& id, const Category& category);
    bool delete_category(const std::string& id);
    std::vector<Category> get_all_categories();

    // Password generation
    std::string generate_password(int length = 20,
                                   bool use_uppercase = true,
                                   bool use_lowercase = true,
                                   bool use_numbers = true,
                                   bool use_symbols = true);

    // Import/Export
    bool export_vault(const std::string& path, const std::string& format = "lpd");
    bool import_vault(const std::string& path, const std::string& format);

    // Sync operations
    bool start_sync_server(int port = 51820);
    bool connect_to_sync_server(const std::string& host, int port);
    bool sync_with_file(const std::string& file_path);

    // Settings
    void set_auto_lock_minutes(int minutes);
    void set_clipboard_timeout(int seconds);
    int get_auto_lock_minutes() const;
    int get_clipboard_timeout() const;

    // Version Control & Rollback
    bool rollback_vault(int versions_back = 1);
    bool can_rollback() const;
    std::vector<VaultVersion> get_available_versions() const;
    bool restore_version(const std::string& version_id);

    // Change History
    std::vector<ChangeRecord> get_recent_changes(int limit = 100);
    std::vector<ChangeRecord> get_entry_history(const std::string& entry_id);
    std::vector<SyncSession> get_sync_history(int limit = 20);
    bool rollback_sync_session(const std::string& session_id);
    bool clear_history(bool keep_sessions = true);
    bool export_history(const std::string& path, const std::string& format = "json");

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

// Error handling
enum class ErrorCode {
    SUCCESS = 0,
    INVALID_PASSWORD,
    VAULT_NOT_FOUND,
    VAULT_CORRUPTED,
    ENTRY_NOT_FOUND,
    CATEGORY_NOT_FOUND,
    FILE_ACCESS_ERROR,
    ENCRYPTION_ERROR,
    SYNC_ERROR,
    UNKNOWN_ERROR
};

class LocalPDubException : public std::exception {
public:
    LocalPDubException(ErrorCode code, const std::string& message);
    const char* what() const noexcept override;
    ErrorCode error_code() const;

private:
    ErrorCode code_;
    std::string message_;
};

} // namespace localpdub