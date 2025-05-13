#include "connection/ConnectionManager.hpp"
#include <thread>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace BarrenEngine {

ConnectionManager::ConnectionManager()
    : running_(false)
    , monitoring_(false)
    , monitoringInterval_(1000) // Default 1 second
{
    resetStats();
}

ConnectionManager::~ConnectionManager() {
    stop();
}

bool ConnectionManager::initialize(const ConnectionConfig& config) {
    if (!validateConfig(config)) {
        return false;
    }
    
    config_ = config;
    return true;
}

bool ConnectionManager::start() {
    if (running_) return true;
    
    running_ = true;
    return true;
}

void ConnectionManager::stop() {
    if (!running_) return;
    
    running_ = false;
    monitoring_ = false;
    
    // Disconnect all connections
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    for (const auto& [address, state] : connectionStates_) {
        if (state == ConnectionState::CONNECTED) {
            disconnect(address);
        }
    }
    connectionStates_.clear();
}

bool ConnectionManager::connect(const std::string& address, uint16_t port) {
    if (!running_ || !validateAddress(address) || !validatePort(port)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    
    // Check if already connected
    if (connectionStates_[address] == ConnectionState::CONNECTED) {
        return true;
    }
    
    // Set connecting state
    connectionStates_[address] = ConnectionState::CONNECTING;
    
    // Attempt connection
    bool success = true; // Implementation specific
    
    if (success) {
        connectionStates_[address] = ConnectionState::CONNECTED;
        handleConnection(address, true);
    } else {
        connectionStates_[address] = ConnectionState::FAILED;
        handleConnectionFailure(address, "Connection failed");
    }
    
    return success;
}

void ConnectionManager::disconnect(const std::string& address) {
    if (!running_) return;
    
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    
    if (connectionStates_[address] == ConnectionState::CONNECTED) {
        connectionStates_[address] = ConnectionState::DISCONNECTING;
        
        // Implementation specific disconnect
        
        connectionStates_[address] = ConnectionState::DISCONNECTED;
        handleDisconnection(address);
    }
}

bool ConnectionManager::isConnected(const std::string& address) {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    return connectionStates_.count(address) > 0 && 
           connectionStates_.at(address) == ConnectionState::CONNECTED;
}

std::vector<std::string> ConnectionManager::getConnectedPeers() {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    std::vector<std::string> peers;
    for (const auto& pair : connectionStates_) {
        if (pair.second == ConnectionState::CONNECTED) {
            peers.push_back(pair.first);
        }
    }
    return peers;
}

ConnectionState ConnectionManager::getConnectionState(const std::string& address) {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    return connectionStates_.count(address) > 0 ? 
           connectionStates_.at(address) : ConnectionState::DISCONNECTED;
}

bool ConnectionManager::send(const std::string& address, const std::vector<uint8_t>& data) {
    if (!running_ || !isConnected(address)) return false;
    
    bool success = true; // Implementation specific
    
    if (success) {
        handleDataSent(address, data);
    } else {
        handleError(address, "Failed to send data");
    }
    
    return success;
}

bool ConnectionManager::broadcast(const std::vector<uint8_t>& data) {
    if (!running_) return false;
    
    bool success = true;
    for (const auto& peer : getConnectedPeers()) {
        success &= send(peer, data);
    }
    
    return success;
}

std::vector<uint8_t> ConnectionManager::receive(const std::string& address) {
    if (!running_ || !isConnected(address)) return {};
    
    std::vector<uint8_t> data; // Implementation specific
    
    if (!data.empty()) {
        handleDataReceived(address, data);
    }
    
    return data;
}

void ConnectionManager::setConnectionTimeout(uint32_t timeoutMs) {
    config_.timeoutMs = timeoutMs;
}

void ConnectionManager::setMaxRetries(uint32_t maxRetries) {
    config_.maxRetries = maxRetries;
}

void ConnectionManager::setKeepAliveInterval(uint32_t intervalMs) {
    config_.keepAliveIntervalMs = intervalMs;
}

void ConnectionManager::setMaxPacketSize(uint32_t size) {
    config_.maxPacketSize = size;
}

void ConnectionManager::enableCompression(bool enable) {
    config_.enableCompression = enable;
}

void ConnectionManager::enableEncryption(bool enable) {
    config_.enableEncryption = enable;
}

void ConnectionManager::enableReliability(bool enable) {
    config_.enableReliability = enable;
}

void ConnectionManager::enableOrdering(bool enable) {
    config_.enableOrdering = enable;
}

void ConnectionManager::enableSequencing(bool enable) {
    config_.enableSequencing = enable;
}

ConnectionStats ConnectionManager::getStats() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return globalStats_;
}

