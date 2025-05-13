#include "NetworkManager.hpp"
#include <iostream>
#include <cstring>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace BarrenEngine {

NetworkManager::NetworkManager()
    : running_(false)
    , socket_(-1)
    , bytesSent_(0)
    , bytesReceived_(0)
    , averageLatency_(0.0f)
    , packetLoss_(0.0f)
    , nextMessageId_(0)
    , packetValidationEnabled_(false)
    , packetLoggingEnabled_(false)
{
    // Socket system initialization removed (using custom socket layer)
}

NetworkManager::~NetworkManager() {
    shutdown();
    // Socket system cleanup removed (using custom socket layer)
}

bool NetworkManager::initialize(const NetworkConfig& config) {
    config_ = config;
    packetValidationEnabled_ = config.enablePacketValidation;
    packetLoggingEnabled_ = config.enablePacketLogging;

    if (packetLoggingEnabled_) {
        packetLog_.open("network_packets.log", std::ios::app);
        if (!packetLog_.is_open()) {
            std::cerr << "Failed to open packet log file" << std::endl;
            return false;
        }
    }

    return setupSocket();
}

void NetworkManager::shutdown() {
    running_ = false;
    if (networkThread_.joinable()) {
        networkThread_.join();
    }
    cleanupSocket();
    
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    connections_.clear();
}

bool NetworkManager::setupSocket() {
    // Remove all remaining references to send(socket_, ...), setupSocket(), INVALID_SOCKET, and any other socket API calls. Replace with stubs or comments for your custom socket layer.
    return true;
}

void NetworkManager::cleanupSocket() {
    // Remove all remaining references to send(socket_, ...), setupSocket(), INVALID_SOCKET, and any other socket API calls. Replace with stubs or comments for your custom socket layer.
}

bool NetworkManager::startServer() {
    // Server start logic removed (using custom socket layer)
    running_ = true;
    networkThread_ = std::thread(&NetworkManager::networkLoop, this);
    return true;
}

bool NetworkManager::connect(const std::string& address, uint16_t port) {
    // Connect logic removed (using custom socket layer)
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    connections_[0] = std::make_unique<Connection>(config_.bufferSize);
    running_ = true;
    networkThread_ = std::thread(&NetworkManager::networkLoop, this);
    return true;
}

void NetworkManager::disconnect() {
    running_ = false;
    if (networkThread_.joinable()) {
        networkThread_.join();
    }
    cleanupSocket();
}

int NetworkManager::send(const NetworkMessage& message) {
    if (!running_) return -1;

    // Generate message ID if not set
    NetworkMessage msg = message;
    if (msg.messageId == 0) {
        msg.messageId = ++nextMessageId_;
    }

    // Fragment large messages
    if (msg.data.size() > config_.fragmentSize) {
        auto fragments = fragmentMessage(msg);
        int totalSent = 0;
        for (const auto& fragment : fragments) {
            int sent = send(fragment);
            if (sent < 0) return sent;
            totalSent += sent;
        }
        return totalSent;
    }

    // Process and send the message
    std::vector<uint8_t> processedData = processOutgoingData(msg.data);
    if (processedData.empty()) return -1;

    // Log outgoing packet
    if (packetLoggingEnabled_) {
        logPacket(processedData, true);
    }

    // Validate packet if enabled
    if (packetValidationEnabled_) {
        validatePacket(processedData);
    }

    // Send the packet
    int bytesSent = 0;
    // [Custom socket send logic should go here]
    bytesSent = static_cast<int>(processedData.size());
    if (bytesSent > 0) {
        bytesSent_ += bytesSent;
    }
    return bytesSent;
}

bool NetworkManager::receive(NetworkMessage& message) {
    std::lock_guard<std::mutex> lock(messageQueueMutex_);
    if (messageQueue_.empty()) return false;

    message = messageQueue_.front();
    messageQueue_.pop();
    return true;
}

void NetworkManager::setMessageCallback(std::function<void(const NetworkMessage&)> callback) {
    messageCallback_ = callback;
}

void NetworkManager::disconnectClient(uint32_t clientId) {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    connections_.erase(clientId);
}

bool NetworkManager::isClientConnected(uint32_t clientId) const {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    return connections_.find(clientId) != connections_.end();
}

std::vector<uint32_t> NetworkManager::getConnectedClients() const {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    std::vector<uint32_t> clients;
    clients.reserve(connections_.size());
    for (const auto& pair : connections_) {
        clients.push_back(pair.first);
    }
    return clients;
}

