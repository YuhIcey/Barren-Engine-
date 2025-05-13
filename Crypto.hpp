#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <array>

namespace BarrenEngine {

class Crypto {
public:
    // Block cipher modes
    enum class Mode {
        ECB,    // Electronic Codebook (not recommended for most uses)
        CBC,    // Cipher Block Chaining
        GCM     // Galois/Counter Mode (authenticated encryption)
    };

    // Key sizes in bits
    static constexpr size_t KEY_SIZE_128 = 128;
    static constexpr size_t KEY_SIZE_256 = 256;

    // Block size in bytes
    static constexpr size_t BLOCK_SIZE = 16;

    // GCM authentication tag size in bytes
    static constexpr size_t GCM_TAG_SIZE = 16;

    // IV size in bytes
    static constexpr size_t IV_SIZE = 12;

    // Key generation
    static std::vector<uint8_t> generateKey(size_t keySize);
    static std::vector<uint8_t> generateIV();

    // Core encryption/decryption
    static std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data,
                                      const std::vector<uint8_t>& key,
                                      const std::vector<uint8_t>& iv,
                                      Mode mode = Mode::GCM);
    
    static std::vector<uint8_t> decrypt(const std::vector<uint8_t>& encryptedData,
                                      const std::vector<uint8_t>& key,
                                      const std::vector<uint8_t>& iv,
                                      Mode mode = Mode::GCM);

    // Helper functions
    static bool validateKey(const std::vector<uint8_t>& key);
    static bool validateIV(const std::vector<uint8_t>& iv);

    // Cryptographic hash function
    static std::vector<uint8_t> hash(const std::vector<uint8_t>& data);

    // Digital signature functions
    static std::vector<uint8_t> sign(const std::vector<uint8_t>& data,
                                   const std::vector<uint8_t>& privateKey);
    static bool verify(const std::vector<uint8_t>& data,
                      const std::vector<uint8_t>& signature,
                      const std::vector<uint8_t>& publicKey);

private:
    // Core block cipher operations
    static void encryptBlock(std::array<uint8_t, BLOCK_SIZE>& block,
                           const std::array<uint8_t, BLOCK_SIZE>& key);
    static void decryptBlock(std::array<uint8_t, BLOCK_SIZE>& block,
                           const std::array<uint8_t, BLOCK_SIZE>& key);

    // Block cipher modes
    static std::vector<uint8_t> encryptECB(const std::vector<uint8_t>& data,
                                         const std::vector<uint8_t>& key);
    static std::vector<uint8_t> decryptECB(const std::vector<uint8_t>& data,
                                         const std::vector<uint8_t>& key);
    
    static std::vector<uint8_t> encryptCBC(const std::vector<uint8_t>& data,
                                         const std::vector<uint8_t>& key,
                                         const std::vector<uint8_t>& iv);
    static std::vector<uint8_t> decryptCBC(const std::vector<uint8_t>& data,
                                         const std::vector<uint8_t>& key,
                                         const std::vector<uint8_t>& iv);
    
    static std::vector<uint8_t> encryptGCM(const std::vector<uint8_t>& data,
                                         const std::vector<uint8_t>& key,
                                         const std::vector<uint8_t>& iv);
    static std::vector<uint8_t> decryptGCM(const std::vector<uint8_t>& data,
                                         const std::vector<uint8_t>& key,
                                         const std::vector<uint8_t>& iv);

    // Utility functions
    static void xorBlocks(std::array<uint8_t, BLOCK_SIZE>& dest,
                         const std::array<uint8_t, BLOCK_SIZE>& src);
    static void padBlock(std::vector<uint8_t>& data);
    static void unpadBlock(std::vector<uint8_t>& data);
    static std::array<uint8_t, BLOCK_SIZE> generateRoundKey(const std::array<uint8_t, BLOCK_SIZE>& key,
                                                          size_t round);
};

} // namespace BarrenEngine 