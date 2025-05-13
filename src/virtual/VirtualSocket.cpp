#include "virtual/VirtualSocket.hpp"
#include <algorithm>
#include <iostream>
#include <atomic>

namespace BarrenEngine {
namespace Virtual {

// Define default QoS profile
const QoSProfile VirtualSocket::defaultQoS_ = {
    PacketPriority::MEDIUM_PRIORITY,
    PacketReliability::RELIABLE,
    3,      // maxRetries
    1000,   // timeout (1 second)
    true,   // compression
    true    // encryption
};

VirtualSocket::VirtualSocket() = default;
VirtualSocket::~VirtualSocket() = default;

VirtualSocket::VirtualSocket(VirtualSocket&& other) noexcept
    : isBound_(other.isBound_),
      isConnected_(other.isConnected_),
      isListening_(other.isListening_),
      simulationEnabled_(other.simulationEnabled_),
      localPort_(other.localPort_),
      remoteAddress_(std::move(other.remoteAddress_)),
      remotePort_(other.remotePort_),
      networkCondition_(other.networkCondition_),
      statistics_(other.statistics_),
      sendQueues_(std::move(other.sendQueues_)),
      receiveQueue_(std::move(other.receiveQueue_)),
      qosProfiles_(std::move(other.qosProfiles_)),
      nextSequenceNumber_(other.nextSequenceNumber_),
      orderedPackets_(std::move(other.orderedPackets_)),
      ackTimeouts_(std::move(other.ackTimeouts_)),
      rng_(std::move(other.rng_)),
      dist_(std::move(other.dist_)),
      packetCallback_(std::move(other.packetCallback_)),
      ackCallback_(std::move(other.ackCallback_)),
      lastBandwidthUpdate_(other.lastBandwidthUpdate_),
      bytesInCurrentWindow_(other.bytesInCurrentWindow_)
{
    // mutexes are default-constructed
}

VirtualSocket& VirtualSocket::operator=(VirtualSocket&& other) noexcept {
    if (this != &other) {
        isBound_ = other.isBound_;
        isConnected_ = other.isConnected_;
        isListening_ = other.isListening_;
        simulationEnabled_ = other.simulationEnabled_;
        localPort_ = other.localPort_;
        remoteAddress_ = std::move(other.remoteAddress_);
        remotePort_ = other.remotePort_;
        networkCondition_ = other.networkCondition_;
        statistics_ = other.statistics_;
        sendQueues_ = std::move(other.sendQueues_);
        receiveQueue_ = std::move(other.receiveQueue_);
        qosProfiles_ = std::move(other.qosProfiles_);
        nextSequenceNumber_ = other.nextSequenceNumber_;
        orderedPackets_ = std::move(other.orderedPackets_);
        ackTimeouts_ = std::move(other.ackTimeouts_);
        rng_ = std::move(other.rng_);
        dist_ = std::move(other.dist_);
        packetCallback_ = std::move(other.packetCallback_);
        ackCallback_ = std::move(other.ackCallback_);
        lastBandwidthUpdate_ = other.lastBandwidthUpdate_;
        bytesInCurrentWindow_ = other.bytesInCurrentWindow_;
        // mutexes are default-constructed
    }
    return *this;
}

bool VirtualSocket::bind(uint16_t port) {
    if (isBound_) {
        return false;
    }
    localPort_ = port;
    isBound_ = true;
    return true;
}

bool VirtualSocket::connect(const std::string& address, uint16_t port) {
    if (!isBound_ || isConnected_) {
        return false;
    }
    remoteAddress_ = address;
    remotePort_ = port;
    isConnected_ = true;
    return true;
}

bool VirtualSocket::listen(int backlog) {
    if (!isBound_ || isConnected_) {
        return false;
    }
    isListening_ = true;
    return true;
}

VirtualSocket VirtualSocket::accept() {
    if (!isListening_) {
        return VirtualSocket();
    }
    // In a real implementation, this would wait for a connection
    // For now, we'll just create a new connected socket
    VirtualSocket clientSocket;
    clientSocket.bind(0); // Use any available port
    return clientSocket;
}

void VirtualSocket::close() {
    isBound_ = false;
    isConnected_ = false;
    isListening_ = false;
    localPort_ = 0;
    remotePort_ = 0;
    remoteAddress_.clear();
    std::lock_guard<std::mutex> sendLock(sendMutex_);
    std::lock_guard<std::mutex> receiveLock(receiveMutex_);
    for (auto& [priority, queue] : sendQueues_) {
        while (!queue.empty()) queue.pop();
    }
    while (!receiveQueue_.empty()) receiveQueue_.pop();
}

int VirtualSocket::send(const std::vector<uint8_t>& data, const QoSProfile& qos) {
    if (!isConnected_) {
        return -1;
    }
    return sendTo(data, remoteAddress_, remotePort_, qos);
}

int VirtualSocket::receive(std::vector<uint8_t>& data) {
    if (!isConnected_) {
        return -1;
    }
    std::string address;
    uint16_t port;
    return receiveFrom(data, address, port);
}

int VirtualSocket::sendTo(const std::vector<uint8_t>& data, const std::string& address, uint16_t port, const QoSProfile& qos) {
    if (!isBound_) {
        return -1;
    }

    // Check MTU
    if (data.size() > networkCondition_.mtu) {
        return -1;
    }

    // Create packet
    Packet packet;
    packet.data = data;
    packet.address = address;
    packet.port = port;
    packet.timestamp = std::chrono::steady_clock::now();
    packet.isCorrupted = false;
    packet.qos = qos;
    packet.sequenceNumber = generateSequenceNumber();
    packet.retryCount = 0;

    // Apply network simulation
    if (simulationEnabled_) {
        if (simulatePacketLoss()) {
            updateStatistics(packet, true, false, false);
            return static_cast<int>(data.size());
        }
        if (simulateCorruption(packet.data)) {
            packet.isCorrupted = true;
        }
        simulateLatency(packet);
        simulateReordering();
        updateBandwidth(packet);
    }

    // Queue packet for sending by priority
    {
        std::lock_guard<std::mutex> lock(sendMutex_);
        sendQueues_[qos.priority].push(packet);
    }

    updateStatistics(packet, false, packet.isCorrupted, false);
    return static_cast<int>(data.size());
}

int VirtualSocket::receiveFrom(std::vector<uint8_t>& data, std::string& address, uint16_t& port) {
    if (!isBound_) {
        return -1;
    }

    std::lock_guard<std::mutex> lock(receiveMutex_);
    if (receiveQueue_.empty()) {
        return 0;
    }

    Packet packet = receiveQueue_.front();
    receiveQueue_.pop();

    data = packet.data;
    address = packet.address;
    port = packet.port;

    // Process packet if callback is set
    if (packetCallback_) {
        packetCallback_(data, address, port);
    }

    return static_cast<int>(data.size());
}

void VirtualSocket::setNetworkCondition(const NetworkCondition& condition) {
    networkCondition_ = condition;
}

NetworkCondition VirtualSocket::getNetworkCondition() const {
    return networkCondition_;
}

void VirtualSocket::enableSimulation(bool enable) {
    simulationEnabled_ = enable;
}

bool VirtualSocket::isSimulationEnabled() const {
    return simulationEnabled_;
}

VirtualSocket::Statistics VirtualSocket::getStatistics() const {
    return statistics_;
}

void VirtualSocket::resetStatistics() {
    statistics_ = Statistics{};
}

void VirtualSocket::setPacketCallback(PacketCallback callback) {
    packetCallback_ = callback;
}

bool VirtualSocket::simulatePacketLoss() {
    return dist_(rng_) < networkCondition_.packetLoss;
}

bool VirtualSocket::simulateCorruption(std::vector<uint8_t>& data) {
    if (dist_(rng_) >= networkCondition_.corruption) {
        return false;
    }

    // Corrupt a random byte in the packet
    if (!data.empty()) {
        size_t index = static_cast<size_t>(dist_(rng_) * data.size());
        data[index] = static_cast<uint8_t>(dist_(rng_) * 256);
        return true;
    }
    return false;
}

void VirtualSocket::simulateLatency(Packet& packet) {
    if (networkCondition_.latency > 0.0f || networkCondition_.jitter > 0.0f) {
        float totalLatency = networkCondition_.latency;
        if (networkCondition_.jitter > 0.0f) {
            totalLatency += (dist_(rng_) * 2.0f - 1.0f) * networkCondition_.jitter;
        }
        packet.timestamp += std::chrono::milliseconds(static_cast<int64_t>(totalLatency));
    }
}

void VirtualSocket::simulateReordering() {
    if (networkCondition_.reorder > 0.0f && dist_(rng_) < networkCondition_.reorder) {
        std::lock_guard<std::mutex> lock(sendMutex_);
        if (sendQueues_[PacketPriority::MEDIUM_PRIORITY].size() >= 2) {
            // Swap the last two packets
            Packet p1 = sendQueues_[PacketPriority::MEDIUM_PRIORITY].front();
            sendQueues_[PacketPriority::MEDIUM_PRIORITY].pop();
            Packet p2 = sendQueues_[PacketPriority::MEDIUM_PRIORITY].front();
            sendQueues_[PacketPriority::MEDIUM_PRIORITY].pop();
            sendQueues_[PacketPriority::MEDIUM_PRIORITY].push(p2);
            sendQueues_[PacketPriority::MEDIUM_PRIORITY].push(p1);
        }
    }
}

void VirtualSocket::updateBandwidth(Packet& packet) {
    if (networkCondition_.bandwidth > 0.0f) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - lastBandwidthUpdate_).count();

