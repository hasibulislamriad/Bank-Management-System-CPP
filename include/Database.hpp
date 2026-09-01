#pragma once
#include <sqlite3.h>
#include <stdexcept>
#include <string>

namespace bank {
class Statement {
    sqlite3_stmt* stmt_{};
public:
    Statement(sqlite3* db, const char* sql) {
        if (sqlite3_prepare_v2(db, sql, -1, &stmt_, nullptr) != SQLITE_OK)
            throw std::runtime_error(std::string("SQLite prepare failed: ") + sqlite3_errmsg(db));
    }
    ~Statement() { if (stmt_) sqlite3_finalize(stmt_); }
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    sqlite3_stmt* get() const noexcept { return stmt_; }
};

class Transaction {
    sqlite3* db_;
    bool committed_{false};
public:
    explicit Transaction(sqlite3* db) : db_(db) {
        if (sqlite3_exec(db_, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr) != SQLITE_OK)
            throw std::runtime_error(sqlite3_errmsg(db_));
    }
    void commit() {
        if (sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr) != SQLITE_OK)
            throw std::runtime_error(sqlite3_errmsg(db_));
        committed_ = true;
    }
    ~Transaction() { if (!committed_) sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr); }
    Transaction(const Transaction&) = delete;
};

class Database {
    sqlite3* db_{};
public:
    explicit Database(const std::string& path = "bank.db");
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    sqlite3* raw() const noexcept { return db_; }
    void initialize();
};
}
