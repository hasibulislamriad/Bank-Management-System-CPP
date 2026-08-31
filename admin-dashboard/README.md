# Admin Dashboard Architecture

Planned administrator dashboard for the bank system.

## Dashboard metrics

- Total accounts
- Active/frozen accounts
- Total deposits
- Today's transaction volume
- Recent transfers
- Failed authentication attempts
- Recent audit events

## Admin controls

- Search accounts
- Freeze/unfreeze account
- Review transactions
- Review audit logs
- Export reports

## Security

Dashboard access must use role-based access control. Money-moving operations should not be exposed as unauthenticated UI actions. Sensitive values such as PINs must never be displayed.

The dashboard is intentionally documented separately from the core C++ application so the eventual UI can consume a stable REST API.
