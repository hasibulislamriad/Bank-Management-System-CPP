# Authentication Security

## PIN hashing

The database layer must store only a salted, slow password hash. Never store plaintext PINs, salts shared between users, or reversible encrypted PINs.

Recommended production choices:

- Argon2id (preferred)
- bcrypt (acceptable when Argon2id is unavailable)
- PBKDF2-HMAC-SHA256 when platform constraints require it

Use a well-maintained cryptographic library rather than implementing a password-hashing algorithm yourself.

## Authentication controls

- Constant-time verification
- Per-account random salt
- Rate limiting / lockout
- Generic login errors to reduce account enumeration
- Audit successful and failed authentication events
- Never log PINs or password hashes
- Securely clear sensitive buffers where practical

## Migration

For an existing plaintext-PIN dataset, require users to set a new PIN rather than attempting to recover and re-hash plaintext credentials from the old file.
