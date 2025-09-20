#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <argon2.h>
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>

namespace localpdub {
namespace crypto {

// Constants
constexpr int AES_KEY_SIZE = 32;  // 256 bits
constexpr int AES_GCM_IV_SIZE = 12;
constexpr int AES_GCM_TAG_SIZE = 16;
constexpr int SALT_SIZE = 32;

// Argon2 parameters
constexpr uint32_t ARGON2_TIME_COST = 3;
constexpr uint32_t ARGON2_MEMORY_COST = 65536;  // 64 MB
constexpr uint32_t ARGON2_PARALLELISM = 4;

class CryptoImpl {
public:
    // Generate random bytes
    static std::vector<uint8_t> generate_random(size_t length) {
        std::vector<uint8_t> buffer(length);
        if (RAND_bytes(buffer.data(), length) != 1) {
            throw std::runtime_error("Failed to generate random bytes");
        }
        return buffer;
    }

    // Derive key from password using Argon2id
    static std::vector<uint8_t> derive_key(const std::string& password,
                                          const std::vector<uint8_t>& salt) {
        std::vector<uint8_t> key(AES_KEY_SIZE);

        int result = argon2id_hash_raw(
            ARGON2_TIME_COST,
            ARGON2_MEMORY_COST,
            ARGON2_PARALLELISM,
            password.c_str(),
            password.length(),
            salt.data(),
            salt.size(),
            key.data(),
            key.size()
        );

        if (result != ARGON2_OK) {
            throw std::runtime_error("Key derivation failed: " + std::string(argon2_error_message(result)));
        }

        return key;
    }

    // AES-256-GCM encryption
    static std::vector<uint8_t> encrypt_aes_gcm(const std::vector<uint8_t>& plaintext,
                                                const std::vector<uint8_t>& key,
                                                const std::vector<uint8_t>& iv) {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) throw std::runtime_error("Failed to create cipher context");

        // Initialize encryption
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to initialize encryption");
        }

        // Set IV length
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_GCM_IV_SIZE, nullptr) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to set IV length");
        }

        // Initialize with key and IV
        if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to set key and IV");
        }

        // Encrypt
        std::vector<uint8_t> ciphertext(plaintext.size() + AES_BLOCK_SIZE);
        int len;
        int ciphertext_len;

        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to encrypt data");
        }
        ciphertext_len = len;

        // Finalize
        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to finalize encryption");
        }
        ciphertext_len += len;

        // Get tag
        std::vector<uint8_t> tag(AES_GCM_TAG_SIZE);
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AES_GCM_TAG_SIZE, tag.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to get authentication tag");
        }

        EVP_CIPHER_CTX_free(ctx);

        // Combine ciphertext and tag
        ciphertext.resize(ciphertext_len);
        ciphertext.insert(ciphertext.end(), tag.begin(), tag.end());

        return ciphertext;
    }

    // AES-256-GCM decryption
    static std::vector<uint8_t> decrypt_aes_gcm(const std::vector<uint8_t>& ciphertext,
                                                const std::vector<uint8_t>& key,
                                                const std::vector<uint8_t>& iv) {
        if (ciphertext.size() < AES_GCM_TAG_SIZE) {
            throw std::runtime_error("Ciphertext too short");
        }

        // Split ciphertext and tag
        size_t actual_ciphertext_len = ciphertext.size() - AES_GCM_TAG_SIZE;
        std::vector<uint8_t> tag(ciphertext.end() - AES_GCM_TAG_SIZE, ciphertext.end());

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) throw std::runtime_error("Failed to create cipher context");

        // Initialize decryption
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to initialize decryption");
        }

        // Set IV length
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_GCM_IV_SIZE, nullptr) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to set IV length");
        }

        // Initialize with key and IV
        if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to set key and IV");
        }

        // Decrypt
        std::vector<uint8_t> plaintext(actual_ciphertext_len);
        int len;
        int plaintext_len;

        if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), actual_ciphertext_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to decrypt data");
        }
        plaintext_len = len;

        // Set tag
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, AES_GCM_TAG_SIZE,
                                const_cast<uint8_t*>(tag.data())) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to set authentication tag");
        }

        // Finalize
        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Authentication failed - data may be corrupted");
        }
        plaintext_len += len;

        EVP_CIPHER_CTX_free(ctx);

        plaintext.resize(plaintext_len);
        return plaintext;
    }

    // SHA-256 hash
    static std::vector<uint8_t> sha256(const std::vector<uint8_t>& data) {
        std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
        SHA256(data.data(), data.size(), hash.data());
        return hash;
    }

    // HMAC-SHA256
    static std::vector<uint8_t> hmac_sha256(const std::vector<uint8_t>& data,
                                           const std::vector<uint8_t>& key) {
        std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
        unsigned int len = SHA256_DIGEST_LENGTH;

        HMAC(EVP_sha256(), key.data(), key.size(),
             data.data(), data.size(), hash.data(), &len);

        return hash;
    }
};

// Public API functions
std::vector<uint8_t> generate_salt() {
    return CryptoImpl::generate_random(SALT_SIZE);
}

std::vector<uint8_t> generate_nonce() {
    return CryptoImpl::generate_random(AES_GCM_IV_SIZE);
}

std::vector<uint8_t> derive_key_from_password(const std::string& password,
                                              const std::vector<uint8_t>& salt) {
    return CryptoImpl::derive_key(password, salt);
}

std::vector<uint8_t> encrypt_data(const std::string& plaintext,
                                 const std::vector<uint8_t>& key) {
    std::vector<uint8_t> data(plaintext.begin(), plaintext.end());
    std::vector<uint8_t> nonce = generate_nonce();

    auto encrypted = CryptoImpl::encrypt_aes_gcm(data, key, nonce);

    // Prepend nonce to encrypted data
    encrypted.insert(encrypted.begin(), nonce.begin(), nonce.end());

    return encrypted;
}

std::string decrypt_data(const std::vector<uint8_t>& encrypted,
                        const std::vector<uint8_t>& key) {
    if (encrypted.size() < AES_GCM_IV_SIZE + AES_GCM_TAG_SIZE) {
        throw std::runtime_error("Invalid encrypted data");
    }

    // Extract nonce
    std::vector<uint8_t> nonce(encrypted.begin(), encrypted.begin() + AES_GCM_IV_SIZE);

    // Extract ciphertext with tag
    std::vector<uint8_t> ciphertext(encrypted.begin() + AES_GCM_IV_SIZE, encrypted.end());

    auto decrypted = CryptoImpl::decrypt_aes_gcm(ciphertext, key, nonce);

    return std::string(decrypted.begin(), decrypted.end());
}

} // namespace crypto
} // namespace localpdub