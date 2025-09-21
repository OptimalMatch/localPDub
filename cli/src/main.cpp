#include <iostream>
#include <string>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <algorithm>
#include <random>
#include <thread>
#include <chrono>
#include <sstream>
#include <mutex>
#include <atomic>
#include "../../core/include/ui/ansi_colors.h"
#include "../../core/src/storage/vault_storage.cpp"
#include "../../core/src/crypto/crypto.cpp"
#include "../../core/src/sync/network_discovery.cpp"
#include "../../core/src/sync/sync_manager.cpp"

using namespace localpdub;
using json = nlohmann::json;

class LocalPDubCLI {
private:
    storage::VaultStorage vault;
    bool running = true;

public:
    void run() {
        // Initialize ANSI colors
        ui::AnsiUI::setColorsEnabled(ui::AnsiUI::supportsColor());

        // Display colorful BBS-style welcome screen
        std::cout << ui::AnsiUI::color(ui::ansi::CLEAR_SCREEN);
        std::cout << "\n";

        // Animated title with gradient effect
        std::cout << ui::AnsiUI::color(ui::ansi::BRIGHT_CYAN);
        std::cout << ui::box::DOUBLE_TOP_LEFT;
        for (int i = 0; i < 42; ++i) std::cout << ui::box::DOUBLE_HORIZONTAL;
        std::cout << ui::box::DOUBLE_TOP_RIGHT << "\n";

        std::cout << ui::box::DOUBLE_VERTICAL;
        std::cout << ui::AnsiUI::color(ui::ansi::BRIGHT_WHITE) << ui::AnsiUI::color(ui::ansi::BOLD);
        std::cout << "  LocalPDub Password Manager v0.1.0     ";
        std::cout << ui::AnsiUI::color(ui::ansi::BRIGHT_CYAN) << ui::box::DOUBLE_VERTICAL << "\n";

        std::cout << ui::box::DOUBLE_VERTICAL;
        std::cout << ui::AnsiUI::color(ui::ansi::BRIGHT_MAGENTA);
        std::cout << "  " << ui::box::STAR << " Secure " << ui::box::STAR << " Local " << ui::box::STAR << " Private " << ui::box::STAR << "       ";
        std::cout << ui::AnsiUI::color(ui::ansi::BRIGHT_CYAN) << ui::box::DOUBLE_VERTICAL << "\n";

        std::cout << ui::box::DOUBLE_BOTTOM_LEFT;
        for (int i = 0; i < 42; ++i) std::cout << ui::box::DOUBLE_HORIZONTAL;
        std::cout << ui::box::DOUBLE_BOTTOM_RIGHT;
        std::cout << ui::AnsiUI::color(ui::ansi::RESET) << "\n\n";

        // Check if vault exists
        std::string home = std::getenv("HOME");
        std::filesystem::path vault_path = std::filesystem::path(home) / ".localpdub" / "vault.lpd";

        if (std::filesystem::exists(vault_path)) {
            open_existing_vault();
        } else {
            create_new_vault();
        }

        if (vault.is_vault_open()) {
            main_menu();
        }
    }

private:
    void main_menu() {
        while (running) {
            std::cout << "\n";
            std::cout << ui::AnsiUI::color(ui::ansi::BRIGHT_YELLOW);
            std::cout << ui::box::DOUBLE_HORIZONTAL << ui::box::DOUBLE_HORIZONTAL << ui::box::DOUBLE_HORIZONTAL;
            std::cout << " " << ui::AnsiUI::bold("Main Menu") << " ";
            std::cout << ui::box::DOUBLE_HORIZONTAL << ui::box::DOUBLE_HORIZONTAL << ui::box::DOUBLE_HORIZONTAL;
            std::cout << ui::AnsiUI::color(ui::ansi::RESET) << "\n\n";

            std::cout << ui::AnsiUI::cyan("1") << ". List all entries\n";
            std::cout << ui::AnsiUI::cyan("2") << ". Search entries\n";
            std::cout << ui::AnsiUI::cyan("3") << ". " << ui::AnsiUI::green("Add new entry") << "\n";
            std::cout << ui::AnsiUI::cyan("4") << ". View entry details\n";
            std::cout << ui::AnsiUI::cyan("5") << ". " << ui::AnsiUI::yellow("Edit entry") << "\n";
            std::cout << ui::AnsiUI::cyan("6") << ". " << ui::AnsiUI::red("Delete entry") << "\n";
            std::cout << ui::AnsiUI::cyan("7") << ". " << ui::AnsiUI::magenta("Generate password") << "\n";
            std::cout << ui::AnsiUI::cyan("8") << ". " << ui::AnsiUI::blue("Sync with other devices") << "\n";
            std::cout << ui::AnsiUI::cyan("9") << ". " << ui::AnsiUI::green("Save and exit") << "\n";
            std::cout << ui::AnsiUI::cyan("0") << ". " << ui::AnsiUI::yellow("Exit without saving") << "\n";

            std::cout << "\n" << ui::AnsiUI::color(ui::ansi::BRIGHT_WHITE);
            std::cout << ui::box::ARROW_RIGHT << " Choice: ";
            std::cout << ui::AnsiUI::color(ui::ansi::RESET);

            int choice;
            std::cin >> choice;
            std::cin.ignore();

            switch (choice) {
                case 1: list_entries(); break;
                case 2: search_entries(); break;
                case 3: add_entry(); break;
                case 4: view_entry(); break;
                case 5: edit_entry(); break;
                case 6: delete_entry(); break;
                case 7: generate_password_menu(); break;
                case 8: sync_with_devices(); break;
                case 9: save_and_exit(); break;
                case 0: exit_without_saving(); break;
                default: std::cout << "Invalid choice. Try again.\n";
            }
        }
    }