void ConnectionManager::resetStats() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    globalStats_ = ConnectionStats{};
    connectionStats_.clear();
}

ConnectionStats ConnectionManager::getConnectionStats(const std::string& address) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return connectionStats_.count(address) > 0 ? 
           connectionStats_.at(address) : ConnectionStats{};
}

void ConnectionManager::setConnectionEventCallback(ConnectionEventCallback callback) {
    connectionEventCallback_ = callback;
}

void ConnectionManager::setDataCallback(DataCallback callback) {
    dataCallback_ = callback;
}

void ConnectionManager::setErrorCallback(ErrorCallback callback) {
    errorCallback_ = callback;
}

void ConnectionManager::startMonitoring() {
    if (!running_) return;
    
    monitoring_ = true;
    lastMonitoring_ = std::chrono::system_clock::now();
}

void ConnectionManager::stopMonitoring() {
    monitoring_ = false;
}

bool ConnectionManager::isMonitoring() const {
    return monitoring_;
}

void ConnectionManager::setMonitoringInterval(uint32_t intervalMs) {
    monitoringInterval_ = intervalMs;
}

void ConnectionManager::handleConnection(const std::string& address, bool connected) {
    ConnectionEvent event{
        connected ? ConnectionEventType::CONNECTED : ConnectionEventType::DISCONNECTED,
        address,
        connected ? "Connected successfully" : "Disconnected",
        std::chrono::system_clock::now()
    };
    
    eventQueue_.push(event);
    
    if (connectionEventCallback_) {
        connectionEventCallback_(event);
    }
}

void ConnectionManager::handleDisconnection(const std::string& address) {
    handleConnection(address, false);
}

void ConnectionManager::handleConnectionFailure(const std::string& address, const std::string& reason) {
    ConnectionEvent event{
        ConnectionEventType::CONNECTION_FAILED,
        address,
        reason,
        std::chrono::system_clock::now()
    };
    
    eventQueue_.push(event);
    
    if (connectionEventCallback_) {
        connectionEventCallback_(event);
    }
    
    if (errorCallback_) {
        errorCallback_(address, reason);
    }
}

void ConnectionManager::handleConnectionTimeout(const std::string& address) {
    ConnectionEvent event{
        ConnectionEventType::CONNECTION_TIMEOUT,
        address,
        "Connection timed out",
        std::chrono::system_clock::now()
    };
    
    eventQueue_.push(event);
    
    if (connectionEventCallback_) {
        connectionEventCallback_(event);
    }
}

void ConnectionManager::handleConnectionRetry(const std::string& address) {
    ConnectionEvent event{
        ConnectionEventType::CONNECTION_RETRY,
        address,
        "Retrying connection",
        std::chrono::system_clock::now()
    };
    
    eventQueue_.push(event);
    
    if (connectionEventCallback_) {
        connectionEventCallback_(event);
    }
}

void ConnectionManager::handleDataReceived(const std::string& address, const std::vector<uint8_t>& data) {
    if (dataCallback_) {
        dataCallback_(address, data);
    }
}

void ConnectionManager::handleDataSent(const std::string& address, const std::vector<uint8_t>& data) {
    // Update statistics
    std::lock_guard<std::mutex> lock(statsMutex_);
    if (connectionStats_.count(address) > 0) {
        connectionStats_[address].bytesSent += data.size();
        connectionStats_[address].packetsSent++;
        connectionStats_[address].lastActivity = std::chrono::system_clock::now();
    }
}

