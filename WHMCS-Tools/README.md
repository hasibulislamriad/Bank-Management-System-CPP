# WHMCS Tools & Hooks

Small production-oriented WHMCS snippets for Client Area customization.

## Included

### `hooks/service-information.php`
Adds a Service Information panel to the Client Area secondary sidebar with username, domain, server hostname, IP address and nameservers.

The password is intentionally not exposed by the hook. Credentials should not be embedded in page HTML or JavaScript.

## Install

Copy the PHP hook file into:

```text
WHMCS/includes/hooks/service-information.php
```

Test on a staging installation before production deployment.

## Compatibility

Designed for WHMCS 8.x style hooks and PHP 8.x compatible syntax.
