# SQLite + PIN Hashing Roadmap

The schema in `schema.sql` is ready for the next database-backed implementation.

## Database design

- `accounts`: identity, account state and balance
- `transactions`: immutable transaction records
- `audit_logs`: security and administrative audit events

## PIN security

The current console implementation still uses a local demo PIN field. The database migration should replace that with a salted password/PIN hash using a vetted password hashing library such as Argon2id or bcrypt. Do not implement a custom cryptographic hash.

## Transaction safety

Money movement should run inside one SQLite transaction: validate accounts, update both balances, insert the corresponding ledger records, then COMMIT. Any failure must ROLLBACK the entire operation.
