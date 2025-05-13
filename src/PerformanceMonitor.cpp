#include "performance/PerformanceMonitor.hpp"
#include <thread>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <json/json.h>

namespace BarrenEngine {

PerformanceMonitor::PerformanceMonitor()
    : running_(false)
    , monitoring_(false)
    , optimizationEnabled_(false)
    , optimizationLevel_(0)
    , monitoringInterval_(1000) // Default 1 second
{
    resetMetrics();
}

PerformanceMonitor::~PerformanceMonitor() {
    stop();
}

bool PerformanceMonitor::initialize() {
    if (running_) return true;
    
    // Initialize performance monitoring
    running_ = true;
    return true;
}

void PerformanceMonitor::start() {
    if (!running_) return;
    
    monitoring_ = true;
    lastMonitoring_ = std::chrono::system_clock::now();
}

void PerformanceMonitor::stop() {
    if (!running_) return;
    
    running_ = false;
    monitoring_ = false;
    optimizationEnabled_ = false;
}

bool PerformanceMonitor::isRunning() const {
    return running_;
}

void PerformanceMonitor::updateMetrics() {
    if (!running_) return;
    
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    collectCpuMetrics();
    collectMemoryMetrics();
    collectNetworkMetrics();
    collectTimingMetrics();
    collectCustomMetrics();
    
    lastUpdate_ = std::chrono::system_clock::now();
    
    if (metricsCallback_) {
        metricsCallback_(metrics_);
    }
}

PerformanceMetrics PerformanceMonitor::getMetrics() const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    return metrics_;
}

void PerformanceMonitor::resetMetrics() {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    metrics_ = PerformanceMetrics{};
}

void PerformanceMonitor::addCustomMetric(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    metrics_.customMetrics[name] = value;
}

void PerformanceMonitor::removeCustomMetric(const std::string& name) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    metrics_.customMetrics.erase(name);
}

void PerformanceMonitor::setThresholds(const PerformanceThresholds& thresholds) {
    std::lock_guard<std::mutex> lock(thresholdsMutex_);
    thresholds_ = thresholds;
}

PerformanceThresholds PerformanceMonitor::getThresholds() const {
    std::lock_guard<std::mutex> lock(thresholdsMutex_);
    return thresholds_;
}

void PerformanceMonitor::setCustomThreshold(const std::string& metric, double threshold) {
    std::lock_guard<std::mutex> lock(thresholdsMutex_);
    customThresholds_[metric] = threshold;
}

void PerformanceMonitor::removeCustomThreshold(const std::string& metric) {
    std::lock_guard<std::mutex> lock(thresholdsMutex_);
    customThresholds_.erase(metric);
}

void PerformanceMonitor::setPerformanceEventCallback(PerformanceEventCallback callback) {
    performanceEventCallback_ = callback;
}

void PerformanceMonitor::setMetricsCallback(MetricsCallback callback) {
    metricsCallback_ = callback;
}

void PerformanceMonitor::setThresholdCallback(ThresholdCallback callback) {
    thresholdCallback_ = callback;
}

void PerformanceMonitor::enableOptimization(bool enable) {
    optimizationEnabled_ = enable;
}

void PerformanceMonitor::setOptimizationLevel(int level) {
    optimizationLevel_ = level;
}

void PerformanceMonitor::addOptimizationRule(const std::string& component, std::function<void()> rule) {
    std::lock_guard<std::mutex> lock(rulesMutex_);
    optimizationRules_[component] = rule;
}

void PerformanceMonitor::removeOptimizationRule(const std::string& component) {
    std::lock_guard<std::mutex> lock(rulesMutex_);
    optimizationRules_.erase(component);
}

void PerformanceMonitor::startMonitoring() {
    if (!running_) return;
    
    monitoring_ = true;
    lastMonitoring_ = std::chrono::system_clock::now();
}

void PerformanceMonitor::stopMonitoring() {
    monitoring_ = false;
}

bool PerformanceMonitor::isMonitoring() const {
    return monitoring_;
}

void PerformanceMonitor::setMonitoringInterval(uint32_t intervalMs) {
    monitoringInterval_ = intervalMs;
}

void PerformanceMonitor::analyzePerformance() {
    if (!running_) return;
    
    analyzeCpuUsage();
    analyzeMemoryUsage();
    analyzeNetworkPerformance();
    analyzeTimingPerformance();
    
    if (optimizationEnabled_) {
        applyOptimizations();
    }
}

std::string PerformanceMonitor::generateReport() const {
    std::stringstream ss;
    
    generateCpuReport(ss);
    generateMemoryReport(ss);
    generateNetworkReport(ss);
    generateTimingReport(ss);
    generateCustomReport(ss);
    
    return ss.str();
}