        if (elapsed >= 1) {
            // Reset bandwidth counter every second
            bytesInCurrentWindow_ = 0;
            lastBandwidthUpdate_ = now;
        }

        bytesInCurrentWindow_ += packet.data.size();
        if (bytesInCurrentWindow_ > networkCondition_.bandwidth) {
            // Simulate bandwidth limit by adding delay
            float delay = (bytesInCurrentWindow_ - networkCondition_.bandwidth) / 
                         networkCondition_.bandwidth * 1000.0f;
            packet.timestamp += std::chrono::milliseconds(static_cast<int64_t>(delay));
        }
    }
}

void VirtualSocket::processPacket(const Packet& packet) {
    std::lock_guard<std::mutex> lock(receiveMutex_);
    receiveQueue_.push(packet);
}

void VirtualSocket::updateStatistics(const Packet& packet, bool wasLost, bool wasCorrupted, bool wasReordered) {
    statistics_.packetsSent++;
    if (!wasLost) {
        statistics_.packetsReceived++;
    } else {
        statistics_.packetsLost++;
    }
    if (wasCorrupted) {
        statistics_.packetsCorrupted++;
    }
    if (wasReordered) {
        statistics_.packetsReordered++;
    }
    statistics_.bytesSent += packet.data.size();
    if (!wasLost) {
        statistics_.bytesReceived += packet.data.size();
    }

    // Update average latency
    if (!wasLost) {
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - packet.timestamp).count();
        statistics_.averageLatency = (statistics_.averageLatency * (statistics_.packetsReceived - 1) + latency) / 
                                    statistics_.packetsReceived;
    }

    // Update current bandwidth
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - lastBandwidthUpdate_).count();
    if (elapsed >= 1) {
        statistics_.currentBandwidth = bytesInCurrentWindow_;
        bytesInCurrentWindow_ = 0;
        lastBandwidthUpdate_ = now;
    }
}

uint32_t VirtualSocket::calculateChecksum(const std::vector<uint8_t>& data) {
    uint32_t checksum = 0;
    for (uint8_t byte : data) {
        checksum = (checksum << 8) | byte;
        if (checksum & 0xFF000000) {
            checksum = (checksum & 0x00FFFFFF) + ((checksum & 0xFF000000) >> 24);
        }
    }
    return checksum;
}

uint32_t VirtualSocket::generateSequenceNumber() {
    static std::atomic<uint32_t> sequenceCounter(0);
    return sequenceCounter++;
}

} // namespace Virtual
} // namespace BarrenEngine 