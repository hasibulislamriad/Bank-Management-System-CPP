#pragma once
#include <cstdint>
#include <string>

namespace bank {
using AccountId = std::int64_t;
using Cents = std::int64_t;

struct Account {
    AccountId id{};
    std::string number;
    std::string name;
    std::string phone;
    std::string address;
    Cents balance{};
    bool frozen{};
};
}
