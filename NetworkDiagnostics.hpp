#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <atomic>
#include <mutex>
#include <queue>
#include <functional>
#include <memory>
#include <fstream>

namespace BarrenEngine {

struct NetworkMetrics {
    double latency;           // Current latency in milliseconds
    double packetLoss;        // Packet loss percentage
    double bandwidth;         // Current bandwidth in bytes per second
    double jitter;           // Jitter in milliseconds
    size_t bytesSent;        // Total bytes sent
    size_t bytesReceived;    // Total bytes received
    size_t packetsSent;      // Total packets sent
    size_t packetsReceived;  // Total packets received
    size_t errors;           // Total errors encountered
};

struct NetworkCondition {
    double latency;          // Simulated latency in milliseconds
    double packetLoss;       // Simulated packet loss percentage
    double bandwidth;        // Simulated bandwidth limit in bytes per second
    double jitter;          // Simulated jitter in milliseconds
    bool enabled;           // Whether simulation is enabled
};

class NetworkDiagnostics {
public:
    NetworkDiagnostics();
    ~NetworkDiagnostics();

    // Basic metrics
    void updateMetrics(const NetworkMetrics& metrics);
    NetworkMetrics getCurrentMetrics() const;
    void resetMetrics();

    // Network condition simulation
    void setNetworkCondition(const NetworkCondition& condition);
    NetworkCondition getNetworkCondition();
    void disableNetworkCondition();

    // Packet capture
    void startPacketCapture(const std::string& filename);
    void stopPacketCapture();
    bool isCapturing() const;

    // Bandwidth monitoring
    void setBandwidthLimit(size_t bytesPerSecond);
    size_t getBandwidthLimit() const;
    size_t getCurrentBandwidth();

    // Connection quality
    double getConnectionQuality();
    std::string getConnectionQualityString();

    // Error tracking
    void logError(const std::string& error);
    std::vector<std::string> getRecentErrors();
    void clearErrors();

    // Statistics
    struct Statistics {
        double averageLatency;
        double maxLatency;
        double minLatency;
        double averagePacketLoss;
        double maxPacketLoss;
        double averageBandwidth;
        double maxBandwidth;
        size_t totalErrors;
    };
    Statistics getStatistics() const;

    // Callbacks
    using MetricsCallback = std::function<void(const NetworkMetrics&)>;
    void setMetricsCallback(MetricsCallback callback);
    void setErrorCallback(std::function<void(const std::string&)> callback);

private:
    NetworkMetrics currentMetrics_;
    NetworkCondition networkCondition_;
    std::atomic<bool> isCapturing_;
    std::atomic<size_t> bandwidthLimit_;
    std::queue<std::string> recentErrors_;
    mutable std::mutex metricsMutex_;
    mutable std::mutex errorMutex_;
    MetricsCallback metricsCallback_;
    std::function<void(const std::string&)> errorCallback_;
    std::ofstream captureFile_;
    std::vector<NetworkMetrics> metricsHistory_;
    static const size_t MAX_ERRORS = 100;
    static const size_t MAX_METRICS_HISTORY = 1000;

    void applyNetworkCondition(std::vector<uint8_t>& data);
    void updateStatistics(const NetworkMetrics& metrics);
    void writePacketToCapture(const std::vector<uint8_t>& data, bool isOutgoing);
};

} // namespace BarrenEngine 