#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <limits>
#include <ctime>
#include <sstream>
#include <algorithm>

using namespace std;

class Account {
public:
    long long accountNumber;
    string name;
    string phone;
    string address;
    string pin;
    double balance;

    Account() : accountNumber(0), balance(0.0) {}
    Account(long long number, const string& name, const string& phone,
            const string& address, const string& pin, double balance = 0.0)
        : accountNumber(number), name(name), phone(phone), address(address), pin(pin), balance(balance) {}
};

class Bank {
private:
    vector<Account> accounts;
    const string accountsFile = "accounts.txt";
    const string transactionsFile = "transactions.txt";

    Account* findAccount(long long number) {
        for (auto& account : accounts)
            if (account.accountNumber == number) return &account;
        return nullptr;
    }

    bool accountExists(long long number) const {
        for (const auto& account : accounts)
            if (account.accountNumber == number) return true;
        return false;
    }

    string currentDateTime() const {
        time_t now = time(nullptr);
        tm* local = localtime(&now);
        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local);
        return buffer;
    }

    void recordTransaction(long long accountNumber, const string& type,
                           double amount, double balanceAfter, const string& note = "") const {
        ofstream file(transactionsFile, ios::app);
        if (!file) return;
        file << currentDateTime() << "|" << accountNumber << "|" << type << "|"
             << fixed << setprecision(2) << amount << "|" << balanceAfter << "|" << note << "\n";
    }

    bool readAmount(double& amount) const {
        cin >> amount;
        if (cin.fail() || amount <= 0) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid amount.\n";
            return false;
        }
        return true;
    }

    bool authenticate(Account*& account) {
        long long number;
        string pin;
        cout << "Account Number: ";
        cin >> number;
        cout << "PIN: ";
        cin >> pin;
        account = findAccount(number);
        if (!account || account->pin != pin) {
            cout << "Invalid account number or PIN.\n";
            account = nullptr;
            return false;
        }
        return true;
    }

