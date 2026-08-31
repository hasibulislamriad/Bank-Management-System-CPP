# REST API Roadmap

Planned API layer for the bank system.

## Endpoints

- `POST /api/v1/auth/login`
- `POST /api/v1/accounts`
- `GET /api/v1/accounts/{accountNumber}`
- `POST /api/v1/accounts/{accountNumber}/deposit`
- `POST /api/v1/accounts/{accountNumber}/withdraw`
- `POST /api/v1/transfers`
- `GET /api/v1/accounts/{accountNumber}/transactions`

## Security requirements

Use HTTPS, short-lived authenticated sessions/tokens, authorization checks, request validation, rate limiting, audit logs, idempotency keys for money-moving requests, and server-side database transactions. Never return or log PINs.

This repository currently documents the API design; a web server framework and JSON library should be selected before implementing the HTTP layer.
