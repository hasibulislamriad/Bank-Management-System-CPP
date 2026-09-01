# JWT + RBAC

## Roles

- `customer`: may access only their own account and permitted customer operations.
- `admin`: may access administrative statistics, account management, audit views, and other explicitly authorized admin endpoints.

## Token requirements

JWTs must be:

- Short-lived
- Signed with a vetted algorithm/library
- Validated for signature, issuer, audience, expiry, and not-before claims
- Issued only after successful authentication
- Free of PINs, password hashes, and other secrets

Signing keys must come from secure deployment configuration/secret management and must not be committed to Git.

## Authorization rule

Authentication answers “who are you?”; RBAC answers “what may you do?”. Every protected handler must perform both checks before invoking the service layer.

Customer account routes must enforce ownership. Admin routes must require the admin role.

## HTTP errors

- `401 Unauthorized`: missing/invalid/expired token
- `403 Forbidden`: valid identity but insufficient permission
- `400 Bad Request`: invalid JSON/input
- `409 Conflict`: business conflict/idempotency conflict
- `429 Too Many Requests`: rate limit exceeded
- `500 Internal Server Error`: unexpected server failure

Never expose PIN verification details or internal SQL errors to clients.
