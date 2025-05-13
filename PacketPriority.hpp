#pragma once

#include <queue>
#include <chrono>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace BarrenEngine {

enum class PacketPriority {
    CRITICAL = 0,    // Highest priority, immediate delivery
    HIGH = 1,        // High priority, guaranteed delivery
    MEDIUM = 2,      // Normal priority, best effort delivery
    LOW = 3,         // Low priority, can be delayed
    BACKGROUND = 4   // Lowest priority, only sent when bandwidth available
};

enum class QoSLevel {
    ULTRA_LOW_LATENCY,    // For real-time critical data
    LOW_LATENCY,          // For time-sensitive data
    BALANCED,             // Default QoS level
    HIGH_THROUGHPUT,      // For bulk data transfer
    RELIABLE              // For guaranteed delivery
};

struct PacketMetadata {
    PacketPriority priority;
    QoSLevel qos;
    std::chrono::steady_clock::time_point deadline;
    size_t size;
    uint32_t sequenceNumber;
    bool requiresAck;
    float bandwidthLimit;  // Maximum bandwidth usage in bytes per second
};

class PrioritizedPacket {
public:
    PrioritizedPacket(const std::vector<uint8_t>& data, const PacketMetadata& metadata)
        : data_(data), metadata_(metadata) {}

    const std::vector<uint8_t>& getData() const { return data_; }
    const PacketMetadata& getMetadata() const { return metadata_; }
    
    bool operator<(const PrioritizedPacket& other) const {
        if (metadata_.priority != other.metadata_.priority)
            return static_cast<int>(metadata_.priority) > static_cast<int>(other.metadata_.priority);
        return metadata_.deadline > other.metadata_.deadline;
    }

private:
    std::vector<uint8_t> data_;
    PacketMetadata metadata_;
};

class PacketScheduler {
public:
    PacketScheduler(size_t maxQueueSize = 1000)
        : maxQueueSize_(maxQueueSize)
        , currentBandwidth_(0)
        , maxBandwidth_(0) {}

    bool enqueuePacket(const std::vector<uint8_t>& data, const PacketMetadata& metadata);
    bool dequeuePacket(std::vector<uint8_t>& data, PacketMetadata& metadata);
    void setMaxBandwidth(size_t bandwidth);
    size_t getCurrentBandwidth() const;
    size_t getQueueSize();
    void updateBandwidthUsage(size_t bytes);

private:
    std::priority_queue<PrioritizedPacket> packetQueue_;
    std::mutex queueMutex_;
    size_t maxQueueSize_;
    std::atomic<size_t> currentBandwidth_;
    std::atomic<size_t> maxBandwidth_;
};

} // namespace BarrenEngine 