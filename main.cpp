#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

namespace util {
string trim(string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

string now() {
    const auto t = chrono::system_clock::to_time_t(chrono::system_clock::now());
    tm local{};
#ifdef _WIN32
    localtime_s(&local, &t);
#else
    localtime_r(&t, &local);
#endif
    ostringstream out;
    out << put_time(&local, "%Y-%m-%d %H:%M:%S");
    return out.str();
}

bool validPin(const string& pin) {
    return pin.size() == 4 && all_of(pin.begin(), pin.end(), [](unsigned char c) {
        return std::isdigit(c) != 0;
    });
}
}

class Money {
public:
    static constexpr long long SCALE = 100;
    long long cents{};

    static optional<Money> fromDouble(double value) {
        if (!isfinite(value) || value <= 0) return nullopt;
        return Money{static_cast<long long>(llround(value * SCALE))};
    }

    string str() const {
        ostringstream out;
        out << fixed << setprecision(2) << (static_cast<double>(cents) / SCALE);
        return out.str();
    }
};

struct Account {
    long long number{};
    string name;
    string phone;
    string address;
    string pin; // Demo project only. Production systems should use a salted password hash.
    long long balanceCents{};
    bool frozen{false};
    int failedAttempts{0};
};

class Bank {
    vector<Account> accounts;
    mutable mutex dataMutex;
    const string accountsFile = "accounts.txt";
    const string transactionsFile = "transactions.txt";
    static constexpr int MAX_FAILED_ATTEMPTS = 3;

    Account* findUnlocked(long long number) {
        for (auto& a : accounts) if (a.number == number) return &a;
        return nullptr;
    }

    const Account* findUnlocked(long long number) const {
        for (const auto& a : accounts) if (a.number == number) return &a;
        return nullptr;
    }

    void audit(const string& type, long long account, long long amount, const string& note) const {
        ofstream file(transactionsFile, ios::app);
        if (!file) return;
        file << util::now() << '|' << account << '|' << type << '|'
             << Money{amount}.str() << '|' << note << '\n';
    }

    static void line() { cout << string(58, '-') << '\n'; }

    optional<long long> readAmountCents() const {
        double value;
        cin >> value;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return nullopt;
        }
        auto money = Money::fromDouble(value);
        return money ? optional<long long>(money->cents) : nullopt;
    }

    Account* authenticate() {
        long long number;
        string pin;
        cout << "Account Number: ";
        cin >> number;
        cout << "PIN: ";
        cin >> pin;

        Account* account = findUnlocked(number);
        if (!account || account->frozen) {
            cout << "Invalid or frozen account.\n";
            return nullptr;
        }
        if (account->pin != pin) {
            ++account->failedAttempts;
            if (account->failedAttempts >= MAX_FAILED_ATTEMPTS) account->frozen = true;
            cout << "Invalid PIN. Attempts: " << account->failedAttempts << '/' << MAX_FAILED_ATTEMPTS << '\n';
            if (account->frozen) cout << "Account has been frozen after too many failed attempts.\n";
            return nullptr;
        }
        account->failedAttempts = 0;
        return account;
    }

public:
    Bank() { load(); }

    void load() {
        lock_guard lock(dataMutex);
        accounts.clear();
        ifstream file(accountsFile);
        if (!file) return;
        Account a;
        int frozen = 0;
        while (file >> a.number) {
            file.ignore(numeric_limits<streamsize>::max(), '\n');
            getline(file, a.name);
            getline(file, a.phone);
            getline(file, a.address);
            getline(file, a.pin);
            file >> a.balanceCents >> frozen >> a.failedAttempts;
            file.ignore(numeric_limits<streamsize>::max(), '\n');
            a.frozen = frozen != 0;
            accounts.push_back(a);
        }
    }

    void save() const {
        lock_guard lock(dataMutex);
        ofstream file(accountsFile, ios::trunc);
        if (!file) throw runtime_error("Unable to save accounts file");
        for (const auto& a : accounts) {
            file << a.number << '\n' << a.name << '\n' << a.phone << '\n' << a.address << '\n'
                 << a.pin << '\n' << a.balanceCents << ' ' << a.frozen << ' ' << a.failedAttempts << '\n';
        }
    }

