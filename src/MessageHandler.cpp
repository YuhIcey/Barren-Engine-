#include "message/MessageHandler.hpp"
#include <thread>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace BarrenEngine {

MessageHandler::MessageHandler()
    : running_(false)
{
    resetStats();
}

MessageHandler::~MessageHandler() {
    stop();
}

bool MessageHandler::initialize(const MessageQueueConfig& config) {
    if (running_) return true;
    
    config_ = config;
    resizeQueue(config.maxQueueSize);
    return true;
}

void MessageHandler::start() {
    if (running_) return;
    
    running_ = true;
    lastProcessed_ = std::chrono::system_clock::now();
    lastTimeoutCheck_ = std::chrono::system_clock::now();
}

void MessageHandler::stop() {
    if (!running_) return;
    
    running_ = false;
    clear();
}

bool MessageHandler::isRunning() const {
    return running_;
}

bool MessageHandler::send(const Message& message) {
    if (!running_ || !validateMessage(message)) {
        return false;
    }
    
    enqueueMessage(message);
    
    MessageEvent event{
        MessageEventType::MESSAGE_SENT,
        message,
        "",
        std::chrono::system_clock::now()
    };
    
    handleMessageEvent(event);
    return true;
}

bool MessageHandler::broadcast(const Message& message) {
    if (!running_ || !validateMessage(message)) {
        return false;
    }
    
    // Implementation specific broadcast logic
    return true;
}

bool MessageHandler::process() {
    if (!running_) return false;
    
    auto now = std::chrono::system_clock::now();
    if (now - lastProcessed_ < config_.processingInterval) {
        return true;
    }
    
    lastProcessed_ = now;
    processQueue();
    checkTimeouts();
    
    return true;
}

void MessageHandler::clear() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    clearQueue();
}

void MessageHandler::registerCallback(MessageType type, MessageCallback callback) {
    callbacks_[type] = callback;
}

void MessageHandler::unregisterCallback(MessageType type) {
    callbacks_.erase(type);
}

void MessageHandler::registerEventCallback(MessageEventCallback callback) {
    eventCallback_ = callback;
}

void MessageHandler::registerFilter(MessageFilter filter) {
    messageFilter_ = filter;
}

size_t MessageHandler::getQueueSize() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return messageQueue_.size();
}

size_t MessageHandler::getQueueCapacity() const {
    return config_.maxQueueSize;
}

bool MessageHandler::isQueueFull() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return messageQueue_.size() >= config_.maxQueueSize;
}

bool MessageHandler::isQueueEmpty() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return messageQueue_.empty();
}

void MessageHandler::setQueueSize(size_t size) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    resizeQueue(size);
}

void MessageHandler::setProcessingInterval(std::chrono::milliseconds interval) {
    config_.processingInterval = interval;
}

void MessageHandler::enableCompression(bool enable) {
    config_.enableCompression = enable;
}

void MessageHandler::enableEncryption(bool enable) {
    config_.enableEncryption = enable;
}

void MessageHandler::enableValidation(bool enable) {
    config_.enableValidation = enable;
}

void MessageHandler::enableLogging(bool enable) {
    config_.enableLogging = enable;
}

MessageStats MessageHandler::getStats() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

void MessageHandler::resetStats() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_ = MessageStats{};
}

void MessageHandler::setStatsCallback(std::function<void(const MessageStats&)> callback) {
    statsCallback_ = callback;
}

bool MessageHandler::validateMessage(const Message& message) {
    if (!config_.enableValidation) return true;
    
    return validateMetadata(message.metadata) && validateData(message.data);
}

bool MessageHandler::validateMetadata(const MessageMetadata& metadata) const {
    if (!validateMessageType(metadata.type)) return false;
    if (!validateMessagePriority(metadata.priority)) return false;
    if (!validateMessageReliability(metadata.reliability)) return false;
    if (!validateMessageSource(metadata.source)) return false;
    if (!validateMessageDestination(metadata.destination)) return false;
    if (!validateMessageTimestamp(metadata.timestamp)) return false;
    if (!validateMessageSequence(metadata.sequenceNumber)) return false;
    if (!validateMessageOrder(metadata.orderNumber)) return false;
    
    return true;
}

bool MessageHandler::validateData(const MessageData& data) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    // Validation logic here
    return true;
}

void MessageHandler::processMessage(const Message& message) {
    if (!running_) return;
    
    processMessageInternal(message);
}

void MessageHandler::handleMessageEvent(const MessageEvent& event) {
    if (!running_) return;
    
    handleMessageEventInternal(event);
}

void MessageHandler::retryMessage(const Message& message) {
    if (!running_) return;
    
    if (message.metadata.retryCount < message.metadata.maxRetries) {
        Message retryMessage = message;
        retryMessage.metadata.retryCount++;
        retryMessage.metadata.timestamp = std::chrono::system_clock::now();
        
        enqueueMessage(retryMessage);
        
        MessageEvent event{
            MessageEventType::MESSAGE_RETRIED,
            retryMessage,
            "",
            std::chrono::system_clock::now()
        };
        
        handleMessageEvent(event);
    } else {
        dropMessage(message);
    }
}