float NetworkManager::getAverageLatency() const {
    return averageLatency_;
}

float NetworkManager::getPacketLoss() const {
    return packetLoss_;
}

size_t NetworkManager::getBytesSent() const {
    return bytesSent_;
}

size_t NetworkManager::getBytesReceived() const {
    return bytesReceived_;
}

void NetworkManager::networkLoop() {
    std::vector<uint8_t> buffer(config_.bufferSize);
    
    while (running_) {
        // [Custom socket receive logic should go here]
        // Process outgoing messages
        {
            std::lock_guard<std::mutex> lock(connectionsMutex_);
            for (auto& pair : connections_) {
                auto& connection = pair.second;
                connection->update(0.016f); // Assume 60 FPS update rate
                auto packets = connection->getPacketsToSend();
                for (const auto& packet : packets) {
                    // [Custom socket send logic should go here]
                    bytesSent_ += packet.data.size();
                }
            }
        }
        // Update statistics
        updateStatistics();
        // Small sleep to prevent CPU spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void NetworkManager::processIncomingData(const std::vector<uint8_t>& data, uint32_t clientId) {
    if (data.empty()) return;

    // Log incoming packet
    if (packetLoggingEnabled_) {
        logPacket(data, false);
    }

    // Validate packet if enabled
    if (packetValidationEnabled_) {
        validatePacket(data);
    }

    // Process the data
    std::vector<uint8_t> processedData = data;
    if (config_.enableEncryption) {
        // Extract IV from the beginning of the data
        if (data.size() < Crypto::IV_SIZE) {
            std::cerr << "Invalid encrypted data size" << std::endl;
            return;
        }

        std::vector<uint8_t> iv(data.begin(), data.begin() + Crypto::IV_SIZE);
        std::vector<uint8_t> encryptedData(data.begin() + Crypto::IV_SIZE, data.end());

        try {
            processedData = Crypto::decrypt(encryptedData, config_.encryptionKey, iv, config_.encryptionMode);
        } catch (const std::exception& e) {
            std::cerr << "Decryption failed: " << e.what() << std::endl;
            return;
        }
    }

    if (config_.enableCompression) {
        try {
            processedData = Compression::decompress(processedData, config_.compressionAlgorithm);
        } catch (const std::exception& e) {
            std::cerr << "Decompression failed: " << e.what() << std::endl;
            return;
        }
    }

    // Create message from processed data
    NetworkMessage message;
    message.data = processedData;
    message.timestamp = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());

    // Handle fragments
    if (message.isFragment) {
        std::lock_guard<std::mutex> lock(fragmentMutex_);
        auto& fragmentInfo = fragmentMap_[message.messageId];
        
        if (fragmentInfo.fragments.empty()) {
            fragmentInfo.timestamp = std::chrono::steady_clock::now();
            fragmentInfo.totalFragments = message.totalFragments;
            fragmentInfo.receivedFragments = 0;
        }

        fragmentInfo.fragments[message.fragmentIndex] = message;
        fragmentInfo.receivedFragments++;

        if (isFragmentComplete(fragmentInfo)) {
            message = reassembleFragments(fragmentInfo);
            fragmentMap_.erase(message.messageId);
        } else {
            return; // Wait for more fragments
        }
    }

    // Update last activity
    lastActivity_[clientId] = std::chrono::steady_clock::now();

    // Process the message
    if (messageCallback_) {
        messageCallback_(message);
    }

    std::lock_guard<std::mutex> lock(messageQueueMutex_);
    messageQueue_.push(message);
}

std::vector<NetworkMessage> NetworkManager::fragmentMessage(const NetworkMessage& message) {
    std::vector<NetworkMessage> fragments;
    size_t totalFragments = (message.data.size() + config_.fragmentSize - 1) / config_.fragmentSize;

    for (size_t i = 0; i < totalFragments; ++i) {
        NetworkMessage fragment;
        fragment.messageId = message.messageId;
        fragment.fragmentIndex = static_cast<uint32_t>(i);
        fragment.totalFragments = static_cast<uint32_t>(totalFragments);
        fragment.isFragment = true;
        fragment.reliability = message.reliability;
        fragment.timestamp = message.timestamp;

        size_t start = i * config_.fragmentSize;
        size_t end = std::min(start + config_.fragmentSize, message.data.size());
        fragment.data.assign(message.data.begin() + start, message.data.begin() + end);

        fragments.push_back(fragment);
    }

    return fragments;
}

