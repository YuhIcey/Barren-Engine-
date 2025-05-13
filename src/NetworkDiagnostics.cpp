#include "NetworkDiagnostics.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>

namespace BarrenEngine {

NetworkDiagnostics::NetworkDiagnostics()
    : isCapturing_(false)
    , bandwidthLimit_(0)
{
    resetMetrics();
}

NetworkDiagnostics::~NetworkDiagnostics() {
    if (isCapturing_) {
        stopPacketCapture();
    }
}

void NetworkDiagnostics::updateMetrics(const NetworkMetrics& metrics) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    currentMetrics_ = metrics;
    
    // Update metrics history
    metricsHistory_.push_back(metrics);
    if (metricsHistory_.size() > MAX_METRICS_HISTORY) {
        metricsHistory_.erase(metricsHistory_.begin());
    }
    
    // Call metrics callback if set
    if (metricsCallback_) {
        metricsCallback_(metrics);
    }
}

NetworkMetrics NetworkDiagnostics::getCurrentMetrics() const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    return currentMetrics_;
}

void NetworkDiagnostics::resetMetrics() {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    currentMetrics_ = NetworkMetrics{};
    metricsHistory_.clear();
}

void NetworkDiagnostics::setNetworkCondition(const NetworkCondition& condition) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    networkCondition_ = condition;
}

NetworkCondition NetworkDiagnostics::getNetworkCondition() {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    return networkCondition_;
}

void NetworkDiagnostics::disableNetworkCondition() {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    networkCondition_.enabled = false;
}

void NetworkDiagnostics::startPacketCapture(const std::string& filename) {
    if (isCapturing_) {
        stopPacketCapture();
    }
    
    captureFile_.open(filename, std::ios::binary);
    if (captureFile_.is_open()) {
        isCapturing_ = true;
    }
}

void NetworkDiagnostics::stopPacketCapture() {
    if (isCapturing_) {
        captureFile_.close();
        isCapturing_ = false;
    }
}

bool NetworkDiagnostics::isCapturing() const {
    return isCapturing_;
}

void NetworkDiagnostics::setBandwidthLimit(size_t bytesPerSecond) {
    bandwidthLimit_ = bytesPerSecond;
}

size_t NetworkDiagnostics::getBandwidthLimit() const {
    return bandwidthLimit_;
}

size_t NetworkDiagnostics::getCurrentBandwidth() {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    return currentMetrics_.bandwidth;
}

double NetworkDiagnostics::getConnectionQuality() {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    // Calculate connection quality based on various metrics
    double quality = 1.0;
    // Latency impact (0-100ms is good, >500ms is bad)
    quality *= std::max(0.0, 1.0 - (currentMetrics_.latency / 500.0));
    // Packet loss impact
    quality *= (1.0 - currentMetrics_.packetLoss);
    // Jitter impact
    quality *= std::max(0.0, 1.0 - (currentMetrics_.jitter / 100.0));
    return std::max(0.0, std::min(1.0, quality));
}

std::string NetworkDiagnostics::getConnectionQualityString() {
    double quality = getConnectionQuality();
    if (quality > 0.8) return "Excellent";
    if (quality > 0.6) return "Good";
    if (quality > 0.4) return "Fair";
    if (quality > 0.2) return "Poor";
    return "Bad";
}

void NetworkDiagnostics::logError(const std::string& error) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    
    recentErrors_.push(error);
    while (recentErrors_.size() > MAX_ERRORS) {
        recentErrors_.pop();
    }
    
    if (errorCallback_) {
        errorCallback_(error);
    }
}

std::vector<std::string> NetworkDiagnostics::getRecentErrors() {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    std::vector<std::string> errors;
    std::queue<std::string> tempQueue = recentErrors_;
    while (!tempQueue.empty()) {
        errors.push_back(tempQueue.front());
        tempQueue.pop();
    }
    return errors;
}

void NetworkDiagnostics::clearErrors() {
    std::lock_guard<std::mutex> lock(errorMutex_);
    while (!recentErrors_.empty()) {
        recentErrors_.pop();
    }
}

NetworkDiagnostics::Statistics NetworkDiagnostics::getStatistics() const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    Statistics stats{};
    if (metricsHistory_.empty()) {
        return stats;
    }
    
    // Calculate statistics from metrics history
    double totalLatency = 0;
    double totalPacketLoss = 0;
    double totalBandwidth = 0;
    stats.maxLatency = 0;
    stats.minLatency = std::numeric_limits<double>::max();
    stats.maxPacketLoss = 0;
    stats.maxBandwidth = 0;
    
    for (const auto& metrics : metricsHistory_) {
        totalLatency += metrics.latency;
        totalPacketLoss += metrics.packetLoss;
        totalBandwidth += metrics.bandwidth;
        
        stats.maxLatency = std::max(stats.maxLatency, metrics.latency);
        stats.minLatency = std::min(stats.minLatency, metrics.latency);
        stats.maxPacketLoss = std::max(stats.maxPacketLoss, metrics.packetLoss);
        stats.maxBandwidth = std::max(stats.maxBandwidth, metrics.bandwidth);
    }
    
    size_t count = metricsHistory_.size();
    stats.averageLatency = totalLatency / count;
    stats.averagePacketLoss = totalPacketLoss / count;
    stats.averageBandwidth = totalBandwidth / count;
    stats.totalErrors = currentMetrics_.errors;
    
    return stats;
}

void NetworkDiagnostics::setMetricsCallback(MetricsCallback callback) {
    metricsCallback_ = callback;
}

void NetworkDiagnostics::setErrorCallback(std::function<void(const std::string&)> callback) {
    errorCallback_ = callback;
}

void NetworkDiagnostics::applyNetworkCondition(std::vector<uint8_t>& data) {
    if (!networkCondition_.enabled) return;
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 1.0);
    
    // Apply packet loss
    if (dis(gen) < networkCondition_.packetLoss) {
        data.clear();
        return;
    }
    
    // Apply bandwidth limit
    if (networkCondition_.bandwidth > 0) {
        size_t maxBytes = static_cast<size_t>(networkCondition_.bandwidth / 1000.0); // per millisecond
        if (data.size() > maxBytes) {
            data.resize(maxBytes);
        }
    }
    
    // Apply jitter
    if (networkCondition_.jitter > 0) {
        std::normal_distribution<> jitter(0.0, networkCondition_.jitter);
        double delay = jitter(gen);
        if (delay > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(delay)));
        }
    }
}

void NetworkDiagnostics::writePacketToCapture(const std::vector<uint8_t>& data, bool isOutgoing) {
    if (!isCapturing_ || !captureFile_.is_open()) return;
    
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    captureFile_ << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << " "
                 << (isOutgoing ? "OUT" : "IN ") << " "
                 << data.size() << " bytes\n";
    
    // Write first 16 bytes in hex
    for (size_t i = 0; i < std::min(data.size(), size_t(16)); ++i) {
        captureFile_ << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(data[i]) << " ";
    }
    captureFile_ << std::dec << "\n\n";
}

} // namespace BarrenEngine 