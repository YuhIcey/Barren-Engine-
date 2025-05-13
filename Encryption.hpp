#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <memory>

namespace BarrenEngine {

class Encryption {
public:
    enum class Algorithm {
        NONE,
        AES_256_GCM,    // Fast, secure, with authentication
        CHACHA20_POLY1305  // Very fast, secure, with authentication
    };

    static std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data, 
                                      const std::string& key,
                                      Algorithm algorithm = Algorithm::AES_256_GCM);
    
    static std::vector<uint8_t> decrypt(const std::vector<uint8_t>& encryptedData,
                                      const std::string& key,
                                      Algorithm algorithm = Algorithm::AES_256_GCM);

    // Key management
    static std::string generateKey(Algorithm algorithm = Algorithm::AES_256_GCM);
    static bool validateKey(const std::string& key, Algorithm algorithm = Algorithm::AES_256_GCM);

private:
    // Constants
    static constexpr size_t AES_256_KEY_SIZE = 32;  // 256 bits
    static constexpr size_t CHACHA20_KEY_SIZE = 32; // 256 bits
    static constexpr size_t GCM_IV_SIZE = 12;       // 96 bits
    static constexpr size_t POLY1305_TAG_SIZE = 16; // 128 bits

    // Helper functions
    static std::vector<uint8_t> generateIV();
    static std::vector<uint8_t> deriveKey(const std::string& key, Algorithm algorithm);
};

} // namespace BarrenEngine 