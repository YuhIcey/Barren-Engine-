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
#include <variant>

namespace BarrenEngine {

// Message types
enum class MessageType {
    SYSTEM,
    USER,
    COMMAND,
    EVENT,
    DATA,
    CONTROL,
    CUSTOM
};

// Message priority
enum class MessagePriority {
    LOW,
    NORMAL,
    HIGH,
    CRITICAL
};

// Message reliability
enum class MessageReliability {
    UNRELIABLE,
    RELIABLE,
    SEQUENCED,
    ORDERED
};

// Message metadata
struct MessageMetadata {
    MessageType type;
    MessagePriority priority;
    MessageReliability reliability;
    std::string source;
    std::string destination;
    std::chrono::system_clock::time_point timestamp;
    uint32_t sequenceNumber;
    uint32_t orderNumber;
    bool requiresAck;
    uint32_t retryCount;
    uint32_t maxRetries;
    std::chrono::milliseconds timeout;
};

// Message data types
using MessageData = std::variant<
    std::vector<uint8_t>,
    std::string,
    int32_t,
    uint32_t,
    int64_t,
    uint64_t,
    float,
    double,
    bool
>;

// Message structure
struct Message {
    MessageMetadata metadata;
    MessageData data;
};

// Message queue configuration
struct MessageQueueConfig {
    size_t maxQueueSize;
    size_t maxMessageSize;
    std::chrono::milliseconds processingInterval;
    bool enableCompression;
    bool enableEncryption;
    bool enableValidation;
    bool enableLogging;
};

// Message statistics
struct MessageStats {
    uint64_t messagesProcessed;
    uint64_t messagesDropped;
    uint64_t messagesRetried;
    uint64_t messagesTimedOut;
    uint64_t bytesProcessed;
    uint64_t bytesDropped;
    uint32_t averageProcessingTime;
    uint32_t queueSize;
    uint32_t queueCapacity;
    std::chrono::system_clock::time_point lastProcessed;
};

// Message event types
enum class MessageEventType {
    MESSAGE_RECEIVED,
    MESSAGE_SENT,
    MESSAGE_PROCESSED,
    MESSAGE_DROPPED,
    MESSAGE_RETRIED,
    MESSAGE_TIMED_OUT,
    QUEUE_FULL,
    QUEUE_EMPTY,
    ERROR
};

// Message event
struct MessageEvent {
    MessageEventType type;
    Message message;
    std::string error;
    std::chrono::system_clock::time_point timestamp;
};

// Message callback types
using MessageCallback = std::function<void(const Message&)>;
using MessageEventCallback = std::function<void(const MessageEvent&)>;
using MessageFilter = std::function<bool(const Message&)>;

class MessageHandler {
public:
    MessageHandler();
    ~MessageHandler();

    // Initialization
    bool initialize(const MessageQueueConfig& config);
    void start();
    void stop();
    bool isRunning() const;

    // Message handling
    bool send(const Message& message);
    bool broadcast(const Message& message);
    bool process();
    void clear();

    // Message registration
    void registerCallback(MessageType type, MessageCallback callback);
    void unregisterCallback(MessageType type);
    void registerEventCallback(MessageEventCallback callback);
    void registerFilter(MessageFilter filter);

    // Queue management
    size_t getQueueSize();
    size_t getQueueCapacity() const;
    bool isQueueFull();
    bool isQueueEmpty();
    void setQueueSize(size_t size);
    void setProcessingInterval(std::chrono::milliseconds interval);

    // Configuration
    void enableCompression(bool enable);
    void enableEncryption(bool enable);
    void enableValidation(bool enable);
    void enableLogging(bool enable);

    // Statistics
    MessageStats getStats();
    void resetStats();
    void setStatsCallback(std::function<void(const MessageStats&)> callback);

    // Message validation
    bool validateMessage(const Message& message);
    bool validateMetadata(const MessageMetadata& metadata) const;
    bool validateData(const MessageData& data);

    // Message processing
    void processMessage(const Message& message);
    void handleMessageEvent(const MessageEvent& event);
    void retryMessage(const Message& message);
    void dropMessage(const Message& message);

    size_t getMessageSize(const Message& message) const;

private:
    // Internal message handling
    void processQueue();
    void processMessageInternal(const Message& message);
    void handleMessageEventInternal(const MessageEvent& event);
    void updateStats(const Message& message, bool processed);
    void checkTimeouts();

    // Message queue management
    void enqueueMessage(const Message& message);
    Message dequeueMessage();
    void clearQueue();
    void resizeQueue(size_t size);

    // Message validation
    bool validateMessageType(MessageType type) const;
    bool validateMessagePriority(MessagePriority priority) const;
    bool validateMessageReliability(MessageReliability reliability) const;
    bool validateMessageSource(const std::string& source) const;
    bool validateMessageDestination(const std::string& destination) const;
    bool validateMessageTimestamp(const std::chrono::system_clock::time_point& timestamp) const;
    bool validateMessageSequence(uint32_t sequence) const;
    bool validateMessageOrder(uint32_t order) const;

    // Member variables
    std::atomic<bool> running_;
    MessageQueueConfig config_;
    std::mutex queueMutex_;
    std::mutex statsMutex_;
    std::queue<Message> messageQueue_;
    std::unordered_map<MessageType, MessageCallback> callbacks_;
    MessageEventCallback eventCallback_;
    MessageFilter messageFilter_;
    std::function<void(const MessageStats&)> statsCallback_;
    MessageStats stats_;
    std::chrono::system_clock::time_point lastProcessed_;
    std::chrono::system_clock::time_point lastTimeoutCheck_;
};

} // namespace BarrenEngine 