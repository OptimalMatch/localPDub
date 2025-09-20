#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace localpdub {
namespace crypto {

// Generate cryptographically secure random salt
std::vector<uint8_t> generate_salt();

// Generate random nonce for AES-GCM
std::vector<uint8_t> generate_nonce();

// Derive encryption key from password using Argon2id
std::vector<uint8_t> derive_key_from_password(const std::string& password,
                                              const std::vector<uint8_t>& salt);

// Encrypt data using AES-256-GCM
std::vector<uint8_t> encrypt_data(const std::string& plaintext,
                                 const std::vector<uint8_t>& key);

// Decrypt data using AES-256-GCM
std::string decrypt_data(const std::vector<uint8_t>& encrypted,
                        const std::vector<uint8_t>& key);

// Secure memory cleanup
template<typename T>
void secure_clear(T& container) {
    if (!container.empty()) {
        volatile auto* ptr = container.data();
        for (size_t i = 0; i < container.size(); ++i) {
            ptr[i] = 0;
        }
    }
    container.clear();
}

} // namespace crypto
} // namespace localpdub