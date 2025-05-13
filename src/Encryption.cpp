#include "Encryption.hpp"
#include "Crypto.hpp"
#include <stdexcept>

namespace BarrenEngine {

std::vector<uint8_t> Encryption::encrypt(const std::vector<uint8_t>& data, 
                                       const std::string& key,
                                       Algorithm algorithm) {
    if (data.empty()) {
        return data;
    }

    // Generate a new IV for each encryption
    std::vector<uint8_t> iv = Crypto::generateIV();
    
    // Convert algorithm to Crypto mode
    Crypto::Mode mode;
    switch (algorithm) {
        case Algorithm::AES_256_GCM:
            mode = Crypto::Mode::GCM;
            break;
        case Algorithm::CHACHA20_POLY1305:
            mode = Crypto::Mode::GCM;
            break;
        default:
            throw std::runtime_error("Unsupported encryption algorithm");
    }

    // Convert std::string key to std::vector<uint8_t>
    std::vector<uint8_t> keyVec(key.begin(), key.end());
    std::vector<uint8_t> encryptedData = Crypto::encrypt(data, keyVec, iv, mode);
    
    // Prepend the IV to the encrypted data
    encryptedData.insert(encryptedData.begin(), iv.begin(), iv.end());
    
    return encryptedData;
}

std::vector<uint8_t> Encryption::decrypt(const std::vector<uint8_t>& encryptedData,
                                       const std::string& key,
                                       Algorithm algorithm) {
    if (encryptedData.empty()) {
        return encryptedData;
    }

    // Extract IV from the beginning of the data
    if (encryptedData.size() < Crypto::IV_SIZE) {
        throw std::runtime_error("Invalid encrypted data size");
    }

    std::vector<uint8_t> iv(encryptedData.begin(), encryptedData.begin() + Crypto::IV_SIZE);
    std::vector<uint8_t> data(encryptedData.begin() + Crypto::IV_SIZE, encryptedData.end());

    // Convert algorithm to Crypto mode
    Crypto::Mode mode;
    switch (algorithm) {
        case Algorithm::AES_256_GCM:
            mode = Crypto::Mode::GCM;
            break;
        case Algorithm::CHACHA20_POLY1305:
            mode = Crypto::Mode::GCM;
            break;
        default:
            throw std::runtime_error("Unsupported encryption algorithm");
    }

    // Convert std::string key to std::vector<uint8_t>
    std::vector<uint8_t> keyVec(key.begin(), key.end());
    return Crypto::decrypt(data, keyVec, iv, mode);
}

std::string Encryption::generateKey(Algorithm algorithm) {
    size_t keySize;
    switch (algorithm) {
        case Algorithm::AES_256_GCM:
            keySize = Crypto::KEY_SIZE_256;
            break;
        case Algorithm::CHACHA20_POLY1305:
            keySize = Crypto::KEY_SIZE_256;
            break;
        default:
            throw std::runtime_error("Unsupported encryption algorithm");
    }
    std::vector<uint8_t> keyVec = Crypto::generateKey(keySize);
    return std::string(keyVec.begin(), keyVec.end());
}

bool Encryption::validateKey(const std::string& key, Algorithm algorithm) {
    size_t expectedSize;
    switch (algorithm) {
        case Algorithm::AES_256_GCM:
            expectedSize = Crypto::KEY_SIZE_256;
            break;
        case Algorithm::CHACHA20_POLY1305:
            expectedSize = Crypto::KEY_SIZE_256;
            break;
        default:
            throw std::runtime_error("Unsupported encryption algorithm");
    }
    std::vector<uint8_t> keyVec(key.begin(), key.end());
    return Crypto::validateKey(keyVec);
}

} // namespace BarrenEngine 