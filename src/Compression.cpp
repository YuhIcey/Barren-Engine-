#include "Compression.hpp"
#include <lz4.h>
#include <zstd.h>
#include <algorithm>

namespace BarrenEngine {

std::vector<uint8_t> Compression::compress(const std::vector<uint8_t>& data, Algorithm algorithm) {
    if (data.empty() || !shouldCompress(data, algorithm)) {
        return data;
    }

    switch (algorithm) {
        case Algorithm::LZ4: {
            const int maxCompressedSize = LZ4_compressBound(data.size());
            std::vector<uint8_t> compressed(maxCompressedSize);
            
            const int compressedSize = LZ4_compress_default(
                reinterpret_cast<const char*>(data.data()),
                reinterpret_cast<char*>(compressed.data()),
                data.size(),
                maxCompressedSize
            );

            if (compressedSize > 0) {
                compressed.resize(compressedSize);
                return compressed;
            }
            break;
        }

        case Algorithm::ZSTD: {
            const size_t maxCompressedSize = ZSTD_compressBound(data.size());
            std::vector<uint8_t> compressed(maxCompressedSize);
            
            const size_t compressedSize = ZSTD_compress(
                compressed.data(),
                maxCompressedSize,
                data.data(),
                data.size(),
                3  // Compression level (1-22, higher = better compression but slower)
            );

            if (!ZSTD_isError(compressedSize)) {
                compressed.resize(compressedSize);
                return compressed;
            }
            break;
        }

        default:
            break;
    }

    return data;
}

std::vector<uint8_t> Compression::decompress(const std::vector<uint8_t>& compressedData, Algorithm algorithm) {
    if (compressedData.empty()) {
        return compressedData;
    }

    switch (algorithm) {
        case Algorithm::LZ4: {
            // First 4 bytes contain the original size
            if (compressedData.size() < 4) return compressedData;
            
            const uint32_t originalSize = *reinterpret_cast<const uint32_t*>(compressedData.data());
            std::vector<uint8_t> decompressed(originalSize);
            
            const int decompressedSize = LZ4_decompress_safe(
                reinterpret_cast<const char*>(compressedData.data() + 4),
                reinterpret_cast<char*>(decompressed.data()),
                compressedData.size() - 4,
                originalSize
            );

            if (decompressedSize > 0) {
                return decompressed;
            }
            break;
        }

        case Algorithm::ZSTD: {
            const size_t originalSize = ZSTD_getFrameContentSize(compressedData.data(), compressedData.size());
            if (originalSize == ZSTD_CONTENTSIZE_ERROR || originalSize == ZSTD_CONTENTSIZE_UNKNOWN) {
                return compressedData;
            }

            std::vector<uint8_t> decompressed(originalSize);
            const size_t decompressedSize = ZSTD_decompress(
                decompressed.data(),
                originalSize,
                compressedData.data(),
                compressedData.size()
            );

            if (!ZSTD_isError(decompressedSize)) {
                return decompressed;
            }
            break;
        }

        default:
            break;
    }

    return compressedData;
}

bool Compression::shouldCompress(const std::vector<uint8_t>& data, Algorithm algorithm) {
    if (data.size() < MIN_COMPRESSION_SIZE) {
        return false;
    }

    // Try compression and check ratio
    std::vector<uint8_t> compressed = compress(data, algorithm);
    float ratio = static_cast<float>(compressed.size()) / data.size();
    
    return ratio < COMPRESSION_RATIO_THRESHOLD;
}

} // namespace BarrenEngine 