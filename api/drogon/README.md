# Drogon REST API

This directory defines the HTTP layer for the bank service.

## Stack

- Drogon C++ web framework
- JSON request/response payloads
- Existing BankService/domain layer
- SQLite persistence

## API contract

`POST /api/v1/auth/login`

Request:
```json
{"account":"100001","pin":"1234"}
```

Response:
```json
{"token":"<short-lived-token>"}
```

`POST /api/v1/transfers`

Headers:
```text
Authorization: Bearer <token>
Idempotency-Key: <unique-request-id>
```

Request:
```json
{"to":"100002","amount":25.50}
```

The handler must validate authentication and authorization before invoking the service layer. Money-moving requests must execute as a single database transaction and must be safe to retry with the same idempotency key.

## Security

- HTTPS at deployment time
- Short-lived authentication tokens
- RBAC for customer/admin routes
- Rate limiting
- Strict JSON validation
- Generic authentication errors
- No PINs in logs or responses
- Audit every sensitive operation

The HTTP layer must not contain business rules or direct SQL; it should delegate to the service layer.
