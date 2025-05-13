#include "PacketPriority.hpp"
#include <algorithm>
#include <chrono>

namespace BarrenEngine {

// Implementation of bandwidth management
void PacketScheduler::updateBandwidthUsage(size_t bytes) {
    currentBandwidth_ = bytes;
}

bool PacketScheduler::enqueuePacket(const std::vector<uint8_t>& data, const PacketMetadata& metadata) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    if (packetQueue_.size() >= maxQueueSize_) {
        return false;
    }

    packetQueue_.emplace(data, metadata);
    return true;
}

bool PacketScheduler::dequeuePacket(std::vector<uint8_t>& data, PacketMetadata& metadata) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    if (packetQueue_.empty()) {
        return false;
    }

    const auto& packet = packetQueue_.top();
    
    // Check if packet has expired
    if (packet.getMetadata().deadline < std::chrono::steady_clock::now()) {
        packetQueue_.pop();
        return dequeuePacket(data, metadata); // Try next packet
    }

    data = packet.getData();
    metadata = packet.getMetadata();
    packetQueue_.pop();
    
    return true;
}

void PacketScheduler::setMaxBandwidth(size_t bandwidth) {
    maxBandwidth_ = bandwidth;
}

size_t PacketScheduler::getCurrentBandwidth() const {
    return currentBandwidth_;
}

size_t PacketScheduler::getQueueSize() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return packetQueue_.size();
}

} // namespace BarrenEngine 