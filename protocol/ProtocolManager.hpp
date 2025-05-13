#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <queue>
#include <mutex>
#include <atomic>
#include <chrono>

namespace BarrenEngine {

enum class ProtocolType {
    UDP,
    TCP,
    WEBSOCKET,
    QUIC,
    WEBRTC
};

struct ProtocolConfig {
    ProtocolType type;
    uint16_t port;
    std::string host;
    size_t maxConnections;
    size_t bufferSize;
    bool enableMultiplexing;
    bool enableCompression;
    bool enableEncryption;
};

struct ProtocolStats {
    size_t bytesSent;
    size_t bytesReceived;
    size_t packetsSent;
    size_t packetsReceived;
    double latency;
    double packetLoss;
    size_t activeConnections;
    size_t queuedMessages;
};

class ProtocolManager {
public:
    ProtocolManager();
    ~ProtocolManager();

    bool initialize(const ProtocolConfig& config);
    bool start();
    void stop();
    bool isRunning() const { return running_; }

    // Connection Management
    bool connect(const std::string& address, uint16_t port);
    void disconnect(const std::string& address);
    bool isConnected(const std::string& address) const;
    std::vector<std::string> getConnectedPeers() const;

    // Message Handling
    bool send(const std::string& address, const std::vector<uint8_t>& data);
    bool broadcast(const std::vector<uint8_t>& data);
    std::vector<uint8_t> receive(const std::string& address);

    // Protocol Features
    void enableMultiplexing(bool enable);
    void setCompression(bool enable);
    void setEncryption(bool enable);
    void setProtocolType(ProtocolType type);

    // Statistics
    ProtocolStats getStats();
    void resetStats();

    // Callbacks
    using MessageCallback = std::function<void(const std::string&, const std::vector<uint8_t>&)>;
    using ConnectionCallback = std::function<void(const std::string&, bool)>;
    void setMessageCallback(MessageCallback callback);
    void setConnectionCallback(ConnectionCallback callback);

    // Protocol-specific implementations
    class ProtocolImpl;
    std::unique_ptr<ProtocolImpl> impl_;

private:
    ProtocolConfig config_;
    std::atomic<bool> running_;
    std::atomic<bool> multiplexingEnabled_;
    std::atomic<bool> compressionEnabled_;
    std::atomic<bool> encryptionEnabled_;
    
    std::unordered_map<std::string, std::queue<std::vector<uint8_t>>> messageQueues_;
    std::mutex queueMutex_;
    
    ProtocolStats stats_;
    std::mutex statsMutex_;
    
    MessageCallback messageCallback_;
    ConnectionCallback connectionCallback_;
    
    void updateStats(const ProtocolStats& newStats);
    void processIncomingMessage(const std::string& address, const std::vector<uint8_t>& data);
    void handleConnectionEvent(const std::string& address, bool connected);
    bool validateAddress(const std::string& address) const;
};

} // namespace BarrenEngine 