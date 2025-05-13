#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <queue>
#include <atomic>

namespace BarrenEngine {

// Connection states
enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING,
    FAILED
};

// Connection types
enum class ConnectionType {
    CLIENT,
    SERVER,
    PEER
};

// Connection configuration
struct ConnectionConfig {
    std::string address;
    uint16_t port;
    ConnectionType type;
    uint32_t timeoutMs;
    uint32_t maxRetries;
    uint32_t keepAliveIntervalMs;
    uint32_t maxPacketSize;
    bool enableCompression;
    bool enableEncryption;
    bool enableReliability;
    bool enableOrdering;
    bool enableSequencing;
};

// Connection statistics
struct ConnectionStats {
    uint64_t bytesSent;
    uint64_t bytesReceived;
    uint64_t packetsSent;
    uint64_t packetsReceived;
    uint64_t packetsLost;
    uint64_t packetsOutOfOrder;
    uint32_t averageLatency;
    uint32_t packetLoss;
    uint32_t bandwidth;
    uint32_t queueSize;
    uint32_t activeConnections;
    uint32_t failedConnections;
    uint32_t retryCount;
    std::chrono::system_clock::time_point lastActivity;
};

// Connection event types
enum class ConnectionEventType {
    CONNECTED,
    DISCONNECTED,
    CONNECTION_FAILED,
    CONNECTION_TIMEOUT,
    CONNECTION_RETRY,
    DATA_RECEIVED,
    DATA_SENT,
    ERROR
};

// Connection event
struct ConnectionEvent {
    ConnectionEventType type;
    std::string address;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
};

// Connection callback types
using ConnectionEventCallback = std::function<void(const ConnectionEvent&)>;
using DataCallback = std::function<void(const std::string&, const std::vector<uint8_t>&)>;
using ErrorCallback = std::function<void(const std::string&, const std::string&)>;

class ConnectionManager {
public:
    ConnectionManager();
    ~ConnectionManager();

    // Connection management
    bool initialize(const ConnectionConfig& config);
    bool start();
    void stop();
    bool connect(const std::string& address, uint16_t port);
    void disconnect(const std::string& address);
    bool isConnected(const std::string& address);
    std::vector<std::string> getConnectedPeers();
    ConnectionState getConnectionState(const std::string& address);

    // Data transfer
    bool send(const std::string& address, const std::vector<uint8_t>& data);
    bool broadcast(const std::vector<uint8_t>& data);
    std::vector<uint8_t> receive(const std::string& address);

    // Configuration
    void setConnectionTimeout(uint32_t timeoutMs);
    void setMaxRetries(uint32_t maxRetries);
    void setKeepAliveInterval(uint32_t intervalMs);
    void setMaxPacketSize(uint32_t size);
    void enableCompression(bool enable);
    void enableEncryption(bool enable);
    void enableReliability(bool enable);
    void enableOrdering(bool enable);
    void enableSequencing(bool enable);

    // Statistics
    ConnectionStats getStats();
    void resetStats();
    ConnectionStats getConnectionStats(const std::string& address);

    // Callbacks
    void setConnectionEventCallback(ConnectionEventCallback callback);
    void setDataCallback(DataCallback callback);
    void setErrorCallback(ErrorCallback callback);

    // Connection monitoring
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;
    void setMonitoringInterval(uint32_t intervalMs);

private:
    // Internal connection management
    void handleConnection(const std::string& address, bool connected);
    void handleDisconnection(const std::string& address);
    void handleConnectionFailure(const std::string& address, const std::string& reason);
    void handleConnectionTimeout(const std::string& address);
    void handleConnectionRetry(const std::string& address);
    void handleDataReceived(const std::string& address, const std::vector<uint8_t>& data);
    void handleDataSent(const std::string& address, const std::vector<uint8_t>& data);
    void handleError(const std::string& address, const std::string& error);

    // Connection maintenance
    void checkConnections();
    void sendKeepAlive();
    void processQueuedMessages();
    void cleanupStaleConnections();

    // Statistics tracking
    void updateStats(const std::string& address, const ConnectionStats& stats);
    void updateGlobalStats();
    void resetConnectionStats(const std::string& address);

    // Validation
    bool validateAddress(const std::string& address);
    bool validatePort(uint16_t port);
    bool validateConfig(const ConnectionConfig& config);

    // Member variables
    ConnectionConfig config_;
    std::atomic<bool> running_;
    std::atomic<bool> monitoring_;
    std::mutex connectionsMutex_;
    std::mutex statsMutex_;
    std::unordered_map<std::string, ConnectionState> connectionStates_;
    std::unordered_map<std::string, ConnectionStats> connectionStats_;
    ConnectionStats globalStats_;
    std::queue<ConnectionEvent> eventQueue_;
    ConnectionEventCallback connectionEventCallback_;
    DataCallback dataCallback_;
    ErrorCallback errorCallback_;
    std::chrono::system_clock::time_point lastKeepAlive_;
    std::chrono::system_clock::time_point lastMonitoring_;
    uint32_t monitoringInterval_;
};

} // namespace BarrenEngine 