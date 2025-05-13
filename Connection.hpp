#pragma once

#include <unordered_map>
#include <queue>
#include <chrono>
#include <mutex>
#include <vector>
#include <cstdint>

namespace BarrenEngine {

enum class PacketReliability {
    UNRELIABLE,              // No guarantee of delivery
    UNRELIABLE_SEQUENCED,    // No guarantee of delivery, but packets arrive in order
    RELIABLE,                // Guaranteed delivery
    RELIABLE_SEQUENCED,      // Guaranteed delivery and order
    RELIABLE_ORDERED         // Guaranteed delivery and strict order
};

struct Packet {
    uint32_t sequenceNumber;
    uint32_t timestamp;
    PacketReliability reliability;
    std::vector<uint8_t> data;
    bool isAcknowledged;
    std::chrono::steady_clock::time_point lastResendTime;
};

class Connection {
public:
    Connection(uint32_t maxPacketSize = 1024);
    ~Connection();

    // Packet handling
    void queuePacket(const std::vector<uint8_t>& data, PacketReliability reliability);
    bool processIncomingPacket(const std::vector<uint8_t>& data);
    std::vector<Packet> getPacketsToSend();
    void update(float deltaTime);

    // Connection state
    bool isConnected() const { return connected_; }
    void setConnected(bool connected) { connected_ = connected; }
    float getRTT() const { return rtt_; }
    float getPacketLoss() const { return packetLoss_; }

    // Statistics
    uint32_t getPacketsSent() const { return packetsSent_; }
    uint32_t getPacketsReceived() const { return packetsReceived_; }
    uint32_t getPacketsLost() const { return packetsLost_; }

private:
    void handleAcknowledgment(uint32_t sequenceNumber);
    void resendUnacknowledgedPackets();
    bool shouldResendPacket(const Packet& packet) const;
    void updateStatistics();

    std::unordered_map<uint32_t, Packet> unacknowledgedPackets_;
    std::queue<Packet> outgoingPackets_;
    std::mutex packetMutex_;

    uint32_t nextSequenceNumber_;
    uint32_t maxPacketSize_;
    bool connected_;
    float rtt_;
    float packetLoss_;

    // Statistics
    uint32_t packetsSent_;
    uint32_t packetsReceived_;
    uint32_t packetsLost_;
    std::chrono::steady_clock::time_point lastStatsUpdate_;

    // Constants
    static constexpr float RESEND_TIMEOUT = 0.1f;  // 100ms
    static constexpr float STATS_UPDATE_INTERVAL = 1.0f;  // 1 second
    static constexpr uint32_t MAX_RESEND_ATTEMPTS = 5;
};

} // namespace BarrenEngine 