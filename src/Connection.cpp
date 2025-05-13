#include "Connection.hpp"
#include <algorithm>
#include <iostream>

namespace BarrenEngine {

Connection::Connection(uint32_t maxPacketSize)
    : nextSequenceNumber_(0)
    , maxPacketSize_(maxPacketSize)
    , connected_(false)
    , rtt_(0.0f)
    , packetLoss_(0.0f)
    , packetsSent_(0)
    , packetsReceived_(0)
    , packetsLost_(0)
    , lastStatsUpdate_(std::chrono::steady_clock::now())
{
}

Connection::~Connection() {
    std::lock_guard<std::mutex> lock(packetMutex_);
    unacknowledgedPackets_.clear();
    while (!outgoingPackets_.empty()) {
        outgoingPackets_.pop();
    }
}

void Connection::queuePacket(const std::vector<uint8_t>& data, PacketReliability reliability) {
    std::lock_guard<std::mutex> lock(packetMutex_);
    
    Packet packet;
    packet.sequenceNumber = nextSequenceNumber_++;
    packet.timestamp = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    packet.reliability = reliability;
    packet.data = data;
    packet.isAcknowledged = false;
    packet.lastResendTime = std::chrono::steady_clock::now();

    if (reliability == PacketReliability::UNRELIABLE) {
        outgoingPackets_.push(packet);
    } else {
        unacknowledgedPackets_[packet.sequenceNumber] = packet;
    }
}

bool Connection::processIncomingPacket(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(uint32_t)) {
        return false;
    }

    // Extract sequence number from the first 4 bytes
    uint32_t sequenceNumber = *reinterpret_cast<const uint32_t*>(data.data());
    
    // Send acknowledgment
    std::vector<uint8_t> ackData(sizeof(uint32_t));
    *reinterpret_cast<uint32_t*>(ackData.data()) = sequenceNumber;
    queuePacket(ackData, PacketReliability::UNRELIABLE);

    // Process the actual packet data
    std::lock_guard<std::mutex> lock(packetMutex_);
    packetsReceived_++;

    // Handle acknowledgment if this is an ack packet
    if (data.size() == sizeof(uint32_t)) {
        handleAcknowledgment(sequenceNumber);
        return true;
    }

    return true;
}

std::vector<Packet> Connection::getPacketsToSend() {
    std::lock_guard<std::mutex> lock(packetMutex_);
    std::vector<Packet> packets;

    // Get all unacknowledged packets that need to be resent
    for (auto& pair : unacknowledgedPackets_) {
        if (shouldResendPacket(pair.second)) {
            packets.push_back(pair.second);
            pair.second.lastResendTime = std::chrono::steady_clock::now();
        }
    }

    // Get all queued packets
    while (!outgoingPackets_.empty()) {
        packets.push_back(outgoingPackets_.front());
        outgoingPackets_.pop();
    }

    return packets;
}

void Connection::update(float deltaTime) {
    std::lock_guard<std::mutex> lock(packetMutex_);
    
    // Update statistics
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastStatsUpdate_).count() >= 1) {
        updateStatistics();
        lastStatsUpdate_ = now;
    }

    // Resend unacknowledged packets
    resendUnacknowledgedPackets();
}

void Connection::handleAcknowledgment(uint32_t sequenceNumber) {
    auto it = unacknowledgedPackets_.find(sequenceNumber);
    if (it != unacknowledgedPackets_.end()) {
        it->second.isAcknowledged = true;
        unacknowledgedPackets_.erase(it);
    }
}

void Connection::resendUnacknowledgedPackets() {
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = unacknowledgedPackets_.begin(); it != unacknowledgedPackets_.end();) {
        if (shouldResendPacket(it->second)) {
            it->second.lastResendTime = now;
            ++it;
        } else {
            it = unacknowledgedPackets_.erase(it);
        }
    }
}

bool Connection::shouldResendPacket(const Packet& packet) const {
    if (packet.isAcknowledged) {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastResend = std::chrono::duration_cast<std::chrono::seconds>(
        now - packet.lastResendTime).count();

    return timeSinceLastResend >= RESEND_TIMEOUT;
}

void Connection::updateStatistics() {
    if (packetsSent_ > 0) {
        packetLoss_ = static_cast<float>(packetsLost_) / packetsSent_;
    }
}

} // namespace BarrenEngine 