#include <sqlite3.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cctype>
#include <limits>

using namespace std;

class Db {
    sqlite3* db_ = nullptr;

    static void check(int rc, sqlite3* db, const string& action) {
        if (rc != SQLITE_OK && rc != SQLITE_DONE && rc != SQLITE_ROW)
            throw runtime_error(action + ": " + sqlite3_errmsg(db));
    }

public:
    explicit Db(const string& path = "bank.db") {
        check(sqlite3_open(path.c_str(), &db_), db_, "open database");
        sqlite3_exec(db_, "PRAGMA foreign_keys=ON; PRAGMA journal_mode=WAL; PRAGMA busy_timeout=5000;", nullptr, nullptr, nullptr);
        const char* schema = R"SQL(
CREATE TABLE IF NOT EXISTS accounts(
 id INTEGER PRIMARY KEY AUTOINCREMENT,
 account_number TEXT NOT NULL UNIQUE,
 name TEXT NOT NULL,
 phone TEXT NOT NULL DEFAULT '',
 address TEXT NOT NULL DEFAULT '',
 pin_hash TEXT NOT NULL,
 balance_cents INTEGER NOT NULL DEFAULT 0 CHECK(balance_cents >= 0),
 frozen INTEGER NOT NULL DEFAULT 0,
 failed_attempts INTEGER NOT NULL DEFAULT 0,
 created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
 updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE IF NOT EXISTS transactions(
 id INTEGER PRIMARY KEY AUTOINCREMENT,
 account_id INTEGER NOT NULL,
 type TEXT NOT NULL,
 amount_cents INTEGER NOT NULL CHECK(amount_cents >= 0),
 balance_after_cents INTEGER NOT NULL CHECK(balance_after_cents >= 0),
 reference TEXT,
 note TEXT,
 created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
 FOREIGN KEY(account_id) REFERENCES accounts(id)
);
CREATE TABLE IF NOT EXISTS audit_logs(
 id INTEGER PRIMARY KEY AUTOINCREMENT,
 account_id INTEGER,
 action TEXT NOT NULL,
 details TEXT,
 created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
 FOREIGN KEY(account_id) REFERENCES accounts(id)
);
CREATE INDEX IF NOT EXISTS idx_accounts_number ON accounts(account_number);
CREATE INDEX IF NOT EXISTS idx_transactions_account ON transactions(account_id);
)SQL";
        check(sqlite3_exec(db_, schema, nullptr, nullptr, nullptr), db_, "create schema");
    }

    ~Db() { if (db_) sqlite3_close(db_); }
    sqlite3* raw() { return db_; }

    void begin() { check(sqlite3_exec(db_, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr), db_, "begin transaction"); }
    void commit() { check(sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr), db_, "commit transaction"); }
    void rollback() { sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr); }
};

static string hex(const unsigned char* data, size_t n) {
    ostringstream out;
    out << hex << setfill('0');
    for (size_t i = 0; i < n; ++i) out << setw(2) << static_cast<int>(data[i]);
    return out.str();
}

static vector<unsigned char> unhex(const string& s) {
    if (s.size() % 2) throw runtime_error("Invalid hash encoding");
    vector<unsigned char> out(s.size() / 2);
    for (size_t i = 0; i < out.size(); ++i) {
        unsigned int v;
        string byte = s.substr(i * 2, 2);
        stringstream ss; ss << std::hex << byte; ss >> v;
        out[i] = static_cast<unsigned char>(v);
    }
    return out;
}

// PBKDF2-HMAC-SHA256 with a random 16-byte salt.
// For real production banking, prefer a dedicated password KDF such as Argon2id.
static string hashPin(const string& pin) {
    constexpr int iterations = 200000;
    unsigned char salt[16], digest[32];
    if (RAND_bytes(salt, sizeof salt) != 1)
        throw runtime_error("Unable to generate secure salt");
    if (PKCS5_PBKDF2_HMAC(pin.c_str(), static_cast<int>(pin.size()), salt, sizeof salt,
                         iterations, EVP_sha256(), sizeof digest, digest) != 1)
        throw runtime_error("PIN hashing failed");
    return "pbkdf2-sha256$" + to_string(iterations) + "$" + hex(salt, sizeof salt) + "$" + hex(digest, sizeof digest);
}

