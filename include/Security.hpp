#pragma once
#include <string>

namespace bank::security {
std::string hashPin(const std::string& pin);
bool verifyPin(const std::string& pin, const std::string& encoded);
bool validPin(const std::string& pin);
}
