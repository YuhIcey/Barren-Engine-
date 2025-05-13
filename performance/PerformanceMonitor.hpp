#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <queue>
#include <atomic>

namespace BarrenEngine {

// Performance metrics
struct PerformanceMetrics {
    // CPU metrics
    double cpuUsage;
    uint32_t threadCount;
    uint64_t contextSwitches;
    
    // Memory metrics
    uint64_t memoryUsage;
    uint64_t peakMemoryUsage;
    uint32_t allocationCount;
    uint32_t deallocationCount;
    
    // Network metrics
    uint64_t bytesSent;
    uint64_t bytesReceived;
    uint32_t packetLoss;
    uint32_t latency;
    uint32_t bandwidth;
    
    // Timing metrics
    std::chrono::nanoseconds frameTime;
    std::chrono::nanoseconds updateTime;
    std::chrono::nanoseconds renderTime;
    std::chrono::nanoseconds networkTime;
    
    // Custom metrics
    std::unordered_map<std::string, double> customMetrics;
};

// Performance thresholds
struct PerformanceThresholds {
    double maxCpuUsage;
    uint64_t maxMemoryUsage;
    uint32_t maxPacketLoss;
    uint32_t maxLatency;
    std::chrono::nanoseconds maxFrameTime;
    std::chrono::nanoseconds maxUpdateTime;
    std::chrono::nanoseconds maxRenderTime;
    std::chrono::nanoseconds maxNetworkTime;
};

// Performance event types
enum class PerformanceEventType {
    THRESHOLD_EXCEEDED,
    PERFORMANCE_DEGRADATION,
    MEMORY_LEAK_DETECTED,
    CPU_BOTTLENECK,
    NETWORK_CONGESTION,
    CUSTOM_EVENT
};

// Performance event
struct PerformanceEvent {
    PerformanceEventType type;
    std::string component;
    std::string message;
    PerformanceMetrics metrics;
    std::chrono::system_clock::time_point timestamp;
};

// Performance callback types
using PerformanceEventCallback = std::function<void(const PerformanceEvent&)>;
using MetricsCallback = std::function<void(const PerformanceMetrics&)>;
using ThresholdCallback = std::function<void(const std::string&, double)>;

class PerformanceMonitor {
public:
    PerformanceMonitor();
    ~PerformanceMonitor();

    // Initialization
    bool initialize();
    void start();
    void stop();
    bool isRunning() const;

    // Metrics collection
    void updateMetrics();
    PerformanceMetrics getMetrics() const;
    void resetMetrics();
    void addCustomMetric(const std::string& name, double value);
    void removeCustomMetric(const std::string& name);

    // Threshold management
    void setThresholds(const PerformanceThresholds& thresholds);
    PerformanceThresholds getThresholds() const;
    void setCustomThreshold(const std::string& metric, double threshold);
    void removeCustomThreshold(const std::string& metric);

    // Event handling
    void setPerformanceEventCallback(PerformanceEventCallback callback);
    void setMetricsCallback(MetricsCallback callback);
    void setThresholdCallback(ThresholdCallback callback);

    // Performance optimization
    void enableOptimization(bool enable);
    void setOptimizationLevel(int level);
    void addOptimizationRule(const std::string& component, std::function<void()> rule);
    void removeOptimizationRule(const std::string& component);

    // Monitoring
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;
    void setMonitoringInterval(uint32_t intervalMs);

    // Analysis
    void analyzePerformance();
    std::string generateReport() const;
    void exportMetrics(const std::string& filename) const;
    void importMetrics(const std::string& filename);

private:
    // Internal metrics collection
    void collectCpuMetrics();
    void collectMemoryMetrics();
    void collectNetworkMetrics();
    void collectTimingMetrics();
    void collectCustomMetrics();

    // Threshold checking
    void checkThresholds();
    bool checkCpuThresholds();
    bool checkMemoryThresholds();
    bool checkNetworkThresholds();
    bool checkTimingThresholds();
    bool checkCustomThresholds();

    // Event handling
    void handleThresholdExceeded(const std::string& metric, double value);
    void handlePerformanceDegradation(const std::string& component);
    void handleMemoryLeak(const std::string& component);
    void handleCpuBottleneck(const std::string& component);
    void handleNetworkCongestion(const std::string& component);
    void handleCustomEvent(const std::string& component, const std::string& message);

    // Optimization
    void applyOptimizations();
    void optimizeCpu();
    void optimizeMemory();
    void optimizeNetwork();
    void optimizeTiming();

    // Analysis
    void analyzeCpuUsage();
    void analyzeMemoryUsage();
    void analyzeNetworkPerformance();
    void analyzeTimingPerformance();
    void generateCpuReport(std::stringstream& ss) const;
    void generateMemoryReport(std::stringstream& ss) const;
    void generateNetworkReport(std::stringstream& ss) const;
    void generateTimingReport(std::stringstream& ss) const;
    void generateCustomReport(std::stringstream& ss) const;

    // Member variables
    std::atomic<bool> running_;
    std::atomic<bool> monitoring_;
    std::atomic<bool> optimizationEnabled_;
    int optimizationLevel_;
    uint32_t monitoringInterval_;
    mutable std::mutex metricsMutex_;
    mutable std::mutex thresholdsMutex_;
    mutable std::mutex rulesMutex_;
    PerformanceMetrics metrics_;
    PerformanceThresholds thresholds_;
    std::unordered_map<std::string, double> customThresholds_;
    std::unordered_map<std::string, std::function<void()>> optimizationRules_;
    std::queue<PerformanceEvent> eventQueue_;
    PerformanceEventCallback performanceEventCallback_;
    MetricsCallback metricsCallback_;
    ThresholdCallback thresholdCallback_;
    std::chrono::system_clock::time_point lastUpdate_;
    std::chrono::system_clock::time_point lastMonitoring_;
};

} // namespace BarrenEngine 