static bool verifyPin(const string& pin, const string& stored) {
    const string prefix = "pbkdf2-sha256$";
    if (stored.rfind(prefix, 0) != 0) return false;
    vector<string> parts; string part; stringstream ss(stored);
    while (getline(ss, part, '$')) parts.push_back(part);
    if (parts.size() != 4) return false;
    int iterations = stoi(parts[1]);
    auto salt = unhex(parts[2]);
    auto expected = unhex(parts[3]);
    vector<unsigned char> actual(expected.size());
    return PKCS5_PBKDF2_HMAC(pin.c_str(), static_cast<int>(pin.size()), salt.data(), static_cast<int>(salt.size()),
                             iterations, EVP_sha256(), static_cast<int>(actual.size()), actual.data()) == 1 &&
           CRYPTO_memcmp(actual.data(), expected.data(), expected.size()) == 0;
}

static bool validPin(const string& p) {
    return p.size() == 4 && all_of(p.begin(), p.end(), [](unsigned char c){ return isdigit(c); });
}

static long long amount() {
    double value; cin >> value;
    if (cin.fail() || !isfinite(value) || value <= 0) {
        cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
        throw runtime_error("Invalid amount");
    }
    return llround(value * 100.0);
}

static string money(long long cents) {
    ostringstream out; out << fixed << setprecision(2) << cents / 100.0; return out.str();
}

static void bindText(sqlite3_stmt* s, int i, const string& v) { sqlite3_bind_text(s, i, v.c_str(), -1, SQLITE_TRANSIENT); }

class Bank {
    Db db;
    static constexpr int MAX_ATTEMPTS = 3;

    sqlite3_stmt* prepare(const char* sql) {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(db.raw(), sql, -1, &s, nullptr) != SQLITE_OK)
            throw runtime_error(sqlite3_errmsg(db.raw()));
        return s;
    }

    void audit(long long accountId, const string& action, const string& details) {
        sqlite3_stmt* s = prepare("INSERT INTO audit_logs(account_id,action,details) VALUES(?,?,?)");
        sqlite3_bind_int64(s, 1, accountId); bindText(s, 2, action); bindText(s, 3, details);
        sqlite3_step(s); sqlite3_finalize(s);
    }

    bool account(const string& number, long long& id, string& hash, long long& balance, int& frozen, int& attempts) {
        sqlite3_stmt* s = prepare("SELECT id,pin_hash,balance_cents,frozen,failed_attempts FROM accounts WHERE account_number=?");
        bindText(s, 1, number); bool found = false;
        if (sqlite3_step(s) == SQLITE_ROW) {
            found = true; id = sqlite3_column_int64(s,0); hash = reinterpret_cast<const char*>(sqlite3_column_text(s,1));
            balance = sqlite3_column_int64(s,2); frozen = sqlite3_column_int(s,3); attempts = sqlite3_column_int(s,4);
        }
        sqlite3_finalize(s); return found;
    }

    bool auth(const string& number, const string& pin, long long& id, long long& balance) {
        string hash; int frozen, attempts;
        if (!account(number,id,hash,balance,frozen,attempts) || frozen) return false;
        if (!verifyPin(pin, hash)) {
            ++attempts; int nowFrozen = attempts >= MAX_ATTEMPTS;
            sqlite3_stmt* s = prepare("UPDATE accounts SET failed_attempts=?, frozen=?, updated_at=CURRENT_TIMESTAMP WHERE id=?");
            sqlite3_bind_int(s,1,attempts); sqlite3_bind_int(s,2,nowFrozen); sqlite3_bind_int64(s,3,id); sqlite3_step(s); sqlite3_finalize(s);
            cout << "Invalid PIN. Attempts: " << attempts << '/' << MAX_ATTEMPTS << '\n';
            if (nowFrozen) cout << "Account frozen after too many failed attempts.\n";
            return false;
        }
        sqlite3_stmt* s = prepare("UPDATE accounts SET failed_attempts=0, updated_at=CURRENT_TIMESTAMP WHERE id=?");
        sqlite3_bind_int64(s,1,id); sqlite3_step(s); sqlite3_finalize(s); return true;
    }

    void recordTx(long long accountId, const string& type, long long amountCents, long long balanceAfter, const string& note) {
        sqlite3_stmt* s = prepare("INSERT INTO transactions(account_id,type,amount_cents,balance_after_cents,note) VALUES(?,?,?,?,?)");
        sqlite3_bind_int64(s,1,accountId); bindText(s,2,type); sqlite3_bind_int64(s,3,amountCents); sqlite3_bind_int64(s,4,balanceAfter); bindText(s,5,note);
        sqlite3_step(s); sqlite3_finalize(s);
    }