public:
    Bank() { loadAccounts(); }

    void loadAccounts() {
        accounts.clear();
        ifstream file(accountsFile);
        if (!file) return;

        Account account;
        while (file >> account.accountNumber) {
            file.ignore(numeric_limits<streamsize>::max(), '\n');
            getline(file, account.name);
            getline(file, account.phone);
            getline(file, account.address);
            getline(file, account.pin);
            file >> account.balance;
            file.ignore(numeric_limits<streamsize>::max(), '\n');
            accounts.push_back(account);
        }
    }

    void saveAccounts() const {
        ofstream file(accountsFile);
        for (const auto& account : accounts) {
            file << account.accountNumber << '\n'
                 << account.name << '\n'
                 << account.phone << '\n'
                 << account.address << '\n'
                 << account.pin << '\n'
                 << fixed << setprecision(2) << account.balance << '\n';
        }
    }

    void createAccount() {
        long long number;
        string name, phone, address, pin;
        double initialDeposit;

        cout << "\n========== CREATE ACCOUNT ==========\n";
        cout << "Account Number: ";
        cin >> number;
        if (accountExists(number)) {
            cout << "Account number already exists.\n";
            return;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Full Name: "; getline(cin, name);
        cout << "Phone: "; getline(cin, phone);
        cout << "Address: "; getline(cin, address);
        cout << "Create 4-digit PIN: "; cin >> pin;
        if (pin.size() != 4 || !all_of(pin.begin(), pin.end(), ::isdigit)) {
            cout << "PIN must contain exactly 4 digits.\n";
            return;
        }
        cout << "Initial Deposit: ";
        if (!readAmount(initialDeposit)) return;

        accounts.emplace_back(number, name, phone, address, pin, initialDeposit);
        saveAccounts();
        recordTransaction(number, "CREATE", initialDeposit, initialDeposit, "Initial deposit");
        cout << "Account created successfully!\n";
    }

    void deposit() {
        Account* account;
        cout << "\n========== DEPOSIT ==========\n";
        if (!authenticate(account)) return;
        double amount;
        cout << "Amount: ";
        if (!readAmount(amount)) return;
        account->balance += amount;
        saveAccounts();
        recordTransaction(account->accountNumber, "DEPOSIT", amount, account->balance);
        cout << "Deposit successful. New balance: $" << fixed << setprecision(2) << account->balance << '\n';
    }

    void withdraw() {
        Account* account;
        cout << "\n========== WITHDRAW ==========\n";
        if (!authenticate(account)) return;
        double amount;
        cout << "Amount: ";
        if (!readAmount(amount)) return;
        if (amount > account->balance) {
            cout << "Insufficient balance.\n";
            return;
        }
        account->balance -= amount;
        saveAccounts();
        recordTransaction(account->accountNumber, "WITHDRAW", amount, account->balance);
        cout << "Withdrawal successful. New balance: $" << fixed << setprecision(2) << account->balance << '\n';
    }

    void checkBalance() {
        Account* account;
        cout << "\n========== BALANCE ==========\n";
        if (!authenticate(account)) return;
        cout << "Available Balance: $" << fixed << setprecision(2) << account->balance << '\n';
    }

    void showAccountInfo() {
        Account* account;
        cout << "\n========== ACCOUNT INFORMATION ==========\n";
        if (!authenticate(account)) return;
        cout << "Account Number : " << account->accountNumber << '\n'
             << "Name           : " << account->name << '\n'
             << "Phone          : " << account->phone << '\n'
             << "Address        : " << account->address << '\n'
             << "Balance        : $" << fixed << setprecision(2) << account->balance << '\n';
    }

    void transfer() {
        Account* sender;
        cout << "\n========== MONEY TRANSFER ==========\n";
        if (!authenticate(sender)) return;

        long long receiverNumber;
        double amount;
        cout << "Receiver Account Number: "; cin >> receiverNumber;
        Account* receiver = findAccount(receiverNumber);
        if (!receiver) { cout << "Receiver account not found.\n"; return; }
        if (receiver == sender) { cout << "You cannot transfer to the same account.\n"; return; }
        cout << "Amount: ";
        if (!readAmount(amount)) return;
        if (amount > sender->balance) { cout << "Insufficient balance.\n"; return; }

        sender->balance -= amount;
        receiver->balance += amount;
        saveAccounts();
        recordTransaction(sender->accountNumber, "TRANSFER_OUT", amount, sender->balance, "To " + to_string(receiver->accountNumber));
        recordTransaction(receiver->accountNumber, "TRANSFER_IN", amount, receiver->balance, "From " + to_string(sender->accountNumber));
        cout << "Transfer completed successfully.\n";
    }

    void updateAccount() {
        Account* account;
        cout << "\n========== UPDATE ACCOUNT ==========\n";
        if (!authenticate(account)) return;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "New Phone: "; getline(cin, account->phone);
        cout << "New Address: "; getline(cin, account->address);
        saveAccounts();
        cout << "Account updated successfully.\n";
    }

    void deleteAccount() {
        Account* account;
        cout << "\n========== DELETE ACCOUNT ==========\n";
        if (!authenticate(account)) return;
        if (account->balance != 0) {
            cout << "Account can only be deleted when balance is $0.00.\n";
            return;
        }
        long long number = account->accountNumber;
        accounts.erase(remove_if(accounts.begin(), accounts.end(),
                                 [number](const Account& a) { return a.accountNumber == number; }), accounts.end());
        saveAccounts();
        recordTransaction(number, "DELETE", 0, 0, "Account deleted");
        cout << "Account deleted successfully.\n";
    }

    void transactionHistory() const {
        long long number;
        string pin;
        cout << "\n========== TRANSACTION HISTORY ==========\n";
        cout << "Account Number: "; cin >> number;
        cout << "PIN: "; cin >> pin;
        bool valid = false;
        for (const auto& account : accounts)
            if (account.accountNumber == number && account.pin == pin) valid = true;
        if (!valid) { cout << "Invalid account number or PIN.\n"; return; }

        ifstream file(transactionsFile);
        if (!file) { cout << "No transactions found.\n"; return; }
        string line;
        bool found = false;
        while (getline(file, line)) {
            stringstream ss(line);
            string date, acc, type, amount, balance, note;
            getline(ss, date, '|'); getline(ss, acc, '|'); getline(ss, type, '|');
            getline(ss, amount, '|'); getline(ss, balance, '|'); getline(ss, note);
            if (acc == to_string(number)) {
                cout << date << " | " << type << " | Amount: " << amount
                     << " | Balance: " << balance;
                if (!note.empty()) cout << " | " << note;
                cout << '\n';
                found = true;
            }
        }
        if (!found) cout << "No transactions found for this account.\n";
    }

    void statistics() const {
        double total = 0;
        for (const auto& account : accounts) total += account.balance;
        cout << "\n========== BANK STATISTICS ==========\n"
             << "Total Accounts : " << accounts.size() << '\n'
             << "Total Deposits : $" << fixed << setprecision(2) << total << '\n';
    }
};

int main() {
    Bank bank;
    int choice;

    while (true) {
        cout << "\n===============================================\n"
             << "          PROFESSIONAL BANK SYSTEM\n"
             << "===============================================\n"
             << "1. Create Account\n"
             << "2. Deposit Money\n"
             << "3. Withdraw Money\n"
             << "4. Check Balance\n"
             << "5. Money Transfer\n"
             << "6. Account Information\n"
             << "7. Transaction History\n"
             << "8. Update Account\n"
             << "9. Delete Account\n"
             << "10. Bank Statistics\n"
             << "11. Exit\n"
             << "===============================================\n"
             << "Choice: ";

        cin >> choice;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input.\n";
            continue;
        }

        switch (choice) {
            case 1: bank.createAccount(); break;
            case 2: bank.deposit(); break;
            case 3: bank.withdraw(); break;
            case 4: bank.checkBalance(); break;
            case 5: bank.transfer(); break;
            case 6: bank.showAccountInfo(); break;
            case 7: bank.transactionHistory(); break;
            case 8: bank.updateAccount(); break;
            case 9: bank.deleteAccount(); break;
            case 10: bank.statistics(); break;
            case 11: cout << "Thank you for using Professional Bank System!\n"; return 0;
            default: cout << "Invalid choice. Please try again.\n";
        }
    }
}
