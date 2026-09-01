#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <chrono>

namespace bank::api {

enum class Role { Customer, Admin };

struct Claims {
    std::string subject;
    Role role{Role::Customer};
    std::chrono::system_clock::time_point expires_at{};
};

// JWT contract. Production implementation must use a vetted JWT library,
// a strong signing key loaded from a secret manager/environment, short expiry,
// issuer/audience validation, and key rotation. Never put PINs or secrets in claims.
class JwtService {
public:
    JwtService(std::string issuer, std::string audience, std::string signing_key)
        : issuer_(std::move(issuer)), audience_(std::move(audience)), signing_key_(std::move(signing_key)) {}

    std::string issue(const Claims& claims) const;
    std::optional<Claims> verify(std::string_view token) const;

private:
    std::string issuer_;
    std::string audience_;
    std::string signing_key_;
};

inline bool canAccessCustomerAccount(const Claims& claims, std::string_view account) {
    return claims.role == Role::Admin || claims.subject == account;
}

} // namespace bank::api