public:
    void create() {
        string number,name,phone,address,pin;
        cout << "Account Number: "; cin >> number; cin.ignore(numeric_limits<streamsize>::max(),'\n');
        cout << "Full Name: "; getline(cin,name); cout << "Phone: "; getline(cin,phone); cout << "Address: "; getline(cin,address);
        cout << "Create 4-digit PIN: "; cin >> pin;
        if (!validPin(pin)) throw runtime_error("PIN must contain exactly 4 digits");
        cout << "Initial Deposit: "; long long cents = amount();
        sqlite3_stmt* s = prepare("INSERT INTO accounts(account_number,name,phone,address,pin_hash,balance_cents) VALUES(?,?,?,?,?,?)");
        bindText(s,1,number); bindText(s,2,name); bindText(s,3,phone); bindText(s,4,address); bindText(s,5,hashPin(pin)); sqlite3_bind_int64(s,6,cents);
        if (sqlite3_step(s) != SQLITE_DONE) { string e=sqlite3_errmsg(db.raw()); sqlite3_finalize(s); throw runtime_error("Account creation failed: "+e); }
        long long id=sqlite3_last_insert_rowid(db.raw()); sqlite3_finalize(s); recordTx(id,"CREATE",cents,cents,"Initial deposit"); audit(id,"ACCOUNT_CREATED","Account created");
        cout << "Account created.\n";
    }

    void deposit() {
        string n,p; cout << "Account Number: "; cin>>n; cout<<"PIN: ";cin>>p; long long id,balance;
        if(!auth(n,p,id,balance)){cout<<"Authentication failed.\n";return;} cout<<"Amount: "; auto a=amount();
        db.begin(); try { balance+=a; sqlite3_stmt*s=prepare("UPDATE accounts SET balance_cents=?,updated_at=CURRENT_TIMESTAMP WHERE id=?");sqlite3_bind_int64(s,1,balance);sqlite3_bind_int64(s,2,id);sqlite3_step(s);sqlite3_finalize(s);recordTx(id,"DEPOSIT",a,balance,"Cash deposit");audit(id,"DEPOSIT","Amount "+money(a));db.commit();cout<<"Balance: $"<<money(balance)<<'\n'; } catch(...) {db.rollback();throw;}
    }

    void withdraw() {
        string n,p; cout<<"Account Number: ";cin>>n;cout<<"PIN: ";cin>>p;long long id,balance;
        if(!auth(n,p,id,balance)){cout<<"Authentication failed.\n";return;}cout<<"Amount: ";auto a=amount();if(a>balance){cout<<"Insufficient balance.\n";return;}
        db.begin();try{balance-=a;sqlite3_stmt*s=prepare("UPDATE accounts SET balance_cents=?,updated_at=CURRENT_TIMESTAMP WHERE id=?");sqlite3_bind_int64(s,1,balance);sqlite3_bind_int64(s,2,id);sqlite3_step(s);sqlite3_finalize(s);recordTx(id,"WITHDRAW",a,balance,"Cash withdrawal");audit(id,"WITHDRAW","Amount "+money(a));db.commit();cout<<"Balance: $"<<money(balance)<<'\n';}catch(...){db.rollback();throw;}
    }

    void transfer() {
        string from,p,to;cout<<"Sender Account: ";cin>>from;cout<<"PIN: ";cin>>p;long long sid,sbal;if(!auth(from,p,sid,sbal)){cout<<"Authentication failed.\n";return;}cout<<"Receiver Account: ";cin>>to;long long rid,rbal;string rh;int rf,ra;if(!account(to,rid,rh,rbal,rf,ra)||rf){cout<<"Receiver unavailable.\n";return;}if(sid==rid){cout<<"Cannot transfer to same account.\n";return;}cout<<"Amount: ";auto a=amount();if(a>sbal){cout<<"Insufficient balance.\n";return;}
        db.begin();try{sbal-=a;rbal+=a;sqlite3_stmt*s=prepare("UPDATE accounts SET balance_cents=?,updated_at=CURRENT_TIMESTAMP WHERE id=?");sqlite3_bind_int64(s,1,sbal);sqlite3_bind_int64(s,2,sid);sqlite3_step(s);sqlite3_reset(s);sqlite3_bind_int64(s,1,rbal);sqlite3_bind_int64(s,2,rid);sqlite3_step(s);sqlite3_finalize(s);recordTx(sid,"TRANSFER_OUT",a,sbal,"To "+to);recordTx(rid,"TRANSFER_IN",a,rbal,"From "+from);audit(sid,"TRANSFER_OUT","To "+to+" amount "+money(a));audit(rid,"TRANSFER_IN","From "+from+" amount "+money(a));db.commit();cout<<"Transfer completed.\n";}catch(...){db.rollback();throw;}
    }

    void info() {
        string n,p;cout<<"Account Number: ";cin>>n;cout<<"PIN: ";cin>>p;long long id,b; if(!auth(n,p,id,b)){cout<<"Authentication failed.\n";return;}
        sqlite3_stmt*s=prepare("SELECT name,phone,address,frozen,balance_cents FROM accounts WHERE id=?");sqlite3_bind_int64(s,1,id);if(sqlite3_step(s)==SQLITE_ROW){cout<<"Name: "<<sqlite3_column_text(s,0)<<"\nPhone: "<<sqlite3_column_text(s,1)<<"\nAddress: "<<sqlite3_column_text(s,2)<<"\nStatus: "<<(sqlite3_column_int(s,3)?"FROZEN":"ACTIVE")<<"\nBalance: $"<<money(sqlite3_column_int64(s,4))<<"\n";}sqlite3_finalize(s);
    }

    void history() {
        string n,p;cout<<"Account Number: ";cin>>n;cout<<"PIN: ";cin>>p;long long id,b;if(!auth(n,p,id,b)){cout<<"Authentication failed.\n";return;}
        sqlite3_stmt*s=prepare("SELECT created_at,type,amount_cents,balance_after_cents,note FROM transactions WHERE account_id=? ORDER BY id DESC");sqlite3_bind_int64(s,1,id);while(sqlite3_step(s)==SQLITE_ROW)cout<<sqlite3_column_text(s,0)<<" | "<<sqlite3_column_text(s,1)<<" | $"<<money(sqlite3_column_int64(s,2))<<" | balance $"<<money(sqlite3_column_int64(s,3))<<" | "<<sqlite3_column_text(s,4)<<'\n';sqlite3_finalize(s);
    }
};

int main(){ios::sync_with_stdio(false);cin.tie(nullptr);Bank bank;while(true){cout<<"\n==============================================\n PROFESSIONAL BANK SYSTEM 2026\n==============================================\n1. Create Account\n2. Deposit\n3. Withdraw\n4. Transfer\n5. Account Information\n6. Transaction History\n7. Exit\nChoice: ";int c;if(!(cin>>c)){cin.clear();cin.ignore(numeric_limits<streamsize>::max(),'\n');continue;}try{switch(c){case 1:bank.create();break;case 2:bank.deposit();break;case 3:bank.withdraw();break;case 4:bank.transfer();break;case 5:bank.info();break;case 6:bank.history();break;case 7:return 0;default:cout<<"Invalid choice.\n";}}catch(const exception&e){cerr<<"Error: "<<e.what()<<'\n';}}}
