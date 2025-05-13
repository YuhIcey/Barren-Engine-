#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include "Crypto.hpp"

namespace BarrenEngine {

enum class SecurityLevel {
    NONE,
    BASIC,      // Basic encryption only
    STANDARD,   // Standard security with certificate validation
    HIGH,       // High security with certificate pinning
    MAXIMUM     // Maximum security with additional anti-cheat measures
};

struct Certificate {
    std::vector<uint8_t> publicKey;
    std::vector<uint8_t> privateKey;
    std::chrono::system_clock::time_point expirationDate;
    std::string issuer;
    std::string subject;
    std::vector<std::string> allowedIPs;
};

struct SecurityConfig {
    SecurityLevel level;
    Crypto::Mode encryptionMode;
    std::string certificatePath;
    bool enableAntiCheat;
    bool enablePacketSigning;
    bool enableCertificatePinning;
    std::vector<std::string> trustedCertificates;
};

class SecurityManager {
public:
    SecurityManager();
    ~SecurityManager();

    bool initialize(const SecurityConfig& config);
    bool validateCertificate(const Certificate& cert);
    bool verifyPacketSignature(const std::vector<uint8_t>& data, const std::vector<uint8_t>& signature);
    std::vector<uint8_t> signPacket(const std::vector<uint8_t>& data);
    bool isIPAllowed(const std::string& ip);
    void updateCertificate(const Certificate& cert);
    bool isAntiCheatEnabled() const { return config_.enableAntiCheat; }
    SecurityLevel getSecurityLevel() const { return config_.level; }

    // Encryption methods
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& data);
    std::vector<uint8_t> generateKey();
    std::vector<uint8_t> generateIV();

private:
    SecurityConfig config_;
    std::unique_ptr<Certificate> currentCertificate_;
    std::unordered_map<std::string, Certificate> trustedCertificates_;
    std::mutex securityMutex_;
    bool initialized_;
    std::vector<uint8_t> encryptionKey_;
    std::vector<uint8_t> currentIV_;

    bool loadCertificate(const std::string& path);
    bool validateCertificateChain(const Certificate& cert);
    bool checkCertificateExpiration(const Certificate& cert);
    bool verifyCertificateSignature(const Certificate& cert);
    void initializeAntiCheat();
    std::vector<uint8_t> deriveKey(const std::vector<uint8_t>& key);
};

} // namespace BarrenEngine 