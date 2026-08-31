# WHMCS Client Service Tools Addon

A lightweight WHMCS addon-module example for administrators to inspect hosting service information from the WHMCS admin area.

## Features

- WHMCS addon module lifecycle functions
- Admin dashboard page
- Service lookup by Service ID
- Safe display of non-secret service/server metadata
- Uses WHMCS Capsule database layer
- CSRF token validation for admin actions
- Designed for WHMCS 8.x / PHP 7.4+

## Structure

```text
WHMCS-Addon-Module/
├── whmcs_service_tools.php
├── README.md
└── .gitignore
```

## Installation

1. Copy `whmcs_service_tools.php` to:
   `modules/addons/whmcsservicetools/whmcs_service_tools.php`
2. Create the directory if it does not exist.
3. In WHMCS go to **System Settings → Apps & Integrations**.
4. Find **WHMCS Service Tools** and activate it.
5. Grant access to the required administrator role.

> Never expose decrypted hosting passwords in an addon page. This module intentionally omits passwords and other secrets.

## Notes

The module is an educational/production-oriented starting point. Test it on a staging WHMCS installation before deploying it to production, and follow your WHMCS version's current developer documentation.
