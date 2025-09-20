#include <iostream>
#include <string>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <algorithm>
#include <random>
#include "../../core/src/storage/vault_storage.cpp"
#include "../../core/src/crypto/crypto.cpp"

using namespace localpdub;
using json = nlohmann::json;

class LocalPDubCLI {
private:
    storage::VaultStorage vault;
    bool running = true;

public:
    void run() {
        std::cout << "\n╔══════════════════════════════════════╗\n";
        std::cout << "║  LocalPDub Password Manager v0.1.0   ║\n";
        std::cout << "║  Secure, Local, Private              ║\n";
        std::cout << "╚══════════════════════════════════════╝\n\n";

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
            std::cout << "\n═══ Main Menu ═══\n";
            std::cout << "1. List all entries\n";
            std::cout << "2. Search entries\n";
            std::cout << "3. Add new entry\n";
            std::cout << "4. View entry details\n";
            std::cout << "5. Edit entry\n";
            std::cout << "6. Delete entry\n";
            std::cout << "7. Generate password\n";
            std::cout << "8. Save and exit\n";
            std::cout << "9. Exit without saving\n";
            std::cout << "\nChoice: ";

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
                case 8: save_and_exit(); break;
                case 9: exit_without_saving(); break;
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
            std::cout << "❌ Passwords do not match!\n";
            exit(1);
        }

        if (password.length() < 8) {
            std::cout << "❌ Password must be at least 8 characters!\n";
            exit(1);
        }

        if (vault.create_vault(password)) {
            std::cout << "✓ Vault created successfully!\n";
        } else {
            std::cout << "❌ Failed to create vault!\n";
            exit(1);
        }
    }

    void open_existing_vault() {
        std::cout << "Enter master password: ";
        std::string password = read_password();
        std::cout << "\n";

        if (vault.open_vault(password)) {
            std::cout << "✓ Vault opened successfully!\n";
        } else {
            std::cout << "❌ Invalid password or corrupted vault!\n";
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
        std::cout << "\n✓ Entry added successfully with ID: " << id << "\n";
    }

    void view_entry() {
        list_entries();
        std::cout << "\nEnter entry number to view: ";
        int index;
        std::cin >> index;
        std::cin.ignore();

        auto entries = vault.get_all_entries();
        if (index < 1 || index > entries.size()) {
            std::cout << "❌ Invalid entry number!\n";
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
            std::cout << "❌ Invalid entry number!\n";
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
            std::cout << "❌ Invalid entry number!\n";
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
                std::cout << "✓ Entry deleted successfully!\n";
            } else {
                std::cout << "❌ Failed to delete entry!\n";
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

    void save_and_exit() {
        if (vault.save_vault()) {
            std::cout << "✓ Vault saved successfully!\n";
        } else {
            std::cout << "❌ Failed to save vault!\n";
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