void PerformanceMonitor::exportMetrics(const std::string& filename) const {
    Json::Value root;
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    // Export CPU metrics
    root["cpu"]["usage"] = metrics_.cpuUsage;
    root["cpu"]["threadCount"] = metrics_.threadCount;
    root["cpu"]["contextSwitches"] = Json::Value::UInt64(metrics_.contextSwitches);
    
    // Export memory metrics
    root["memory"]["usage"] = Json::Value::UInt64(metrics_.memoryUsage);
    root["memory"]["peakUsage"] = Json::Value::UInt64(metrics_.peakMemoryUsage);
    root["memory"]["allocationCount"] = metrics_.allocationCount;
    root["memory"]["deallocationCount"] = metrics_.deallocationCount;
    
    // Export network metrics
    root["network"]["bytesSent"] = Json::Value::UInt64(metrics_.bytesSent);
    root["network"]["bytesReceived"] = Json::Value::UInt64(metrics_.bytesReceived);
    root["network"]["packetLoss"] = metrics_.packetLoss;
    root["network"]["latency"] = metrics_.latency;
    root["network"]["bandwidth"] = metrics_.bandwidth;
    
    // Export timing metrics
    root["timing"]["frameTime"] = metrics_.frameTime.count();
    root["timing"]["updateTime"] = metrics_.updateTime.count();
    root["timing"]["renderTime"] = metrics_.renderTime.count();
    root["timing"]["networkTime"] = metrics_.networkTime.count();
    
    // Export custom metrics
    for (const auto& [name, value] : metrics_.customMetrics) {
        root["custom"][name] = value;
    }
    
    // Write to file
    std::ofstream file(filename);
    Json::StyledWriter writer;
    file << writer.write(root);
}

void PerformanceMonitor::importMetrics(const std::string& filename) {
    Json::Value root;
    std::ifstream file(filename);
    Json::Reader reader;
    
    if (reader.parse(file, root)) {
        std::lock_guard<std::mutex> lock(metricsMutex_);
        
        // Import CPU metrics
        metrics_.cpuUsage = root["cpu"]["usage"].asDouble();
        metrics_.threadCount = root["cpu"]["threadCount"].asUInt();
        metrics_.contextSwitches = root["cpu"]["contextSwitches"].asUInt64();
        
        // Import memory metrics
        metrics_.memoryUsage = root["memory"]["usage"].asUInt64();
        metrics_.peakMemoryUsage = root["memory"]["peakUsage"].asUInt64();
        metrics_.allocationCount = root["memory"]["allocationCount"].asUInt();
        metrics_.deallocationCount = root["memory"]["deallocationCount"].asUInt();
        
        // Import network metrics
        metrics_.bytesSent = root["network"]["bytesSent"].asUInt64();
        metrics_.bytesReceived = root["network"]["bytesReceived"].asUInt64();
        metrics_.packetLoss = root["network"]["packetLoss"].asUInt();
        metrics_.latency = root["network"]["latency"].asUInt();
        metrics_.bandwidth = root["network"]["bandwidth"].asUInt();
        
        // Import timing metrics
        metrics_.frameTime = std::chrono::nanoseconds(root["timing"]["frameTime"].asInt64());
        metrics_.updateTime = std::chrono::nanoseconds(root["timing"]["updateTime"].asInt64());
        metrics_.renderTime = std::chrono::nanoseconds(root["timing"]["renderTime"].asInt64());
        metrics_.networkTime = std::chrono::nanoseconds(root["timing"]["networkTime"].asInt64());
        
        // Import custom metrics
        metrics_.customMetrics.clear();
        for (const auto& name : root["custom"].getMemberNames()) {
            metrics_.customMetrics[name] = root["custom"][name].asDouble();
        }
    }
}

void PerformanceMonitor::collectCpuMetrics() {
    // Implementation specific CPU metrics collection
    metrics_.cpuUsage = 0.0;
    metrics_.threadCount = std::thread::hardware_concurrency();
    metrics_.contextSwitches = 0;
}

void PerformanceMonitor::collectMemoryMetrics() {
    // Implementation specific memory metrics collection
    metrics_.memoryUsage = 0;
    metrics_.peakMemoryUsage = 0;
    metrics_.allocationCount = 0;
    metrics_.deallocationCount = 0;
}

void PerformanceMonitor::collectNetworkMetrics() {
    // Implementation specific network metrics collection
    metrics_.bytesSent = 0;
    metrics_.bytesReceived = 0;
    metrics_.packetLoss = 0;
    metrics_.latency = 0;
    metrics_.bandwidth = 0;
}

