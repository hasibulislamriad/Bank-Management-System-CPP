# 🏦 Professional Bank Management System - C++

A console-based banking application built with **C++**, **Object-Oriented Programming (OOP)**, STL, and file handling. It demonstrates practical banking operations with PIN authentication, persistent account data, transfers, and transaction history.

## ✨ Features

- 🆕 Create Account
- 🔐 4-digit PIN authentication
- 💰 Deposit Money
- 💸 Withdraw Money
- 💳 Check Balance
- 🔄 Money Transfer between accounts
- 👤 View Account Information
- 📜 Transaction History
- ✏️ Update Phone and Address
- 🗑️ Delete zero-balance accounts
- 📊 Bank Statistics
- 💾 Persistent storage with text files
- 🛡️ Basic input validation

## 🛠️ Technologies

- C++17 or later
- Object-Oriented Programming
- STL `vector`
- File I/O with `fstream`
- String processing
- Date/time handling

## 📁 Project Structure

```text
Bank-Management-System-CPP/
├── main.cpp
├── README.md
├── .gitignore
├── accounts.txt
└── transactions.txt
```

`accounts.txt` and `transactions.txt` are runtime data files and are created automatically when the program saves data.

## ▶️ Run the Project

### Compile

```bash
g++ -std=c++17 main.cpp -o bank
```

### Linux / macOS

```bash
./bank
```

### Windows

```bash
bank.exe
```

## 📋 Main Menu

```text
1. Create Account
2. Deposit Money
3. Withdraw Money
4. Check Balance
5. Money Transfer
6. Account Information
7. Transaction History
8. Update Account
9. Delete Account
10. Bank Statistics
11. Exit
```

## 🧠 Concepts Demonstrated

- Classes and objects
- Constructors
- Encapsulation
- STL vectors
- CRUD operations
- File persistence
- Authentication logic
- Transaction logging
- Input validation
- Searching and updating records

## ⚠️ Educational Project

This project is designed for learning and portfolio purposes. It is **not suitable for handling real financial data**. A production banking application would require secure password/PIN hashing, encryption, database transactions, authorization controls, audit logging, and many additional security measures.

## 👨‍💻 Author

**Hasibul Islam Riad**

GitHub: https://github.com/hasibulislamriad