void MessageHandler::dropMessage(const Message& message) {
    MessageEvent event{
        MessageEventType::MESSAGE_DROPPED,
        message,
        "Max retries exceeded",
        std::chrono::system_clock::now()
    };
    
    handleMessageEvent(event);
    updateStats(message, false);
}

void MessageHandler::processQueue() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    while (!messageQueue_.empty()) {
        Message message = dequeueMessage();
        
        if (messageFilter_ && !messageFilter_(message)) {
            continue;
        }
        
        processMessageInternal(message);
    }
}

void MessageHandler::processMessageInternal(const Message& message) {
    auto callback = callbacks_.find(message.metadata.type);
    if (callback != callbacks_.end()) {
        callback->second(message);
    }
    
    MessageEvent event{
        MessageEventType::MESSAGE_PROCESSED,
        message,
        "",
        std::chrono::system_clock::now()
    };
    
    handleMessageEvent(event);
    updateStats(message, true);
}

void MessageHandler::handleMessageEventInternal(const MessageEvent& event) {
    if (eventCallback_) {
        eventCallback_(event);
    }
}

void MessageHandler::updateStats(const Message& message, bool processed) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    if (processed) {
        stats_.messagesProcessed++;
        stats_.bytesProcessed += getMessageSize(message);
    } else {
        stats_.messagesDropped++;
        stats_.bytesDropped += getMessageSize(message);
    }
    
    stats_.queueSize = messageQueue_.size();
    stats_.queueCapacity = config_.maxQueueSize;
    stats_.lastProcessed = std::chrono::system_clock::now();
    
    if (statsCallback_) {
        statsCallback_(stats_);
    }
}

void MessageHandler::checkTimeouts() {
    auto now = std::chrono::system_clock::now();
    if (now - lastTimeoutCheck_ < std::chrono::seconds(1)) {
        return;
    }
    
    lastTimeoutCheck_ = now;
    
    std::lock_guard<std::mutex> lock(queueMutex_);
    std::queue<Message> tempQueue;
    
    while (!messageQueue_.empty()) {
        Message message = dequeueMessage();
        
        if (now - message.metadata.timestamp > message.metadata.timeout) {
            MessageEvent event{
                MessageEventType::MESSAGE_TIMED_OUT,
                message,
                "Message timed out",
                now
            };
            
            handleMessageEvent(event);
            stats_.messagesTimedOut++;
        } else {
            tempQueue.push(message);
        }
    }
    
    messageQueue_ = std::move(tempQueue);
}

void MessageHandler::enqueueMessage(const Message& message) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    if (messageQueue_.size() >= config_.maxQueueSize) {
        MessageEvent event{
            MessageEventType::QUEUE_FULL,
            message,
            "Queue is full",
            std::chrono::system_clock::now()
        };
        
        handleMessageEvent(event);
        return;
    }
    
    messageQueue_.push(message);
}

Message MessageHandler::dequeueMessage() {
    Message message = messageQueue_.front();
    messageQueue_.pop();
    return message;
}

void MessageHandler::clearQueue() {
    while (!messageQueue_.empty()) {
        messageQueue_.pop();
    }
}

void MessageHandler::resizeQueue(size_t size) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    while (messageQueue_.size() > size) {
        messageQueue_.pop();
    }
    
    config_.maxQueueSize = size;
}

bool MessageHandler::validateMessageType(MessageType type) const {
    return type >= MessageType::SYSTEM && type <= MessageType::CUSTOM;
}

bool MessageHandler::validateMessagePriority(MessagePriority priority) const {
    return priority >= MessagePriority::LOW && priority <= MessagePriority::CRITICAL;
}

bool MessageHandler::validateMessageReliability(MessageReliability reliability) const {
    return reliability >= MessageReliability::UNRELIABLE && reliability <= MessageReliability::ORDERED;
}

bool MessageHandler::validateMessageSource(const std::string& source) const {
    return !source.empty();
}

bool MessageHandler::validateMessageDestination(const std::string& destination) const {
    return !destination.empty();
}

bool MessageHandler::validateMessageTimestamp(const std::chrono::system_clock::time_point& timestamp) const {
    return timestamp <= std::chrono::system_clock::now();
}

bool MessageHandler::validateMessageSequence(uint32_t sequence) const {
    // Implementation specific sequence validation
    return true;
}

bool MessageHandler::validateMessageOrder(uint32_t order) const {
    return true; // Implementation specific
}

size_t MessageHandler::getMessageSize(const Message& message) const {
    size_t size = 0;
    
    // Add metadata size
    size += sizeof(MessageMetadata);
    
    // Add data size
    std::visit([&size](const auto& data) {
        using T = std::decay_t<decltype(data)>;
        if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
            size += data.size();
        } else if constexpr (std::is_same_v<T, std::string>) {
            size += data.size();
        } else {
            size += sizeof(T);
        }
    }, message.data);
    
    return size;
}

} // namespace BarrenEngine 