    void create_new_vault() {
        std::cout << "Creating new vault...\n";
        std::cout << "Enter master password: ";
        std::string password = read_password();
        std::cout << "\nConfirm master password: ";
        std::string confirm = read_password();
        std::cout << "\n";

        if (password != confirm) {
            std::cout << ui::AnsiUI::error("Passwords do not match!") << "\n";
            exit(1);
        }

        if (password.length() < 8) {
            std::cout << ui::AnsiUI::error("Password must be at least 8 characters!") << "\n";
            exit(1);
        }

        if (vault.create_vault(password)) {
            std::cout << ui::AnsiUI::success("Vault created successfully!") << "\n";
        } else {
            std::cout << ui::AnsiUI::error("Failed to create vault!") << "\n";
            exit(1);
        }
    }

    void open_existing_vault() {
        std::cout << "Enter master password: ";
        std::string password = read_password();
        std::cout << "\n";

        if (vault.open_vault(password)) {
            std::cout << ui::AnsiUI::success("Vault opened successfully!") << "\n";
        } else {
            std::cout << ui::AnsiUI::error("Invalid password or corrupted vault!") << "\n";
            exit(1);
        }
    }

    void list_entries() {
        auto entries = vault.get_all_entries();

        if (entries.empty()) {
            std::cout << "\nNo entries in vault.\n";
            return;
        }

        std::cout << "\n═══ Password Entries ═══\n\n";
        std::cout << std::left << std::setw(5) << "ID"
                  << std::setw(30) << "Title"
                  << std::setw(30) << "Username"
                  << std::setw(30) << "URL" << "\n";
        std::cout << std::string(95, '-') << "\n";

        int index = 1;
        for (const auto& entry : entries) {
            std::cout << std::left << std::setw(5) << index++
                      << std::setw(30) << truncate(entry.value("title", ""), 28)
                      << std::setw(30) << truncate(entry.value("username", ""), 28)
                      << std::setw(30) << truncate(entry.value("url", ""), 28) << "\n";
        }
    }