void PerformanceMonitor::collectTimingMetrics() {
    // Implementation specific timing metrics collection
    metrics_.frameTime = std::chrono::nanoseconds(0);
    metrics_.updateTime = std::chrono::nanoseconds(0);
    metrics_.renderTime = std::chrono::nanoseconds(0);
    metrics_.networkTime = std::chrono::nanoseconds(0);
}

void PerformanceMonitor::collectCustomMetrics() {
    // Custom metrics are managed externally
}

void PerformanceMonitor::checkThresholds() {
    if (!running_ || !monitoring_) return;
    
    auto now = std::chrono::system_clock::now();
    if (now - lastMonitoring_ < std::chrono::milliseconds(monitoringInterval_)) {
        return;
    }
    
    lastMonitoring_ = now;
    
    bool thresholdsExceeded = false;
    thresholdsExceeded |= checkCpuThresholds();
    thresholdsExceeded |= checkMemoryThresholds();
    thresholdsExceeded |= checkNetworkThresholds();
    thresholdsExceeded |= checkTimingThresholds();
    thresholdsExceeded |= checkCustomThresholds();
    
    if (thresholdsExceeded && optimizationEnabled_) {
        applyOptimizations();
    }
}

bool PerformanceMonitor::checkCpuThresholds() {
    std::lock_guard<std::mutex> lock(thresholdsMutex_);
    if (metrics_.cpuUsage > thresholds_.maxCpuUsage) {
        handleThresholdExceeded("CPU Usage", metrics_.cpuUsage);
        return true;
    }
    return false;
}

bool PerformanceMonitor::checkMemoryThresholds() {
    std::lock_guard<std::mutex> lock(thresholdsMutex_);
    if (metrics_.memoryUsage > thresholds_.maxMemoryUsage) {
        handleThresholdExceeded("Memory Usage", metrics_.memoryUsage);
        return true;
    }
    return false;
}

bool PerformanceMonitor::checkNetworkThresholds() {
    std::lock_guard<std::mutex> lock(thresholdsMutex_);
    if (metrics_.packetLoss > thresholds_.maxPacketLoss) {
        handleThresholdExceeded("Packet Loss", metrics_.packetLoss);
        return true;
    }
    if (metrics_.latency > thresholds_.maxLatency) {
        handleThresholdExceeded("Latency", metrics_.latency);
        return true;
    }
    return false;
}

bool PerformanceMonitor::checkTimingThresholds() {
    std::lock_guard<std::mutex> lock(thresholdsMutex_);
    if (metrics_.frameTime > thresholds_.maxFrameTime) {
        handleThresholdExceeded("Frame Time", metrics_.frameTime.count());
        return true;
    }
    if (metrics_.updateTime > thresholds_.maxUpdateTime) {
        handleThresholdExceeded("Update Time", metrics_.updateTime.count());
        return true;
    }
    if (metrics_.renderTime > thresholds_.maxRenderTime) {
        handleThresholdExceeded("Render Time", metrics_.renderTime.count());
        return true;
    }
    if (metrics_.networkTime > thresholds_.maxNetworkTime) {
        handleThresholdExceeded("Network Time", metrics_.networkTime.count());
        return true;
    }
    return false;
}

bool PerformanceMonitor::checkCustomThresholds() {
    std::lock_guard<std::mutex> lock(thresholdsMutex_);
    bool exceeded = false;
    
    for (const auto& [metric, threshold] : customThresholds_) {
        if (metrics_.customMetrics.count(metric) > 0) {
            if (metrics_.customMetrics[metric] > threshold) {
                handleThresholdExceeded(metric, metrics_.customMetrics[metric]);
                exceeded = true;
            }
        }
    }
    
    return exceeded;
}

void PerformanceMonitor::handleThresholdExceeded(const std::string& metric, double value) {
    PerformanceEvent event{
        PerformanceEventType::THRESHOLD_EXCEEDED,
        metric,
        "Threshold exceeded: " + std::to_string(value),
        metrics_,
        std::chrono::system_clock::now()
    };
    
    eventQueue_.push(event);
    
    if (performanceEventCallback_) {
        performanceEventCallback_(event);
    }
    
    if (thresholdCallback_) {
        thresholdCallback_(metric, value);
    }
}

void PerformanceMonitor::handlePerformanceDegradation(const std::string& component) {
    PerformanceEvent event{
        PerformanceEventType::PERFORMANCE_DEGRADATION,
        component,
        "Performance degradation detected",
        metrics_,
        std::chrono::system_clock::now()
    };
    
    eventQueue_.push(event);
    
    if (performanceEventCallback_) {
        performanceEventCallback_(event);
    }
}

