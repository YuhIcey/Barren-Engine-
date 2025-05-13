#include "Security.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <random>

namespace BarrenEngine {

SecurityManager::SecurityManager() : initialized_(false) {
    // Initialize with default values
    encryptionKey_ = generateKey();
    currentIV_ = generateIV();
}

SecurityManager::~SecurityManager() {
    // Clear sensitive data
    std::fill(encryptionKey_.begin(), encryptionKey_.end(), 0);
    std::fill(currentIV_.begin(), currentIV_.end(), 0);
}

bool SecurityManager::initialize(const SecurityConfig& config) {
    std::lock_guard<std::mutex> lock(securityMutex_);
    
    config_ = config;
    
    if (config.level != SecurityLevel::NONE) {
        if (!loadCertificate(config.certificatePath)) {
            return false;
        }
        
        if (config.enableAntiCheat) {
            initializeAntiCheat();
        }
    }
    
    initialized_ = true;
    return true;
}

bool SecurityManager::validateCertificate(const Certificate& cert) {
    std::lock_guard<std::mutex> lock(securityMutex_);
    
    if (!initialized_) return false;
    
    // Check certificate expiration
    if (!checkCertificateExpiration(cert)) {
        return false;
    }
    
    // Verify certificate signature
    if (!verifyCertificateSignature(cert)) {
        return false;
    }
    
    // Validate certificate chain
    if (config_.level >= SecurityLevel::STANDARD) {
        if (!validateCertificateChain(cert)) {
            return false;
        }
    }
    
    return true;
}

bool SecurityManager::verifyPacketSignature(const std::vector<uint8_t>& data, 
                                          const std::vector<uint8_t>& signature) {
    if (!initialized_ || !config_.enablePacketSigning) return false;
    
    // Use our custom Crypto class for signature verification
    try {
        // Generate a hash of the data
        auto hash = Crypto::hash(data);
        
        // Verify the signature using the public key
        return Crypto::verify(hash, signature, currentCertificate_->publicKey);
    } catch (const std::exception& e) {
        return false;
    }
}

std::vector<uint8_t> SecurityManager::signPacket(const std::vector<uint8_t>& data) {
    if (!initialized_ || !config_.enablePacketSigning) return {};
    
    try {
        // Generate a hash of the data
        auto hash = Crypto::hash(data);
        
        // Sign the hash using the private key
        return Crypto::sign(hash, currentCertificate_->privateKey);
    } catch (const std::exception& e) {
        return {};
    }
}

bool SecurityManager::isIPAllowed(const std::string& ip) {
    if (!initialized_ || !currentCertificate_) return false;
    
    return std::find(currentCertificate_->allowedIPs.begin(),
                    currentCertificate_->allowedIPs.end(),
                    ip) != currentCertificate_->allowedIPs.end();
}

void SecurityManager::updateCertificate(const Certificate& cert) {
    std::lock_guard<std::mutex> lock(securityMutex_);
    
    if (validateCertificate(cert)) {
        currentCertificate_ = std::make_unique<Certificate>(cert);
    }
}

bool SecurityManager::loadCertificate(const std::string& path) {
    std::ifstream certFile(path, std::ios::binary);
    if (!certFile.is_open()) {
        return false;
    }
    
    // Read certificate data
    std::vector<uint8_t> certData((std::istreambuf_iterator<char>(certFile)),
                                 std::istreambuf_iterator<char>());
    
    // Parse certificate
    Certificate cert;
    cert.publicKey = std::vector<uint8_t>(certData.begin(), certData.begin() + 32);
    cert.privateKey = std::vector<uint8_t>(certData.begin() + 32, certData.begin() + 64);
    cert.expirationDate = std::chrono::system_clock::now() + std::chrono::hours(24 * 365);
    
    currentCertificate_ = std::make_unique<Certificate>(cert);
    return true;
}

bool SecurityManager::validateCertificateChain(const Certificate& cert) {
    // Implement certificate chain validation using our custom Crypto class
    try {
        // Verify the certificate's signature using the issuer's public key
        auto certHash = Crypto::hash(cert.publicKey);
        return Crypto::verify(certHash, cert.privateKey, cert.publicKey);
    } catch (const std::exception& e) {
        return false;
    }
}

bool SecurityManager::checkCertificateExpiration(const Certificate& cert) {
    return cert.expirationDate > std::chrono::system_clock::now();
}

bool SecurityManager::verifyCertificateSignature(const Certificate& cert) {
    try {
        // Generate a hash of the certificate data
        auto certHash = Crypto::hash(cert.publicKey);
        
        // Verify the signature using the issuer's public key
        return Crypto::verify(certHash, cert.privateKey, cert.publicKey);
    } catch (const std::exception& e) {
        return false;
    }
}

void SecurityManager::initializeAntiCheat() {
    // Initialize anti-cheat system
    // This could include:
    // - Memory scanning
    // - Process monitoring
    // - Network traffic analysis
    // - Client integrity checks
}

std::vector<uint8_t> SecurityManager::encrypt(const std::vector<uint8_t>& data) {
    if (!initialized_) return {};
    
    try {
        return Crypto::encrypt(data, encryptionKey_, currentIV_, config_.encryptionMode);
    } catch (const std::exception& e) {
        return {};
    }
}

std::vector<uint8_t> SecurityManager::decrypt(const std::vector<uint8_t>& data) {
    if (!initialized_) return {};
    
    try {
        return Crypto::decrypt(data, encryptionKey_, currentIV_, config_.encryptionMode);
    } catch (const std::exception& e) {
        return {};
    }
}

std::vector<uint8_t> SecurityManager::generateKey() {
    return Crypto::generateKey(Crypto::KEY_SIZE_256);
}

std::vector<uint8_t> SecurityManager::generateIV() {
    return Crypto::generateIV();
}

// std::vector<uint8_t> SecurityManager::deriveKey(const std::vector<uint8_t>& key) {
//     return Crypto::deriveKey(key);
// }

} // namespace BarrenEngine 