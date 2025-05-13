#pragma once

#include <vector>
#include <cstdint>
#include <memory>

namespace BarrenEngine {

class Compression {
public:
    enum class Algorithm {
        NONE,
        LZ4,        
        ZSTD        // Default compression algorithm
    };

    static std::vector<uint8_t> compress(const std::vector<uint8_t>& data, Algorithm algorithm = Algorithm::ZSTD);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressedData, Algorithm algorithm = Algorithm::ZSTD);

    // Helper to determine if compression would be beneficial
    static bool shouldCompress(const std::vector<uint8_t>& data, Algorithm algorithm = Algorithm::ZSTD);

private:
    // Compression thresholds (in bytes)
    static constexpr size_t MIN_COMPRESSION_SIZE = 64;
    static constexpr float COMPRESSION_RATIO_THRESHOLD = 0.8f; // Only compress if we can achieve at least 20% reduction
};

} // namespace BarrenEngine 