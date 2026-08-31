#pragma once
#include "Database.hpp"
#include "Models.hpp"
#include <optional>
#include <string>

namespace bank {
class BankService {
    Database& db_;
public:
    explicit BankService(Database& db) : db_(db) {}
    void createAccount(const std::string& number, const std::string& name, const std::string& phone,
                      const std::string& address, const std::string& pin, Cents initialDeposit);
    bool authenticate(const std::string& number, const std::string& pin, Account& account);
    void deposit(const std::string& number, const std::string& pin, Cents amount);
    void withdraw(const std::string& number, const std::string& pin, Cents amount);
    void transfer(const std::string& from, const std::string& pin, const std::string& to, Cents amount);
};
}
