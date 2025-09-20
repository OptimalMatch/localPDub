#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <cstdint>

namespace localpdub {

// Version information for rollback
struct VaultVersion {
    std::string id;
    std::string file_path;
    std::chrono::system_clock::time_point created_at;
    std::string source;      // "sync", "manual_save", "auto_backup"
    std::string device_name;
    uint32_t entry_count;
    uint64_t file_size;
    std::string hash;        // SHA-256 of file
    bool is_current;
};

// Change operation types
enum class ChangeOperation : uint8_t {
    ADD = 0,
    UPDATE = 1,
    DELETE = 2,
    SYNC = 3,
    IMPORT = 4,
    ROLLBACK = 5
};

// Field types that can change
enum class FieldType : uint8_t {
    TITLE = 0,
    USERNAME = 1,
    PASSWORD = 2,
    URL = 3,
    NOTES = 4,
    TAGS = 5,
    CUSTOM_FIELD = 6,
    CATEGORY = 7,
    FAVORITE = 8
};

// Source of change
enum class ChangeSource : uint8_t {
    LOCAL = 0,
    SYNC = 1,
    IMPORT = 2,
    ROLLBACK = 3
};

// Individual change record
struct ChangeRecord {
    uint32_t record_id;
    std::chrono::system_clock::time_point timestamp;
    ChangeOperation operation;
    std::string entry_id;
    std::string entry_title;  // For display (encrypted in storage)
    FieldType field_changed;
    std::string old_value;    // Encrypted
    std::string new_value;    // Encrypted
    ChangeSource source;
    std::string device_id;
    std::string device_name;
    std::string session_id;   // Links to sync session if applicable
};

// Sync session information
struct SyncSession {
    std::string id;
    std::chrono::system_clock::time_point started_at;
    std::chrono::system_clock::time_point completed_at;
    std::string sync_with_device;
    uint16_t entries_sent;
    uint16_t entries_received;
    uint16_t conflicts_resolved;
    bool rollback_available;
    bool success;
    std::vector<std::string> error_messages;
};

// Version manager
class VersionManager {
public:
    VersionManager();
    ~VersionManager();

    // Version operations
    void save_current_version(const std::string& source = "manual");
    bool rollback(int versions_back = 1);
    bool restore_specific_version(const std::string& version_id);
    std::vector<VaultVersion> get_versions() const;
    bool has_versions() const;

    // Cleanup
    void cleanup_old_versions(int keep_count = 2);
    uint64_t get_total_version_size() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

// History manager
class HistoryManager {
public:
    HistoryManager();
    ~HistoryManager();

    // Record changes
    void record_change(const ChangeRecord& change);
    void begin_sync_session(const std::string& device_name);
    void end_sync_session(const SyncSession& session);

    // Query history
    std::vector<ChangeRecord> get_recent_changes(int limit = 100) const;
    std::vector<ChangeRecord> get_entry_history(const std::string& entry_id) const;
    std::vector<ChangeRecord> get_changes_since(
        std::chrono::system_clock::time_point since) const;
    std::vector<SyncSession> get_sync_sessions(int limit = 20) const;
    SyncSession get_sync_session(const std::string& session_id) const;

    // Rollback
    bool can_rollback_session(const std::string& session_id) const;
    std::vector<ChangeRecord> get_session_changes(const std::string& session_id) const;

    // Maintenance
    void trim_history(int keep_records = 1000);
    void clear_history();
    uint64_t get_history_size() const;

    // Export
    bool export_to_json(const std::string& path) const;
    bool export_to_csv(const std::string& path) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

// Utility functions
std::string operation_to_string(ChangeOperation op);
std::string field_to_string(FieldType field);
std::string format_change_description(const ChangeRecord& change);

} // namespace localpdub