    void createAccount() {
        lock_guard lock(dataMutex);
        long long number;
        string pin;
        Account a;
        cout << "\n========== CREATE ACCOUNT ==========\nAccount Number: ";
        cin >> number;
        if (findUnlocked(number)) { cout << "Account already exists.\n"; return; }
        a.number = number;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Full Name: "; getline(cin, a.name);
        cout << "Phone: "; getline(cin, a.phone);
        cout << "Address: "; getline(cin, a.address);
        cout << "Create 4-digit PIN: "; cin >> pin;
        if (!util::validPin(pin)) { cout << "PIN must contain exactly 4 digits.\n"; return; }
        a.pin = pin;
        cout << "Initial Deposit: ";
        auto amount = readAmountCents();
        if (!amount) { cout << "Invalid amount.\n"; return; }
        a.balanceCents = *amount;
        accounts.push_back(a);
        ofstream tx(transactionsFile, ios::app);
        if (tx) tx << util::now() << '|' << a.number << "|CREATE|" << Money{a.balanceCents}.str() << "|Initial deposit\n";
        saveUnlocked();
        cout << "Account created successfully.\n";
    }

    void saveUnlocked() const {
        ofstream file(accountsFile, ios::trunc);
        if (!file) throw runtime_error("Unable to save accounts file");
        for (const auto& a : accounts) {
            file << a.number << '\n' << a.name << '\n' << a.phone << '\n' << a.address << '\n'
                 << a.pin << '\n' << a.balanceCents << ' ' << a.frozen << ' ' << a.failedAttempts << '\n';
        }
    }

    void deposit() {
        lock_guard lock(dataMutex);
        cout << "\n========== DEPOSIT ==========\n";
        Account* a = authenticate(); if (!a) return;
        cout << "Amount: "; auto amount = readAmountCents();
        if (!amount) { cout << "Invalid amount.\n"; return; }
        a->balanceCents += *amount;
        audit("DEPOSIT", a->number, *amount, "Cash deposit");
        saveUnlocked();
        cout << "Deposit successful. Balance: $" << Money{a->balanceCents}.str() << '\n';
    }

    void withdraw() {
        lock_guard lock(dataMutex);
        cout << "\n========== WITHDRAW ==========\n";
        Account* a = authenticate(); if (!a) return;
        cout << "Amount: "; auto amount = readAmountCents();
        if (!amount) { cout << "Invalid amount.\n"; return; }
        if (*amount > a->balanceCents) { cout << "Insufficient balance.\n"; return; }
        a->balanceCents -= *amount;
        audit("WITHDRAW", a->number, *amount, "Cash withdrawal");
        saveUnlocked();
        cout << "Withdrawal successful. Balance: $" << Money{a->balanceCents}.str() << '\n';
    }

    void transfer() {
        lock_guard lock(dataMutex);
        cout << "\n========== TRANSFER ==========\n";
        Account* sender = authenticate(); if (!sender) return;
        long long receiverNumber;
        cout << "Receiver Account Number: "; cin >> receiverNumber;
        Account* receiver = findUnlocked(receiverNumber);
        if (!receiver || receiver->frozen) { cout << "Receiver unavailable.\n"; return; }
        if (receiver == sender) { cout << "Cannot transfer to the same account.\n"; return; }
        cout << "Amount: "; auto amount = readAmountCents();
        if (!amount || *amount > sender->balanceCents) { cout << "Invalid amount or insufficient balance.\n"; return; }
        sender->balanceCents -= *amount;
        receiver->balanceCents += *amount;
        audit("TRANSFER_OUT", sender->number, *amount, "To " + to_string(receiver->number));
        audit("TRANSFER_IN", receiver->number, *amount, "From " + to_string(sender->number));
        saveUnlocked();
        cout << "Transfer completed successfully.\n";
    }

