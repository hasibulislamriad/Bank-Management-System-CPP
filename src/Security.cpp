#include "Security.hpp"
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <vector>

namespace bank::security {
namespace {
constexpr int kIterations = 200000;
constexpr std::size_t kSaltSize = 16;
constexpr std::size_t kHashSize = 32;

std::string hexEncode(const unsigned char* data, std::size_t size) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < size; ++i) out << std::setw(2) << static_cast<unsigned>(data[i]);
    return out.str();
}

bool hexDecode(const std::string& text, std::vector<unsigned char>& out) {
    if (text.empty() || text.size() % 2) return false;
    out.resize(text.size() / 2);
    for (std::size_t i = 0; i < out.size(); ++i) {
        unsigned value{};
        std::istringstream in(text.substr(i * 2, 2));
        in >> std::hex >> value;
        if (in.fail() || value > 255) return false;
        out[i] = static_cast<unsigned char>(value);
    }
    return true;
}
}

bool validPin(const std::string& pin) {
    return pin.size() == 4 && std::all_of(pin.begin(), pin.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
}

std::string hashPin(const std::string& pin) {
    if (!validPin(pin)) throw std::invalid_argument("PIN must contain exactly four digits");
    unsigned char salt[kSaltSize]{};
    unsigned char digest[kHashSize]{};
    if (RAND_bytes(salt, sizeof salt) != 1) throw std::runtime_error("Secure salt generation failed");
    if (PKCS5_PBKDF2_HMAC(pin.c_str(), static_cast<int>(pin.size()), salt, sizeof salt, kIterations,
                          EVP_sha256(), sizeof digest, digest) != 1)
        throw std::runtime_error("PIN hashing failed");
    return "pbkdf2-sha256$" + std::to_string(kIterations) + "$" + hexEncode(salt, sizeof salt) + "$" + hexEncode(digest, sizeof digest);
}

bool verifyPin(const std::string& pin, const std::string& encoded) {
    const std::string prefix = "pbkdf2-sha256$";
    if (!validPin(pin) || encoded.rfind(prefix, 0) != 0) return false;
    std::vector<std::string> parts;
    std::stringstream ss(encoded);
    std::string part;
    while (std::getline(ss, part, '$')) parts.push_back(part);
    if (parts.size() != 4) return false;
    int iterations{};
    try { iterations = std::stoi(parts[1]); } catch (...) { return false; }
    if (iterations < 10000 || iterations > 10000000) return false;
    std::vector<unsigned char> salt, expected;
    if (!hexDecode(parts[2], salt) || !hexDecode(parts[3], expected) || salt.size() < kSaltSize || expected.size() != kHashSize) return false;
    std::vector<unsigned char> actual(expected.size());
    if (PKCS5_PBKDF2_HMAC(pin.c_str(), static_cast<int>(pin.size()), salt.data(), static_cast<int>(salt.size()),
                          iterations, EVP_sha256(), static_cast<int>(actual.size()), actual.data()) != 1) return false;
    return CRYPTO_memcmp(actual.data(), expected.data(), expected.size()) == 0;
}
}
