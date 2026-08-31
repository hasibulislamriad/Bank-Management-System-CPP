# Tests

The CI pipeline builds the C++20 application and runs the executable smoke-test entry point.

The next test expansion should cover:

- PIN hashing and verification
- Wrong-PIN lockout
- Account creation
- Deposit and withdrawal balance invariants
- Transfer atomicity
- SQLite rollback on injected failure
- Duplicate account handling
- Invalid/zero/overflow amounts
- Frozen account authorization
- Audit-log creation

Tests must use an isolated temporary database and must never contain real credentials or banking data.