    void search_entries() {
        std::cout << "Enter search query: ";
        std::string query;
        std::getline(std::cin, query);

        auto results = vault.search_entries(query);

        if (results.empty()) {
            std::cout << "\nNo entries found.\n";
            return;
        }

        std::cout << "\n═══ Search Results ═══\n\n";
        int index = 1;
        for (const auto& entry : results) {
            std::cout << index++ << ". " << entry.value("title", "") << " - "
                      << entry.value("username", "") << "\n";
        }
    }

    void add_entry() {
        json entry;

        std::cout << "\n═══ Add New Entry ═══\n\n";

        std::cout << "Title: ";
        std::string title;
        std::getline(std::cin, title);
        entry["title"] = title;

        std::cout << "Username: ";
        std::string username;
        std::getline(std::cin, username);
        entry["username"] = username;

        std::cout << "Password (leave empty to generate): ";
        std::string password;
        std::getline(std::cin, password);
        if (password.empty()) {
            password = generate_password(20, true, true, true, true);
            std::cout << "Generated password: " << password << "\n";
        }
        entry["password"] = password;

        std::cout << "URL: ";
        std::string url;
        std::getline(std::cin, url);
        entry["url"] = url;

        std::cout << "Email (optional): ";
        std::string email;
        std::getline(std::cin, email);
        if (!email.empty()) entry["email"] = email;

        std::cout << "Notes (optional): ";
        std::string notes;
        std::getline(std::cin, notes);
        if (!notes.empty()) entry["notes"] = notes;

        // Custom fields
        std::cout << "Add custom fields? (y/n): ";
        char add_custom;
        std::cin >> add_custom;
        std::cin.ignore();

        if (add_custom == 'y' || add_custom == 'Y') {
            json custom_fields;
            while (true) {
                std::cout << "Field name (or 'done' to finish): ";
                std::string field_name;
                std::getline(std::cin, field_name);
                if (field_name == "done") break;

                std::cout << "Field value: ";
                std::string field_value;
                std::getline(std::cin, field_value);
                custom_fields[field_name] = field_value;
            }
            if (!custom_fields.empty()) {
                entry["custom_fields"] = custom_fields;
            }
        }

        entry["type"] = "password";
        entry["favorite"] = false;

        std::string id = vault.add_entry(entry);
        std::cout << "\n" << ui::AnsiUI::success("Entry added successfully with ID: " + id) << "\n";
    }

    void view_entry() {
        list_entries();
        std::cout << "\nEnter entry number to view: ";
        int index;
        std::cin >> index;
        std::cin.ignore();

        auto entries = vault.get_all_entries();
        if (index < 1 || index > entries.size()) {
            std::cout << ui::AnsiUI::error("Invalid entry number!") << "\n";
            return;
        }

        auto entry = entries[index - 1];
        std::cout << "\n═══ Entry Details ═══\n\n";
        std::cout << "Title:    " << entry.value("title", "") << "\n";
        std::cout << "Username: " << entry.value("username", "") << "\n";
        std::cout << "Password: " << mask_password(entry.value("password", "")) << " ";
        std::cout << "[Press 'r' to reveal]: ";

        char reveal;
        std::cin >> reveal;
        if (reveal == 'r' || reveal == 'R') {
            std::cout << "Password: " << entry.value("password", "") << "\n";
        }
        std::cin.ignore();

        std::cout << "URL:      " << entry.value("url", "") << "\n";
        if (entry.contains("email")) {
            std::cout << "Email:    " << entry.value("email", "") << "\n";
        }
        if (entry.contains("notes")) {
            std::cout << "Notes:    " << entry.value("notes", "") << "\n";
        }

        if (entry.contains("custom_fields")) {
            std::cout << "\nCustom Fields:\n";
            for (auto& [key, value] : entry["custom_fields"].items()) {
                std::cout << "  " << key << ": " << value << "\n";
            }
        }

        std::cout << "\nCreated:  " << entry.value("created_at", "") << "\n";
        std::cout << "Modified: " << entry.value("modified_at", "") << "\n";
    }