void ConnectionManager::handleError(const std::string& address, const std::string& error) {
    if (errorCallback_) {
        errorCallback_(address, error);
    }
}

void ConnectionManager::checkConnections() {
    if (!running_ || !monitoring_) return;
    
    auto now = std::chrono::system_clock::now();
    if (now - lastMonitoring_ < std::chrono::milliseconds(monitoringInterval_)) {
        return;
    }
    
    lastMonitoring_ = now;
    
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    for (const auto& [address, state] : connectionStates_) {
        if (state == ConnectionState::CONNECTED) {
            // Check connection health
            bool healthy = true; // Implementation specific
            
            if (!healthy) {
                handleConnectionTimeout(address);
            }
        }
    }
}

void ConnectionManager::sendKeepAlive() {
    if (!running_) return;
    
    auto now = std::chrono::system_clock::now();
    if (now - lastKeepAlive_ < std::chrono::milliseconds(config_.keepAliveIntervalMs)) {
        return;
    }
    
    lastKeepAlive_ = now;
    
    // Send keep-alive packets to all connected peers
    for (const auto& peer : getConnectedPeers()) {
        std::vector<uint8_t> keepAliveData{0}; // Implementation specific
        send(peer, keepAliveData);
    }
}

void ConnectionManager::processQueuedMessages() {
    while (!eventQueue_.empty()) {
        const auto& event = eventQueue_.front();
        
        if (connectionEventCallback_) {
            connectionEventCallback_(event);
        }
        
        eventQueue_.pop();
    }
}

void ConnectionManager::cleanupStaleConnections() {
    if (!running_) return;
    
    auto now = std::chrono::system_clock::now();
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    
    for (auto it = connectionStates_.begin(); it != connectionStates_.end();) {
        if (it->second == ConnectionState::CONNECTED) {
            auto lastActivity = connectionStats_[it->first].lastActivity;
            if (now - lastActivity > std::chrono::milliseconds(config_.timeoutMs)) {
                handleConnectionTimeout(it->first);
                it = connectionStates_.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
}

void ConnectionManager::updateStats(const std::string& address, const ConnectionStats& stats) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    connectionStats_[address] = stats;
    updateGlobalStats();
}

void ConnectionManager::updateGlobalStats() {
    globalStats_ = ConnectionStats{};
    
    for (const auto& [address, stats] : connectionStats_) {
        globalStats_.bytesSent += stats.bytesSent;
        globalStats_.bytesReceived += stats.bytesReceived;
        globalStats_.packetsSent += stats.packetsSent;
        globalStats_.packetsReceived += stats.packetsReceived;
        globalStats_.packetsLost += stats.packetsLost;
        globalStats_.packetsOutOfOrder += stats.packetsOutOfOrder;
        globalStats_.activeConnections++;
    }
    
    if (!connectionStats_.empty()) {
        globalStats_.averageLatency = 0;
        for (const auto& [address, stats] : connectionStats_) {
            globalStats_.averageLatency += stats.averageLatency;
        }
        globalStats_.averageLatency /= connectionStats_.size();
    }
}

void ConnectionManager::resetConnectionStats(const std::string& address) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    connectionStats_[address] = ConnectionStats{};
}

bool ConnectionManager::validateAddress(const std::string& address) {
    if (address.empty()) return false;
    
    std::istringstream iss(address);
    std::string token;
    int count = 0;
    
    while (std::getline(iss, token, '.')) {
        if (count >= 4) return false;
        
        try {
            int num = std::stoi(token);
            if (num < 0 || num > 255) return false;
        } catch (...) {
            return false;
        }
        
        count++;
    }
    
    return count == 4;
}

bool ConnectionManager::validatePort(uint16_t port) {
    return port > 0 && port < 65536;
}

bool ConnectionManager::validateConfig(const ConnectionConfig& config) {
    if (!validateAddress(config.address) || !validatePort(config.port)) {
        return false;
    }
    
    if (config.timeoutMs == 0 || config.maxRetries == 0 || 
        config.keepAliveIntervalMs == 0 || config.maxPacketSize == 0) {
        return false;
    }
    
    return true;
}

} // namespace BarrenEngine 