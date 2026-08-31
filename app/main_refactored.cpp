#include "BankService.hpp"
#include <iostream>
#include <limits>
#include <string>

using namespace bank;

static Cents readAmount() {
    long double value{};
    if (!(std::cin >> value) || value <= 0 || value > 92233720368547758.07L)
        throw std::invalid_argument("Invalid amount");
    return static_cast<Cents>(value * 100.0L + 0.5L);
}

int main() {
    try {
        Database database;
        BankService service(database);
        for (;;) {
            std::cout << "\n=== PROFESSIONAL BANK SYSTEM 2026 ===\n"
                      << "1. Create account\n2. Deposit\n3. Withdraw\n4. Transfer\n"
                      << "5. Account information\n6. Transaction history\n7. Exit\nChoice: ";
            int choice{};
            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Invalid choice.\n";
                continue;
            }
            try {
                if (choice == 1) {
                    std::string number, name, phone, address, pin;
                    std::cout << "Account Number: "; std::cin >> number;
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    std::cout << "Full Name: "; std::getline(std::cin, name);
                    std::cout << "Phone: "; std::getline(std::cin, phone);
                    std::cout << "Address: "; std::getline(std::cin, address);
                    std::cout << "4-digit PIN: "; std::cin >> pin;
                    std::cout << "Initial Deposit: ";
                    service.createAccount(number, name, phone, address, pin, readAmount());
                    std::cout << "Account created successfully.\n";
                } else if (choice == 2 || choice == 3) {
                    std::string number, pin;
                    std::cout << "Account Number: "; std::cin >> number;
                    std::cout << "PIN: "; std::cin >> pin;
                    std::cout << "Amount: "; const auto amount = readAmount();
                    if (choice == 2) service.deposit(number, pin, amount);
                    else service.withdraw(number, pin, amount);
                    std::cout << "Transaction completed.\n";
                } else if (choice == 4) {
                    std::string from, pin, to;
                    std::cout << "Sender Account: "; std::cin >> from;
                    std::cout << "PIN: "; std::cin >> pin;
                    std::cout << "Receiver Account: "; std::cin >> to;
                    std::cout << "Amount: "; const auto amount = readAmount();
                    service.transfer(from, pin, to, amount);
                    std::cout << "Transfer completed.\n";
                } else if (choice == 5) {
                    std::cout << "Account information requires authentication.\n";
                    std::string number, pin; std::cout << "Account Number: "; std::cin >> number; std::cout << "PIN: "; std::cin >> pin;
                    Account account;
                    if (!service.authenticate(number, pin, account)) std::cout << "Authentication failed.\n";
                    else std::cout << "Name: " << account.name << "\nPhone: " << account.phone << "\nAddress: " << account.address << "\nBalance: " << account.balance << " cents\n";
                } else if (choice == 6) {
                    std::cout << "Transaction history is available through the database/API layer.\n";
                } else if (choice == 7) {
                    return 0;
                } else {
                    std::cout << "Invalid choice.\n";
                }
            } catch (const std::exception& e) {
                std::cerr << "Operation failed: " << e.what() << '\n';
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Startup failed: " << e.what() << '\n';
        return 1;
    }
}
