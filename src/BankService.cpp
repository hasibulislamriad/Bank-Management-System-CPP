#include "BankService.hpp"
#include "Security.hpp"
#include <limits>
#include <stdexcept>

namespace bank {
namespace {
void bindText(sqlite3_stmt* s, int index, const std::string& value) {
    if (sqlite3_bind_text(s, index, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) throw std::runtime_error("SQLite bind failed");
}
void exec(sqlite3* db, const char* sql) {
    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : sqlite3_errmsg(db); sqlite3_free(err); throw std::runtime_error(msg);
    }
}
void record(sqlite3* db, AccountId id, const char* type, Cents amount, Cents balance, const std::string& note) {
    Statement s(db, "INSERT INTO transactions(account_id,type,amount_cents,balance_after_cents,note) VALUES(?,?,?,?,?)");
    sqlite3_bind_int64(s.get(), 1, id); bindText(s.get(), 2, type); sqlite3_bind_int64(s.get(), 3, amount); sqlite3_bind_int64(s.get(), 4, balance); bindText(s.get(), 5, note);
    if (sqlite3_step(s.get()) != SQLITE_DONE) throw std::runtime_error(std::string("Transaction insert failed: ") + sqlite3_errmsg(db));
}
void audit(sqlite3* db, AccountId id, const char* action, const std::string& details) {
    Statement s(db, "INSERT INTO audit_logs(account_id,action,details) VALUES(?,?,?)");
    sqlite3_bind_int64(s.get(), 1, id); bindText(s.get(), 2, action); bindText(s.get(), 3, details);
    if (sqlite3_step(s.get()) != SQLITE_DONE) throw std::runtime_error(std::string("Audit insert failed: ") + sqlite3_errmsg(db));
}
}

void BankService::createAccount(const std::string& number, const std::string& name, const std::string& phone,
                                const std::string& address, const std::string& pin, Cents initialDeposit) {
    if (number.empty() || name.empty() || initialDeposit < 0) throw std::invalid_argument("Invalid account data");
    if (!security::validPin(pin)) throw std::invalid_argument("PIN must contain exactly four digits");
    const std::string hash = security::hashPin(pin);
    Transaction tx(db_.raw());
    Statement s(db_.raw(), "INSERT INTO accounts(account_number,name,phone,address,pin_hash,balance_cents) VALUES(?,?,?,?,?,?)");
    bindText(s.get(), 1, number); bindText(s.get(), 2, name); bindText(s.get(), 3, phone); bindText(s.get(), 4, address); bindText(s.get(), 5, hash); sqlite3_bind_int64(s.get(), 6, initialDeposit);
    if (sqlite3_step(s.get()) != SQLITE_DONE) throw std::runtime_error(std::string("Account creation failed: ") + sqlite3_errmsg(db_.raw()));
    const auto id = sqlite3_last_insert_rowid(db_.raw());
    if (initialDeposit > 0) record(db_.raw(), id, "CREATE", initialDeposit, initialDeposit, "Initial deposit");
    audit(db_.raw(), id, "ACCOUNT_CREATED", "Account created");
    tx.commit();
}

bool BankService::authenticate(const std::string& number, const std::string& pin, Account& account) {
    Statement s(db_.raw(), "SELECT id,account_number,name,phone,address,pin_hash,balance_cents,frozen,failed_attempts FROM accounts WHERE account_number=?");
    bindText(s.get(), 1, number);
    if (sqlite3_step(s.get()) != SQLITE_ROW) return false;
    const auto id = sqlite3_column_int64(s.get(), 0);
    const std::string hash = reinterpret_cast<const char*>(sqlite3_column_text(s.get(), 5));
    const int frozen = sqlite3_column_int(s.get(), 7);
    const int attempts = sqlite3_column_int(s.get(), 8);
    if (frozen) return false;
    if (!security::verifyPin(pin, hash)) {
        const int next = attempts + 1; const int freeze = next >= 3;
        Statement u(db_.raw(), "UPDATE accounts SET failed_attempts=?,frozen=?,updated_at=CURRENT_TIMESTAMP WHERE id=?");
        sqlite3_bind_int(u.get(), 1, next); sqlite3_bind_int(u.get(), 2, freeze); sqlite3_bind_int64(u.get(), 3, id);
        if (sqlite3_step(u.get()) != SQLITE_DONE) throw std::runtime_error(std::string("Authentication update failed: ") + sqlite3_errmsg(db_.raw()));
        audit(db_.raw(), id, "AUTH_FAILURE", freeze ? "Account frozen" : "Invalid PIN");
        return false;
    }
    Statement u(db_.raw(), "UPDATE accounts SET failed_attempts=0,updated_at=CURRENT_TIMESTAMP WHERE id=?"); sqlite3_bind_int64(u.get(), 1, id);
    if (sqlite3_step(u.get()) != SQLITE_DONE) throw std::runtime_error(std::string("Authentication reset failed: ") + sqlite3_errmsg(db_.raw()));
    account = {id, reinterpret_cast<const char*>(sqlite3_column_text(s.get(),1)), reinterpret_cast<const char*>(sqlite3_column_text(s.get(),2)), reinterpret_cast<const char*>(sqlite3_column_text(s.get(),3)), reinterpret_cast<const char*>(sqlite3_column_text(s.get(),4)), sqlite3_column_int64(s.get(),6), false};
    return true;
}

void BankService::deposit(const std::string& number, const std::string& pin, Cents amount) {
    if (amount <= 0) throw std::invalid_argument("Amount must be positive"); Account a; if (!authenticate(number,pin,a)) throw std::runtime_error("Authentication failed");
    if (amount > std::numeric_limits<Cents>::max() - a.balance) throw std::overflow_error("Balance overflow"); Transaction tx(db_.raw()); const auto next = a.balance + amount;
    Statement u(db_.raw(), "UPDATE accounts SET balance_cents=?,updated_at=CURRENT_TIMESTAMP WHERE id=? AND frozen=0"); sqlite3_bind_int64(u.get(),1,next); sqlite3_bind_int64(u.get(),2,a.id);
    if (sqlite3_step(u.get()) != SQLITE_DONE || sqlite3_changes(db_.raw()) != 1) throw std::runtime_error("Deposit update failed"); record(db_.raw(),a.id,"DEPOSIT",amount,next,"Cash deposit"); audit(db_.raw(),a.id,"DEPOSIT","Deposit completed"); tx.commit();
}

void BankService::withdraw(const std::string& number, const std::string& pin, Cents amount) {
    if (amount <= 0) throw std::invalid_argument("Amount must be positive"); Account a; if (!authenticate(number,pin,a)) throw std::runtime_error("Authentication failed");
    if (amount > a.balance) throw std::runtime_error("Insufficient balance"); Transaction tx(db_.raw()); const auto next = a.balance - amount;
    Statement u(db_.raw(), "UPDATE accounts SET balance_cents=?,updated_at=CURRENT_TIMESTAMP WHERE id=? AND balance_cents>=? AND frozen=0"); sqlite3_bind_int64(u.get(),1,next); sqlite3_bind_int64(u.get(),2,a.id); sqlite3_bind_int64(u.get(),3,amount);
    if (sqlite3_step(u.get()) != SQLITE_DONE || sqlite3_changes(db_.raw()) != 1) throw std::runtime_error("Withdrawal update failed"); record(db_.raw(),a.id,"WITHDRAW",amount,next,"Cash withdrawal"); audit(db_.raw(),a.id,"WITHDRAW","Withdrawal completed"); tx.commit();
}

void BankService::transfer(const std::string& from, const std::string& pin, const std::string& to, Cents amount) {
    if (amount <= 0 || from == to) throw std::invalid_argument("Invalid transfer"); Account sender; if (!authenticate(from,pin,sender)) throw std::runtime_error("Authentication failed");
    Statement r(db_.raw(), "SELECT id,balance_cents,frozen FROM accounts WHERE account_number=?"); bindText(r.get(),1,to); if (sqlite3_step(r.get()) != SQLITE_ROW) throw std::runtime_error("Receiver not found");
    const auto rid=sqlite3_column_int64(r.get(),0); const auto rb=sqlite3_column_int64(r.get(),1); if (sqlite3_column_int(r.get(),2)) throw std::runtime_error("Receiver is frozen"); if (amount>sender.balance || rb>std::numeric_limits<Cents>::max()-amount) throw std::runtime_error("Invalid transfer amount");
    Transaction tx(db_.raw()); const auto sb=sender.balance-amount, nb=rb+amount;
    Statement us(db_.raw(),"UPDATE accounts SET balance_cents=?,updated_at=CURRENT_TIMESTAMP WHERE id=? AND balance_cents>=? AND frozen=0"); sqlite3_bind_int64(us.get(),1,sb); sqlite3_bind_int64(us.get(),2,sender.id); sqlite3_bind_int64(us.get(),3,amount); if(sqlite3_step(us.get())!=SQLITE_DONE||sqlite3_changes(db_.raw())!=1) throw std::runtime_error("Sender update failed");
    Statement ur(db_.raw(),"UPDATE accounts SET balance_cents=?,updated_at=CURRENT_TIMESTAMP WHERE id=? AND frozen=0"); sqlite3_bind_int64(ur.get(),1,nb); sqlite3_bind_int64(ur.get(),2,rid); if(sqlite3_step(ur.get())!=SQLITE_DONE||sqlite3_changes(db_.raw())!=1) throw std::runtime_error("Receiver update failed");
    record(db_.raw(),sender.id,"TRANSFER_OUT",amount,sb,"To "+to); record(db_.raw(),rid,"TRANSFER_IN",amount,nb,"From "+from); audit(db_.raw(),sender.id,"TRANSFER_OUT","To "+to); audit(db_.raw(),rid,"TRANSFER_IN","From "+from); tx.commit();
}
}