    void edit_entry() {
        list_entries();
        std::cout << "\nEnter entry number to edit: ";
        int index;
        std::cin >> index;
        std::cin.ignore();

        auto entries = vault.get_all_entries();
        if (index < 1 || index > entries.size()) {
            std::cout << ui::AnsiUI::error("Invalid entry number!") << "\n";
            return;
        }

        auto entry = entries[index - 1];
        std::string id = entry["id"];

        std::cout << "\n═══ Edit Entry ═══\n";
        std::cout << "Leave empty to keep current value\n\n";

        std::cout << "Title [" << entry.value("title", "") << "]: ";
        std::string title;
        std::getline(std::cin, title);
        if (!title.empty()) entry["title"] = title;

        std::cout << "Username [" << entry.value("username", "") << "]: ";
        std::string username;
        std::getline(std::cin, username);
        if (!username.empty()) entry["username"] = username;

        std::cout << "Password [" << mask_password(entry.value("password", "")) << "]: ";
        std::string password;
        std::getline(std::cin, password);
        if (!password.empty()) entry["password"] = password;

        std::cout << "URL [" << entry.value("url", "") << "]: ";
        std::string url;
        std::getline(std::cin, url);
        if (!url.empty()) entry["url"] = url;

        if (vault.update_entry(id, entry)) {
            std::cout << "\n✓ Entry updated successfully!\n";
        } else {
            std::cout << "\n❌ Failed to update entry!\n";
        }
    }

    void delete_entry() {
        list_entries();
        std::cout << "\nEnter entry number to delete: ";
        int index;
        std::cin >> index;
        std::cin.ignore();

        auto entries = vault.get_all_entries();
        if (index < 1 || index > entries.size()) {
            std::cout << ui::AnsiUI::error("Invalid entry number!") << "\n";
            return;
        }

        auto entry = entries[index - 1];
        std::string id = entry["id"];

        std::cout << "Are you sure you want to delete '" << entry.value("title", "")
                  << "'? (y/n): ";
        char confirm;
        std::cin >> confirm;
        std::cin.ignore();

        if (confirm == 'y' || confirm == 'Y') {
            if (vault.delete_entry(id)) {
                std::cout << ui::AnsiUI::success("Entry deleted successfully!") << "\n";
            } else {
                std::cout << ui::AnsiUI::error("Failed to delete entry!") << "\n";
            }
        }
    }

    void generate_password_menu() {
        std::cout << "\n═══ Password Generator ═══\n\n";
        std::cout << "Length (8-128): ";
        int length;
        std::cin >> length;

        if (length < 8) length = 8;
        if (length > 128) length = 128;

        std::cout << "Include uppercase? (y/n): ";
        char upper;
        std::cin >> upper;

        std::cout << "Include lowercase? (y/n): ";
        char lower;
        std::cin >> lower;

        std::cout << "Include numbers? (y/n): ";
        char numbers;
        std::cin >> numbers;

        std::cout << "Include symbols? (y/n): ";
        char symbols;
        std::cin >> symbols;

        std::cin.ignore();

        std::string password = generate_password(
            length,
            upper == 'y' || upper == 'Y',
            lower == 'y' || lower == 'Y',
            numbers == 'y' || numbers == 'Y',
            symbols == 'y' || symbols == 'Y'
        );

        std::cout << "\nGenerated password: " << password << "\n";
        std::cout << "Copy this password? It will be cleared from screen. (y/n): ";
        char copy;
        std::cin >> copy;
        std::cin.ignore();

        if (copy == 'y' || copy == 'Y') {
            // Clear screen
            std::cout << "\033[2J\033[1;1H";
            std::cout << "Password copied to memory. Please paste it where needed.\n";
        }
    }

