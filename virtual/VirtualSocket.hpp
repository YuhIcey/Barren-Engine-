#pragma once

#include <cstdint>
#include <vector>
#include <queue>
#include <mutex>
#include <chrono>
#include <random>
#include <functional>
#include <memory>
#include <string>
#include <map>

namespace BarrenEngine {
namespace Virtual {

enum class PacketPriority {
    IMMEDIATE_PRIORITY,    // Highest priority, sent immediately
    HIGH_PRIORITY,         // High priority, sent within 10ms
    MEDIUM_PRIORITY,       // Normal priority, sent within 100ms
    LOW_PRIORITY,          // Low priority, sent within 500ms
    LOWEST_PRIORITY        // Lowest priority, sent when bandwidth available
};

enum class PacketReliability {
    UNRELIABLE,                    // No guarantee of delivery
    UNRELIABLE_SEQUENCED,          // No guarantee, but packets arrive in order
    RELIABLE,                      // Guaranteed delivery
    RELIABLE_ORDERED,              // Guaranteed delivery and order
    RELIABLE_SEQUENCED,            // Guaranteed delivery, order within sequence
    RELIABLE_WITH_ACK_RECEIPT,     // Guaranteed delivery with acknowledgment
    RELIABLE_ORDERED_WITH_ACK_RECEIPT // Guaranteed delivery, order, and acknowledgment
};

struct NetworkCondition {
    float packetLoss;        // Probability of packet loss (0.0 to 1.0)
    float latency;          // Base latency in milliseconds
    float jitter;           // Random latency variation in milliseconds
    float bandwidth;        // Bandwidth limit in bytes per second
    float corruption;       // Probability of packet corruption (0.0 to 1.0)
    float reorder;          // Probability of packet reordering (0.0 to 1.0)
    uint32_t mtu;          // Maximum Transmission Unit in bytes
};

struct QoSProfile {
    PacketPriority priority;
    PacketReliability reliability;
    uint32_t maxRetries;           // Maximum number of retransmission attempts
    uint32_t timeout;              // Timeout in milliseconds
    bool compression;              // Enable compression for this profile
    bool encryption;               // Enable encryption for this profile
};

class VirtualSocket {
public:
    VirtualSocket();
    ~VirtualSocket();
    VirtualSocket(const VirtualSocket&) = delete;
    VirtualSocket& operator=(const VirtualSocket&) = delete;
    VirtualSocket(VirtualSocket&& other) noexcept;
    VirtualSocket& operator=(VirtualSocket&& other) noexcept;

    // Socket operations
    bool bind(uint16_t port);
    bool connect(const std::string& address, uint16_t port);
    bool listen(int backlog);
    VirtualSocket accept();
    void close();

    // Data transfer with QoS
    int send(const std::vector<uint8_t>& data, const QoSProfile& qos = defaultQoS_);
    int receive(std::vector<uint8_t>& data);
    int sendTo(const std::vector<uint8_t>& data, const std::string& address, uint16_t port, 
               const QoSProfile& qos = defaultQoS_);
    int receiveFrom(std::vector<uint8_t>& data, std::string& address, uint16_t& port);

    // Network condition simulation
    void setNetworkCondition(const NetworkCondition& condition);
    NetworkCondition getNetworkCondition() const;
    void enableSimulation(bool enable);
    bool isSimulationEnabled() const;

    // QoS Management
    void setDefaultQoS(const QoSProfile& qos);
    QoSProfile getDefaultQoS() const;
    void setQoSProfile(uint32_t profileId, const QoSProfile& qos);
    QoSProfile getQoSProfile(uint32_t profileId) const;

    // Statistics
    struct Statistics {
        uint64_t packetsSent;
        uint64_t packetsReceived;
        uint64_t packetsLost;
        uint64_t packetsCorrupted;
        uint64_t packetsReordered;
        uint64_t bytesSent;
        uint64_t bytesReceived;
        float averageLatency;
        float currentBandwidth;
        std::map<PacketPriority, uint64_t> packetsByPriority;
        std::map<PacketReliability, uint64_t> packetsByReliability;
        uint64_t retransmissions;
        uint64_t acknowledgments;
        float packetLossRate;
        float corruptionRate;
        float reorderRate;
    };
    Statistics getStatistics() const;
    void resetStatistics();

    // Callbacks
    using PacketCallback = std::function<void(const std::vector<uint8_t>&, const std::string&, uint16_t)>;
    using AckCallback = std::function<void(uint32_t, bool)>;
    void setPacketCallback(PacketCallback callback);
    void setAckCallback(AckCallback callback);

private:
    struct Packet {
        std::vector<uint8_t> data;
        std::string address;
        uint16_t port;
        std::chrono::steady_clock::time_point timestamp;
        bool isCorrupted;
        uint32_t sequenceNumber;
        uint32_t profileId;
        QoSProfile qos;
        uint32_t retryCount;
        std::vector<uint32_t> ackSequenceNumbers;
    };

    // Network simulation
    bool simulatePacketLoss();
    bool simulateCorruption(std::vector<uint8_t>& data);
    void simulateLatency(Packet& packet);
    void simulateReordering();
    void updateBandwidth(Packet& packet);

    // QoS handling
    void processQoS(Packet& packet);
    void handleReliability(Packet& packet);
    void handleOrdering(Packet& packet);
    void handleAcknowledgments();
    void retransmitPacket(const Packet& packet);

    // Internal helpers
    void processPacket(const Packet& packet);
    void updateStatistics(const Packet& packet, bool wasLost, bool wasCorrupted, bool wasReordered);
    uint32_t calculateChecksum(const std::vector<uint8_t>& data);
    uint32_t generateSequenceNumber();
    bool isPacketExpired(const Packet& packet) const;

    // Member variables
    bool isBound_;
    bool isConnected_;
    bool isListening_;
    bool simulationEnabled_;
    uint16_t localPort_;
    std::string remoteAddress_;
    uint16_t remotePort_;
    NetworkCondition networkCondition_;
    Statistics statistics_;

    // Packet queues with priority
    std::map<PacketPriority, std::queue<Packet>> sendQueues_;
    std::queue<Packet> receiveQueue_;
    std::mutex sendMutex_;
    std::mutex receiveMutex_;

    // QoS profiles
    std::map<uint32_t, QoSProfile> qosProfiles_;
    static const QoSProfile defaultQoS_;

    // Sequence numbers and ordering
    uint32_t nextSequenceNumber_;
    std::map<uint32_t, std::vector<Packet>> orderedPackets_;
    std::map<uint32_t, std::chrono::steady_clock::time_point> ackTimeouts_;

    // Random number generation
    std::mt19937 rng_;
    std::uniform_real_distribution<float> dist_;

    // Callbacks
    PacketCallback packetCallback_;
    AckCallback ackCallback_;

    // Bandwidth control
    std::chrono::steady_clock::time_point lastBandwidthUpdate_;
    uint64_t bytesInCurrentWindow_;
};

} // namespace Virtual
} // namespace BarrenEngine 