NetworkMessage NetworkManager::reassembleFragments(FragmentInfo& fragmentInfo) {
    NetworkMessage reassembled;
    reassembled.messageId = fragmentInfo.fragments[0].messageId;
    reassembled.reliability = fragmentInfo.fragments[0].reliability;
    reassembled.timestamp = fragmentInfo.fragments[0].timestamp;
    reassembled.isFragment = false;

    // Calculate total size
    size_t totalSize = 0;
    for (const auto& fragment : fragmentInfo.fragments) {
        totalSize += fragment.data.size();
    }

    // Reserve space and copy data
    reassembled.data.reserve(totalSize);
    for (const auto& fragment : fragmentInfo.fragments) {
        reassembled.data.insert(reassembled.data.end(), 
            fragment.data.begin(), fragment.data.end());
    }

    return reassembled;
}

void NetworkManager::updateStatistics() {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    
    float totalLatency = 0.0f;
    float totalPacketLoss = 0.0f;
    size_t connectionCount = connections_.size();

    if (connectionCount > 0) {
        for (const auto& pair : connections_) {
            totalLatency += pair.second->getRTT();
            totalPacketLoss += pair.second->getPacketLoss();
        }

        averageLatency_ = totalLatency / connectionCount;
        packetLoss_ = totalPacketLoss / connectionCount;
    }
}

void NetworkManager::validatePacket(const std::vector<uint8_t>& data) {
    // Implement packet validation logic here
    // This could include checksums, signatures, or other validation methods
}

void NetworkManager::logPacket(const std::vector<uint8_t>& data, bool isOutgoing) {
    if (!packetLog_.is_open()) return;

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    packetLog_ << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << " "
               << (isOutgoing ? "OUT" : "IN ") << " "
               << data.size() << " bytes\n";

    // Log first 16 bytes in hex
    for (size_t i = 0; i < std::min(data.size(), size_t(16)); ++i) {
        packetLog_ << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(data[i]) << " ";
    }
    packetLog_ << std::dec << "\n\n";
}

void NetworkManager::handleKeepAlive() {
    auto now = std::chrono::steady_clock::now();
    if (now - lastKeepAlive_ >= std::chrono::milliseconds(config_.keepAliveInterval)) {
        NetworkMessage keepAlive;
        keepAlive.data = {0}; // Empty keep-alive packet
        keepAlive.reliability = PacketReliability::RELIABLE;
        send(keepAlive);
        lastKeepAlive_ = now;
    }
}

void NetworkManager::checkConnectionTimeouts() {
    auto now = std::chrono::steady_clock::now();
    std::vector<uint32_t> timeoutClients;

    for (const auto& activity : lastActivity_) {
        if (now - activity.second >= std::chrono::milliseconds(config_.connectionTimeout)) {
            timeoutClients.push_back(activity.first);
        }
    }

    for (uint32_t clientId : timeoutClients) {
        disconnectClient(clientId);
    }
}

void NetworkManager::cleanupExpiredFragments() {
    auto now = std::chrono::steady_clock::now();
    std::vector<uint32_t> expiredFragments;

    std::lock_guard<std::mutex> lock(fragmentMutex_);
    for (const auto& pair : fragmentMap_) {
        if (now - pair.second.timestamp >= std::chrono::milliseconds(config_.fragmentTimeout)) {
            expiredFragments.push_back(pair.first);
        }
    }

    for (uint32_t messageId : expiredFragments) {
        fragmentMap_.erase(messageId);
    }
}

std::vector<uint8_t> NetworkManager::processOutgoingData(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> processedData = data;

    // Apply compression if enabled
    if (config_.enableCompression) {
        processedData = Compression::compress(processedData, config_.compressionAlgorithm);
    }

    // Apply encryption if enabled
    if (config_.enableEncryption) {
        // Generate a new IV for each message
        std::vector<uint8_t> iv = Crypto::generateIV();
        
        // Encrypt the data
        processedData = Crypto::encrypt(processedData, config_.encryptionKey, iv, config_.encryptionMode);
        
        // Prepend the IV to the encrypted data
        processedData.insert(processedData.begin(), iv.begin(), iv.end());
    }

    return processedData;
}

bool NetworkManager::isFragmentComplete(const FragmentInfo& fragmentInfo) const {
    return fragmentInfo.receivedFragments == fragmentInfo.totalFragments;
}

} // namespace BarrenEngine 