void PerformanceMonitor::handleMemoryLeak(const std::string& component) {
    PerformanceEvent event{
        PerformanceEventType::MEMORY_LEAK_DETECTED,
        component,
        "Memory leak detected",
        metrics_,
        std::chrono::system_clock::now()
    };
    
    eventQueue_.push(event);
    
    if (performanceEventCallback_) {
        performanceEventCallback_(event);
    }
}

void PerformanceMonitor::handleCpuBottleneck(const std::string& component) {
    PerformanceEvent event{
        PerformanceEventType::CPU_BOTTLENECK,
        component,
        "CPU bottleneck detected",
        metrics_,
        std::chrono::system_clock::now()
    };
    
    eventQueue_.push(event);
    
    if (performanceEventCallback_) {
        performanceEventCallback_(event);
    }
}

void PerformanceMonitor::handleNetworkCongestion(const std::string& component) {
    PerformanceEvent event{
        PerformanceEventType::NETWORK_CONGESTION,
        component,
        "Network congestion detected",
        metrics_,
        std::chrono::system_clock::now()
    };
    
    eventQueue_.push(event);
    
    if (performanceEventCallback_) {
        performanceEventCallback_(event);
    }
}

void PerformanceMonitor::handleCustomEvent(const std::string& component, const std::string& message) {
    PerformanceEvent event{
        PerformanceEventType::CUSTOM_EVENT,
        component,
        message,
        metrics_,
        std::chrono::system_clock::now()
    };
    
    eventQueue_.push(event);
    
    if (performanceEventCallback_) {
        performanceEventCallback_(event);
    }
}

void PerformanceMonitor::applyOptimizations() {
    if (!optimizationEnabled_) return;
    
    std::lock_guard<std::mutex> lock(rulesMutex_);
    
    for (const auto& [component, rule] : optimizationRules_) {
        rule();
    }
}

void PerformanceMonitor::optimizeCpu() {
    // Implementation specific CPU optimization
}

void PerformanceMonitor::optimizeMemory() {
    // Implementation specific memory optimization
}

void PerformanceMonitor::optimizeNetwork() {
    // Implementation specific network optimization
}

void PerformanceMonitor::optimizeTiming() {
    // Implementation specific timing optimization
}

void PerformanceMonitor::analyzeCpuUsage() {
    // Implementation specific CPU analysis
}

void PerformanceMonitor::analyzeMemoryUsage() {
    // Implementation specific memory analysis
}

void PerformanceMonitor::analyzeNetworkPerformance() {
    // Implementation specific network analysis
}

void PerformanceMonitor::analyzeTimingPerformance() {
    // Implementation specific timing analysis
}

void PerformanceMonitor::generateCpuReport(std::stringstream& ss) const {
    ss << "CPU Metrics:\n";
    ss << "  Usage: " << std::fixed << std::setprecision(2) << metrics_.cpuUsage << "%\n";
    ss << "  Thread Count: " << metrics_.threadCount << "\n";
    ss << "  Context Switches: " << metrics_.contextSwitches << "\n\n";
}

void PerformanceMonitor::generateMemoryReport(std::stringstream& ss) const {
    ss << "Memory Metrics:\n";
    ss << "  Usage: " << metrics_.memoryUsage << " bytes\n";
    ss << "  Peak Usage: " << metrics_.peakMemoryUsage << " bytes\n";
    ss << "  Allocations: " << metrics_.allocationCount << "\n";
    ss << "  Deallocations: " << metrics_.deallocationCount << "\n\n";
}

void PerformanceMonitor::generateNetworkReport(std::stringstream& ss) const {
    ss << "Network Metrics:\n";
    ss << "  Bytes Sent: " << metrics_.bytesSent << "\n";
    ss << "  Bytes Received: " << metrics_.bytesReceived << "\n";
    ss << "  Packet Loss: " << metrics_.packetLoss << "%\n";
    ss << "  Latency: " << metrics_.latency << " ms\n";
    ss << "  Bandwidth: " << metrics_.bandwidth << " bps\n\n";
}

void PerformanceMonitor::generateTimingReport(std::stringstream& ss) const {
    ss << "Timing Metrics:\n";
    ss << "  Frame Time: " << metrics_.frameTime.count() << " ns\n";
    ss << "  Update Time: " << metrics_.updateTime.count() << " ns\n";
    ss << "  Render Time: " << metrics_.renderTime.count() << " ns\n";
    ss << "  Network Time: " << metrics_.networkTime.count() << " ns\n\n";
}

void PerformanceMonitor::generateCustomReport(std::stringstream& ss) const {
    if (!metrics_.customMetrics.empty()) {
        ss << "Custom Metrics:\n";
        for (const auto& [name, value] : metrics_.customMetrics) {
            ss << "  " << name << ": " << value << "\n";
        }
        ss << "\n";
    }
}

} // namespace BarrenEngine 