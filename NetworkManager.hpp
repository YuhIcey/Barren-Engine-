#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <functional>
#include <atomic>
#include <map>
#include <chrono>
#include "Connection.hpp"
#include "Compression.hpp"
#include "Crypto.hpp"
#include <fstream>

#ifdef BARREN_ENGINE_EXPORTS
    #define BARREN_API __declspec(dllexport)
#else
    #define BARREN_API __declspec(dllimport)
#endif

namespace BarrenEngine {

enum class NetworkProtocol {
    UDP,
    TCP
};

struct BARREN_API NetworkConfig {
    NetworkProtocol protocol;
    uint16_t port;
    uint32_t maxConnections;
    uint32_t bufferSize;
    bool enableCompression;
    Compression::Algorithm compressionAlgorithm;
    bool enableEncryption;
    Crypto::Mode encryptionMode;
    std::vector<uint8_t> encryptionKey;
    uint32_t maxPacketSize;        // Maximum size of a single packet
    uint32_t fragmentSize;         // Size of packet fragments
    uint32_t fragmentTimeout;      // Timeout for fragment reassembly in milliseconds
    uint32_t connectionTimeout;    // Connection timeout in milliseconds
    uint32_t keepAliveInterval;    // Keep-alive interval in milliseconds
    bool enablePacketValidation;   // Enable packet validation
    bool enablePacketLogging;      // Enable packet logging
};

struct BARREN_API NetworkMessage {
    std::vector<uint8_t> data;
    uint32_t timestamp;
    PacketReliability reliability;
    uint32_t messageId;           // Unique message ID
    uint32_t fragmentIndex;       // Fragment index for fragmented messages
    uint32_t totalFragments;      // Total number of fragments
    bool isFragment;              // Whether this is a fragment
};

class BARREN_API NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    bool initialize(const NetworkConfig& config);
    void shutdown();

    bool startServer();
    bool connect(const std::string& address, uint16_t port);
    void disconnect();

    int send(const NetworkMessage& message);
    bool receive(NetworkMessage& message);
    void setMessageCallback(std::function<void(const NetworkMessage&)> callback);

    // Connection management
    void disconnectClient(uint32_t clientId);
    bool isClientConnected(uint32_t clientId) const;
    std::vector<uint32_t> getConnectedClients() const;

    // Statistics
    float getAverageLatency() const;
    float getPacketLoss() const;
    size_t getBytesSent() const;
    size_t getBytesReceived() const;

    // Advanced features
    void setPacketValidation(bool enable);
    void setPacketLogging(bool enable);
    void setKeepAliveInterval(uint32_t milliseconds);
    void setConnectionTimeout(uint32_t milliseconds);
    void setMaxPacketSize(uint32_t size);
    void setFragmentSize(uint32_t size);
    void setFragmentTimeout(uint32_t milliseconds);

private:
    struct FragmentInfo {
        std::vector<NetworkMessage> fragments;
        std::chrono::steady_clock::time_point timestamp;
        uint32_t totalFragments;
        uint32_t receivedFragments;
    };

    bool setupSocket();
    void cleanupSocket();
    void networkLoop();
    void processIncomingData(const std::vector<uint8_t>& data, uint32_t clientId);
    std::vector<uint8_t> processOutgoingData(const std::vector<uint8_t>& data);
    void updateStatistics();
    void handleKeepAlive();
    void checkConnectionTimeouts();
    void validatePacket(const std::vector<uint8_t>& data);
    void logPacket(const std::vector<uint8_t>& data, bool isOutgoing);
    std::vector<NetworkMessage> fragmentMessage(const NetworkMessage& message);
    NetworkMessage reassembleFragments(FragmentInfo& fragmentInfo);
    bool isFragmentComplete(const FragmentInfo& fragmentInfo) const;
    void cleanupExpiredFragments();

    NetworkConfig config_;
    std::atomic<bool> running_;
    int socket_;
    std::thread networkThread_;
    std::function<void(const NetworkMessage&)> messageCallback_;
    std::queue<NetworkMessage> messageQueue_;
    std::mutex messageQueueMutex_;
    std::map<uint32_t, std::unique_ptr<Connection>> connections_;
    mutable std::mutex connectionsMutex_;

    // Statistics
    std::atomic<size_t> bytesSent_;
    std::atomic<size_t> bytesReceived_;
    std::atomic<float> averageLatency_;
    std::atomic<float> packetLoss_;

    // Fragment management
    std::map<uint32_t, FragmentInfo> fragmentMap_;
    std::mutex fragmentMutex_;
    uint32_t nextMessageId_;

    // Keep-alive
    std::chrono::steady_clock::time_point lastKeepAlive_;
    std::map<uint32_t, std::chrono::steady_clock::time_point> lastActivity_;

    // Packet validation
    bool packetValidationEnabled_;
    std::vector<uint8_t> validationKey_;

    // Packet logging
    bool packetLoggingEnabled_;
    std::ofstream packetLog_;
};

} // namespace BarrenEngine 