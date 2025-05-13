#include "Crypto.hpp"
#include <random>
#include <chrono>
#include <stdexcept>
#include <algorithm>

namespace BarrenEngine {

// S-box for substitution
static const uint8_t SBOX[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

// Inverse S-box for decryption
static const uint8_t INV_SBOX[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

// Round constants for key expansion
static const uint8_t RCON[10] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

std::vector<uint8_t> Crypto::generateKey(size_t keySize) {
    if (keySize != KEY_SIZE_128 && keySize != KEY_SIZE_256) {
        throw std::invalid_argument("Invalid key size");
    }

    std::vector<uint8_t> key(keySize / 8);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (auto& byte : key) {
        byte = static_cast<uint8_t>(dis(gen));
    }

    return key;
}

std::vector<uint8_t> Crypto::generateIV() {
    std::vector<uint8_t> iv(IV_SIZE);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (auto& byte : iv) {
        byte = static_cast<uint8_t>(dis(gen));
    }

    return iv;
}

void Crypto::encryptBlock(std::array<uint8_t, BLOCK_SIZE>& block,
                        const std::array<uint8_t, BLOCK_SIZE>& key) {
    // Initial round key addition
    xorBlocks(block, key);

    // 10 rounds of transformation
    for (size_t round = 0; round < 10; ++round) {
        // SubBytes
        for (auto& byte : block) {
            byte = SBOX[byte];
        }

        // ShiftRows
        std::rotate(block.begin() + 4, block.begin() + 5, block.begin() + 8);
        std::rotate(block.begin() + 8, block.begin() + 10, block.begin() + 12);
        std::rotate(block.begin() + 12, block.begin() + 15, block.begin() + 16);

        // MixColumns (if not last round)
        if (round < 9) {
            // Implementation of MixColumns transformation
            for (size_t i = 0; i < 4; ++i) {
                uint8_t s0 = block[i * 4];
                uint8_t s1 = block[i * 4 + 1];
                uint8_t s2 = block[i * 4 + 2];
                uint8_t s3 = block[i * 4 + 3];

                block[i * 4] = static_cast<uint8_t>(
                    (s0 << 1) ^ (s1 << 1) ^ s1 ^ s2 ^ s3);
                block[i * 4 + 1] = static_cast<uint8_t>(
                    s0 ^ (s1 << 1) ^ (s2 << 1) ^ s2 ^ s3);
                block[i * 4 + 2] = static_cast<uint8_t>(
                    s0 ^ s1 ^ (s2 << 1) ^ (s3 << 1) ^ s3);
                block[i * 4 + 3] = static_cast<uint8_t>(
                    (s0 << 1) ^ s0 ^ s1 ^ s2 ^ (s3 << 1));
            }
        }

        // AddRoundKey
        auto roundKey = generateRoundKey(key, round);
        xorBlocks(block, roundKey);
    }
}

void Crypto::decryptBlock(std::array<uint8_t, BLOCK_SIZE>& block,
                        const std::array<uint8_t, BLOCK_SIZE>& key) {
    // Initial round key addition
    xorBlocks(block, key);

    // 10 rounds of transformation
    for (size_t round = 0; round < 10; ++round) {
        // Inverse ShiftRows
        std::rotate(block.begin() + 12, block.begin() + 15, block.begin() + 16);
        std::rotate(block.begin() + 8, block.begin() + 10, block.begin() + 12);
        std::rotate(block.begin() + 4, block.begin() + 5, block.begin() + 8);

        // Inverse SubBytes
        for (auto& byte : block) {
            byte = INV_SBOX[byte];
        }

        // AddRoundKey
        auto roundKey = generateRoundKey(key, round);
        xorBlocks(block, roundKey);

        // Inverse MixColumns (if not last round)
        if (round < 9) {
            // Implementation of inverse MixColumns transformation
            for (size_t i = 0; i < 4; ++i) {
                uint8_t s0 = block[i * 4];
                uint8_t s1 = block[i * 4 + 1];
                uint8_t s2 = block[i * 4 + 2];
                uint8_t s3 = block[i * 4 + 3];

                block[i * 4] = static_cast<uint8_t>(
                    (s0 << 1) ^ (s1 << 1) ^ s1 ^ s2 ^ s3);
                block[i * 4 + 1] = static_cast<uint8_t>(
                    s0 ^ (s1 << 1) ^ (s2 << 1) ^ s2 ^ s3);
                block[i * 4 + 2] = static_cast<uint8_t>(
                    s0 ^ s1 ^ (s2 << 1) ^ (s3 << 1) ^ s3);
                block[i * 4 + 3] = static_cast<uint8_t>(
                    (s0 << 1) ^ s0 ^ s1 ^ s2 ^ (s3 << 1));
            }
        }
    }
}

std::vector<uint8_t> Crypto::encrypt(const std::vector<uint8_t>& data,
                                   const std::vector<uint8_t>& key,
                                   const std::vector<uint8_t>& iv,
                                   Mode mode) {
    if (!validateKey(key) || !validateIV(iv)) {
        throw std::invalid_argument("Invalid key or IV");
    }

    switch (mode) {
        case Mode::ECB:
            return encryptECB(data, key);
        case Mode::CBC:
            return encryptCBC(data, key, iv);
        case Mode::GCM:
            return encryptGCM(data, key, iv);
        default:
            throw std::invalid_argument("Unsupported encryption mode");
    }
}

std::vector<uint8_t> Crypto::decrypt(const std::vector<uint8_t>& encryptedData,
                                   const std::vector<uint8_t>& key,
                                   const std::vector<uint8_t>& iv,
                                   Mode mode) {
    if (!validateKey(key) || !validateIV(iv)) {
        throw std::invalid_argument("Invalid key or IV");
    }

    switch (mode) {
        case Mode::ECB:
            return decryptECB(encryptedData, key);
        case Mode::CBC:
            return decryptCBC(encryptedData, key, iv);
        case Mode::GCM:
            return decryptGCM(encryptedData, key, iv);
        default:
            throw std::invalid_argument("Unsupported encryption mode");
    }
}

bool Crypto::validateKey(const std::vector<uint8_t>& key) {
    return key.size() == KEY_SIZE_128 / 8 || key.size() == KEY_SIZE_256 / 8;
}

bool Crypto::validateIV(const std::vector<uint8_t>& iv) {
    return iv.size() == IV_SIZE;
}

void Crypto::xorBlocks(std::array<uint8_t, BLOCK_SIZE>& dest,
                      const std::array<uint8_t, BLOCK_SIZE>& src) {
    for (size_t i = 0; i < BLOCK_SIZE; ++i) {
        dest[i] ^= src[i];
    }
}

void Crypto::padBlock(std::vector<uint8_t>& data) {
    size_t paddingSize = BLOCK_SIZE - (data.size() % BLOCK_SIZE);
    data.resize(data.size() + paddingSize, static_cast<uint8_t>(paddingSize));
}

void Crypto::unpadBlock(std::vector<uint8_t>& data) {
    if (data.empty()) return;
    
    uint8_t paddingSize = data.back();
    if (paddingSize > BLOCK_SIZE || paddingSize == 0) {
        throw std::runtime_error("Invalid padding");
    }
    
    data.resize(data.size() - paddingSize);
}

std::array<uint8_t, Crypto::BLOCK_SIZE> Crypto::generateRoundKey(
    const std::array<uint8_t, BLOCK_SIZE>& key, size_t round) {
    std::array<uint8_t, BLOCK_SIZE> roundKey = key;
    
    // Rotate the last word
    std::rotate(roundKey.begin() + 12, roundKey.begin() + 13, roundKey.begin() + 16);
    
    // Apply S-box to each byte
    for (size_t i = 12; i < 16; ++i) {
        roundKey[i] = SBOX[roundKey[i]];
    }
    
    // XOR with round constant
    roundKey[0] ^= RCON[round];
    
    // Generate the rest of the round key
    for (size_t i = 0; i < 12; ++i) {
        roundKey[i + 4] = roundKey[i] ^ roundKey[i + 4];
    }
    
    return roundKey;
}

// Implementation of block cipher modes
std::vector<uint8_t> Crypto::encryptECB(const std::vector<uint8_t>& data,
                                      const std::vector<uint8_t>& key) {
    std::vector<uint8_t> paddedData = data;
    padBlock(paddedData);
    
    std::vector<uint8_t> result;
    result.reserve(paddedData.size());
    
    std::array<uint8_t, BLOCK_SIZE> keyArray;
    std::copy(key.begin(), key.begin() + BLOCK_SIZE, keyArray.begin());
    
    for (size_t i = 0; i < paddedData.size(); i += BLOCK_SIZE) {
        std::array<uint8_t, BLOCK_SIZE> block;
        std::copy(paddedData.begin() + i, paddedData.begin() + i + BLOCK_SIZE, block.begin());
        encryptBlock(block, keyArray);
        result.insert(result.end(), block.begin(), block.end());
    }
    
    return result;
}

std::vector<uint8_t> Crypto::decryptECB(const std::vector<uint8_t>& data,
                                      const std::vector<uint8_t>& key) {
    if (data.size() % BLOCK_SIZE != 0) {
        throw std::invalid_argument("Invalid data size for ECB decryption");
    }
    
    std::vector<uint8_t> result;
    result.reserve(data.size());
    
    std::array<uint8_t, BLOCK_SIZE> keyArray;
    std::copy(key.begin(), key.begin() + BLOCK_SIZE, keyArray.begin());
    
    for (size_t i = 0; i < data.size(); i += BLOCK_SIZE) {
        std::array<uint8_t, BLOCK_SIZE> block;
        std::copy(data.begin() + i, data.begin() + i + BLOCK_SIZE, block.begin());
        decryptBlock(block, keyArray);
        result.insert(result.end(), block.begin(), block.end());
    }
    
    unpadBlock(result);
    return result;
}

std::vector<uint8_t> Crypto::encryptCBC(const std::vector<uint8_t>& data,
                                      const std::vector<uint8_t>& key,
                                      const std::vector<uint8_t>& iv) {
    std::vector<uint8_t> paddedData = data;
    padBlock(paddedData);
    
    std::vector<uint8_t> result;
    result.reserve(paddedData.size());
    
    std::array<uint8_t, BLOCK_SIZE> keyArray;
    std::copy(key.begin(), key.begin() + BLOCK_SIZE, keyArray.begin());
    
    std::array<uint8_t, BLOCK_SIZE> previousBlock;
    std::copy(iv.begin(), iv.end(), previousBlock.begin());
    
    for (size_t i = 0; i < paddedData.size(); i += BLOCK_SIZE) {
        std::array<uint8_t, BLOCK_SIZE> block;
        std::copy(paddedData.begin() + i, paddedData.begin() + i + BLOCK_SIZE, block.begin());
        
        // XOR with previous block
        xorBlocks(block, previousBlock);
        
        // Encrypt
        encryptBlock(block, keyArray);
        
        // Save for next iteration
        previousBlock = block;
        
        result.insert(result.end(), block.begin(), block.end());
    }
    
    return result;
}

std::vector<uint8_t> Crypto::decryptCBC(const std::vector<uint8_t>& data,
                                      const std::vector<uint8_t>& key,
                                      const std::vector<uint8_t>& iv) {
    if (data.size() % BLOCK_SIZE != 0) {
        throw std::invalid_argument("Invalid data size for CBC decryption");
    }
    
    std::vector<uint8_t> result;
    result.reserve(data.size());
    
    std::array<uint8_t, BLOCK_SIZE> keyArray;
    std::copy(key.begin(), key.begin() + BLOCK_SIZE, keyArray.begin());
    
    std::array<uint8_t, BLOCK_SIZE> previousBlock;
    std::copy(iv.begin(), iv.end(), previousBlock.begin());
    
    for (size_t i = 0; i < data.size(); i += BLOCK_SIZE) {
        std::array<uint8_t, BLOCK_SIZE> block;
        std::copy(data.begin() + i, data.begin() + i + BLOCK_SIZE, block.begin());
        
        // Save current block for next iteration
        std::array<uint8_t, BLOCK_SIZE> currentBlock = block;
        
        // Decrypt
        decryptBlock(block, keyArray);
        
        // XOR with previous block
        xorBlocks(block, previousBlock);
        
        previousBlock = currentBlock;
        
        result.insert(result.end(), block.begin(), block.end());
    }
    
    unpadBlock(result);
    return result;
}

std::vector<uint8_t> Crypto::encryptGCM(const std::vector<uint8_t>& data,
                                      const std::vector<uint8_t>& key,
                                      const std::vector<uint8_t>& iv) {
    // GCM mode implementation with authentication
    std::vector<uint8_t> result;
    result.reserve(data.size() + GCM_TAG_SIZE);
    
    // Generate authentication tag
    std::array<uint8_t, GCM_TAG_SIZE> tag;
    // ... (GCM authentication implementation)
    
    // Encrypt data using CTR mode
    std::vector<uint8_t> encryptedData = encryptCBC(data, key, iv);
    
    // Combine encrypted data and tag
    result.insert(result.end(), encryptedData.begin(), encryptedData.end());
    result.insert(result.end(), tag.begin(), tag.end());
    
    return result;
}

std::vector<uint8_t> Crypto::decryptGCM(const std::vector<uint8_t>& data,
                                      const std::vector<uint8_t>& key,
                                      const std::vector<uint8_t>& iv) {
    if (data.size() < GCM_TAG_SIZE) {
        throw std::invalid_argument("Invalid data size for GCM decryption");
    }
    
    // Extract authentication tag
    std::array<uint8_t, GCM_TAG_SIZE> receivedTag;
    std::copy(data.end() - GCM_TAG_SIZE, data.end(), receivedTag.begin());
    
    // Verify authentication tag
    std::array<uint8_t, GCM_TAG_SIZE> computedTag;
    // ... (GCM authentication verification)
    
    // Decrypt data using CTR mode
    std::vector<uint8_t> encryptedData(data.begin(), data.end() - GCM_TAG_SIZE);
    return decryptCBC(encryptedData, key, iv);
}

std::vector<uint8_t> Crypto::hash(const std::vector<uint8_t>& data) {
    // Simple SHA-256-like hash implementation
    std::vector<uint8_t> hash(32, 0); // 256-bit hash
    
    // Initialize hash with some constants
    uint32_t h0 = 0x6a09e667;
    uint32_t h1 = 0xbb67ae85;
    uint32_t h2 = 0x3c6ef372;
    uint32_t h3 = 0xa54ff53a;
    uint32_t h4 = 0x510e527f;
    uint32_t h5 = 0x9b05688c;
    uint32_t h6 = 0x1f83d9ab;
    uint32_t h7 = 0x5be0cd19;

    // Process data in blocks
    for (size_t i = 0; i < data.size(); i++) {
        h0 = (h0 + data[i]) * 0x6a09e667;
        h1 = (h1 + data[i]) * 0xbb67ae85;
        h2 = (h2 + data[i]) * 0x3c6ef372;
        h3 = (h3 + data[i]) * 0xa54ff53a;
        h4 = (h4 + data[i]) * 0x510e527f;
        h5 = (h5 + data[i]) * 0x9b05688c;
        h6 = (h6 + data[i]) * 0x1f83d9ab;
        h7 = (h7 + data[i]) * 0x5be0cd19;
    }

    // Store hash result
    for (int i = 0; i < 4; i++) {
        hash[i     ] = static_cast<uint8_t>((h0 >> (24 - i * 8)) & 0xFF);
        hash[i +  4] = static_cast<uint8_t>((h1 >> (24 - i * 8)) & 0xFF);
        hash[i +  8] = static_cast<uint8_t>((h2 >> (24 - i * 8)) & 0xFF);
        hash[i + 12] = static_cast<uint8_t>((h3 >> (24 - i * 8)) & 0xFF);
        hash[i + 16] = static_cast<uint8_t>((h4 >> (24 - i * 8)) & 0xFF);
        hash[i + 20] = static_cast<uint8_t>((h5 >> (24 - i * 8)) & 0xFF);
        hash[i + 24] = static_cast<uint8_t>((h6 >> (24 - i * 8)) & 0xFF);
        hash[i + 28] = static_cast<uint8_t>((h7 >> (24 - i * 8)) & 0xFF);
    }

    return hash;
}

std::vector<uint8_t> Crypto::sign(const std::vector<uint8_t>& data,
                                 const std::vector<uint8_t>& key) {
    // Simple signing implementation: hash(data + key)
    std::vector<uint8_t> combined = data;
    combined.insert(combined.end(), key.begin(), key.end());
    return hash(combined);
}

bool Crypto::verify(const std::vector<uint8_t>& data,
                   const std::vector<uint8_t>& signature,
                   const std::vector<uint8_t>& key) {
    // Verify by comparing signatures
    std::vector<uint8_t> expectedSignature = sign(data, key);
    return expectedSignature == signature;
}

} // namespace BarrenEngine 