    void accountInfo() {
        lock_guard lock(dataMutex);
        cout << "\n========== ACCOUNT INFORMATION ==========\n";
        Account* a = authenticate(); if (!a) return;
        line();
        cout << "Account Number : " << a->number << '\n'
             << "Name           : " << a->name << '\n'
             << "Phone          : " << a->phone << '\n'
             << "Address        : " << a->address << '\n'
             << "Status         : " << (a->frozen ? "FROZEN" : "ACTIVE") << '\n'
             << "Balance        : $" << Money{a->balanceCents}.str() << '\n';
    }

    void history() {
        lock_guard lock(dataMutex);
        cout << "\n========== TRANSACTION HISTORY ==========\n";
        Account* a = authenticate(); if (!a) return;
        ifstream file(transactionsFile);
        if (!file) { cout << "No transaction history.\n"; return; }
        string lineText; bool found = false;
        while (getline(file, lineText)) {
            stringstream ss(lineText);
            string date, id, type, amount, note;
            getline(ss, date, '|'); getline(ss, id, '|'); getline(ss, type, '|'); getline(ss, amount, '|'); getline(ss, note);
            if (id == to_string(a->number)) {
                cout << date << " | " << type << " | $" << amount << " | " << note << '\n';
                found = true;
            }
        }
        if (!found) cout << "No transactions found.\n";
    }

    void updateAccount() {
        lock_guard lock(dataMutex);
        cout << "\n========== UPDATE ACCOUNT ==========\n";
        Account* a = authenticate(); if (!a) return;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "New Phone: "; getline(cin, a->phone);
        cout << "New Address: "; getline(cin, a->address);
        saveUnlocked();
        cout << "Account updated.\n";
    }

    void freezeAccount() {
        lock_guard lock(dataMutex);
        cout << "\n========== FREEZE / UNFREEZE ==========\n";
        long long number; string pin;
        cout << "Account Number: "; cin >> number;
        Account* a = findUnlocked(number);
        if (!a) { cout << "Account not found.\n"; return; }
        cout << "PIN: "; cin >> pin;
        if (a->pin != pin) { cout << "Invalid PIN.\n"; return; }
        a->frozen = !a->frozen;
        a->failedAttempts = 0;
        audit(a->frozen ? "FREEZE" : "UNFREEZE", a->number, 0, "Account status changed");
        saveUnlocked();
        cout << (a->frozen ? "Account frozen.\n" : "Account unfrozen.\n");
    }

    void statistics() const {
        lock_guard lock(dataMutex);
        long long total = 0; size_t frozen = 0;
        for (const auto& a : accounts) { total += a.balanceCents; frozen += a.frozen; }
        cout << "\n========== BANK STATISTICS ==========\n"
             << "Accounts       : " << accounts.size() << '\n'
             << "Active         : " << accounts.size() - frozen << '\n'
             << "Frozen         : " << frozen << '\n'
             << "Total Deposits : $" << Money{total}.str() << '\n';
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    Bank bank;
    while (true) {
        cout << "\n====================================================\n"
             << "           PROFESSIONAL BANK SYSTEM 2026\n"
             << "====================================================\n"
             << "1. Create Account\n2. Deposit\n3. Withdraw\n4. Check Balance\n"
             << "5. Money Transfer\n6. Account Information\n7. Transaction History\n"
             << "8. Update Account\n9. Freeze/Unfreeze Account\n10. Statistics\n11. Exit\n"
             << "====================================================\nChoice: ";
        int choice;
        cin >> choice;
        if (cin.fail()) {
            cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid choice.\n"; continue;
        }
        try {
            switch (choice) {
                case 1: bank.createAccount(); break;
                case 2: bank.deposit(); break;
                case 3: bank.withdraw(); break;
                case 4: { bank.accountInfo(); break; }
                case 5: bank.transfer(); break;
                case 6: bank.accountInfo(); break;
                case 7: bank.history(); break;
                case 8: bank.updateAccount(); break;
                case 9: bank.freezeAccount(); break;
                case 10: bank.statistics(); break;
                case 11: cout << "Goodbye!\n"; return 0;
                default: cout << "Invalid choice.\n";
            }
        } catch (const exception& e) {
            cerr << "Operation failed: " << e.what() << '\n';
        }
    }
}