    std::string generate_password(int length, bool upper, bool lower,
                                 bool numbers, bool symbols) {
        std::string charset;
        if (upper) charset += "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        if (lower) charset += "abcdefghijklmnopqrstuvwxyz";
        if (numbers) charset += "0123456789";
        if (symbols) charset += "!@#$%^&*()_+-=[]{}|;:,.<>?";

        if (charset.empty()) charset = "abcdefghijklmnopqrstuvwxyz";

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, charset.size() - 1);

        std::string password;
        for (int i = 0; i < length; i++) {
            password += charset[dis(gen)];
        }

        return password;
    }

    void sync_with_devices() {
        std::cout << "\n═══ Sync with Other Devices ═══\n\n";

        // Get device name
        char hostname[256];
        gethostname(hostname, sizeof(hostname));
        std::string device_name(hostname);

        // Get vault ID (use vault path as ID for now)
        std::string home = std::getenv("HOME");
        std::filesystem::path vault_path = std::filesystem::path(home) / ".localpdub" / "vault.lpd";
        std::string vault_id = vault_path.string();

        // Start discovery
        std::cout << "Starting device discovery...\n";
        std::cout << "This device will be visible to other LocalPDub clients on the network.\n\n";

        sync::NetworkDiscoveryManager discovery;
        discovery.set_timeout(std::chrono::seconds(60)); // 1 minute timeout

        if (!discovery.start_session(device_name, vault_id)) {
            std::cout << "❌ Failed to start discovery session.\n";
            std::cout << "Make sure no other instance is running and ports 51820-51829 are available.\n";
            return;
        }

        std::cout << "Searching for devices (60 seconds timeout)...\n";
        std::cout << "Press Enter to stop searching and view found devices.\n\n";

        // Wait for user input or timeout
        std::atomic<bool> stop_requested(false);
        std::thread input_thread([&stop_requested]() {
            std::cin.get();
            stop_requested = true;
        });

        // Show discovered devices as they appear
        int last_count = 0;
        auto start_time = std::chrono::steady_clock::now();
        while (!stop_requested && discovery.is_active()) {
            auto devices = discovery.get_discovered_devices();
            if (devices.size() > last_count) {
                std::cout << "✓ Found device: " << devices.back().name
                         << " (" << devices.back().ip_address << ")\n";
                last_count = devices.size();
            }

            // Check for 60 second timeout
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed > std::chrono::seconds(60)) {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        discovery.stop_session();

        if (input_thread.joinable()) {
            input_thread.join();
        }

        auto devices = discovery.get_discovered_devices();

        if (devices.empty()) {
            std::cout << "\nNo devices found.\n";
            std::cout << "Make sure other devices are running sync mode.\n";
            return;
        }

        // Display found devices
        std::cout << "\n═══ Available Devices ═══\n\n";
        for (size_t i = 0; i < devices.size(); ++i) {
            std::cout << i + 1 << ". " << devices[i].name
                     << " (" << devices[i].ip_address << ":" << devices[i].port << ")\n";

            auto time_t = std::chrono::system_clock::to_time_t(devices[i].last_modified);
            std::cout << "   Last modified: " << std::ctime(&time_t);
        }

        // Select devices to sync with
        std::cout << "\nEnter device numbers to sync with (comma-separated) or 'all': ";
        std::string selection;
        std::getline(std::cin, selection);

        std::vector<sync::Device> selected_devices;

        if (selection == "all") {
            selected_devices = devices;
        } else {
            std::istringstream iss(selection);
            std::string num;
            while (std::getline(iss, num, ',')) {
                try {
                    int index = std::stoi(num) - 1;
                    if (index >= 0 && index < devices.size()) {
                        selected_devices.push_back(devices[index]);
                    }
                } catch (...) {
                    // Invalid number, skip
                }
            }
        }

        if (selected_devices.empty()) {
            std::cout << "No devices selected.\n";
            return;
        }

        // Choose sync strategy
        std::cout << "\nConflict resolution strategy:\n";
        std::cout << "1. Newest wins (default)\n";
        std::cout << "2. Local wins\n";
        std::cout << "3. Remote wins\n";
        std::cout << "Choice (1-3): ";

        int strategy_choice;
        std::cin >> strategy_choice;
        std::cin.ignore();

        sync::SyncStrategy strategy = sync::SyncStrategy::NEWEST_WINS;
        switch (strategy_choice) {
            case 2: strategy = sync::SyncStrategy::LOCAL_WINS; break;
            case 3: strategy = sync::SyncStrategy::REMOTE_WINS; break;
            default: strategy = sync::SyncStrategy::NEWEST_WINS; break;
        }

        // Authentication
        std::cout << "\nAuthentication method:\n";
        std::cout << "1. None (trusted network)\n";
        std::cout << "2. Passphrase\n";
        std::cout << "Choice (1-2): ";

        int auth_choice;
        std::cin >> auth_choice;
        std::cin.ignore();

        sync::AuthMethod auth_method = sync::AuthMethod::NONE;
        std::string passphrase;

        if (auth_choice == 2) {
            auth_method = sync::AuthMethod::PASSPHRASE;
            std::cout << "Enter sync passphrase: ";
            passphrase = read_password();
            std::cout << "\n";
        }

        // Start sync server
        sync::SyncManager sync_manager(vault_path.string());
        sync_manager.set_passphrase(passphrase);

        if (!sync_manager.start_sync_server(51820)) {
            std::cout << "❌ Failed to start sync server.\n";
            return;
        }

        // Perform sync
        std::cout << "\n═══ Syncing ═══\n\n";

        auto result = sync_manager.sync_with_devices(selected_devices, strategy, auth_method, passphrase);

        // Stop server
        sync_manager.stop_sync_server();

        // Display results
        std::cout << "\n═══ Sync Results ═══\n\n";

        if (result.success) {
            std::cout << "✓ Sync completed successfully!\n";
        } else {
            std::cout << "⚠ Sync completed with errors.\n";
        }

        std::cout << "  Entries sent: " << result.entries_sent << "\n";
        std::cout << "  Entries received: " << result.entries_received << "\n";
        std::cout << "  Conflicts resolved: " << result.conflicts_resolved << "\n";

        if (!result.errors.empty()) {
            std::cout << "\nErrors:\n";
            for (const auto& error : result.errors) {
                std::cout << "  • " << error << "\n";
            }
        }

        // Reload vault to show synced entries
        if (result.entries_received > 0) {
            std::cout << "\nReloading vault to show synced entries...\n";
            vault.reload_entries();
        }
    }

    void save_and_exit() {
        if (vault.save_vault()) {
            std::cout << ui::AnsiUI::success("Vault saved successfully!") << "\n";
        } else {
            std::cout << ui::AnsiUI::error("Failed to save vault!") << "\n";
        }
        vault.close_vault();
        running = false;
    }

    void exit_without_saving() {
        std::cout << "Are you sure you want to exit without saving? (y/n): ";
        char confirm;
        std::cin >> confirm;
        if (confirm == 'y' || confirm == 'Y') {
            vault.close_vault();
            running = false;
        }
    }

    std::string read_password() {
        termios oldt;
        tcgetattr(STDIN_FILENO, &oldt);
        termios newt = oldt;
        newt.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        std::string password;
        std::getline(std::cin, password);

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return password;
    }

    std::string mask_password(const std::string& password) {
        if (password.length() <= 4) {
            return std::string(password.length(), '*');
        }
        return password.substr(0, 2) + std::string(password.length() - 4, '*') +
               password.substr(password.length() - 2);
    }

    std::string truncate(const std::string& str, size_t width) {
        if (str.length() > width) {
            return str.substr(0, width - 3) + "...";
        }
        return str;
    }
};

int main(int argc, char* argv[]) {
    try {
        LocalPDubCLI cli;
        cli.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}