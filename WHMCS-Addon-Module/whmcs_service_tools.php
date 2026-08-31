<?php

if (!defined('WHMCS')) {
    die('This file cannot be accessed directly');
}

use Illuminate\Database\Capsule\Manager as Capsule;

/**
 * WHMCS Service Tools
 *
 * Small WHMCS addon-module example for administrator-side service lookup.
 * Secrets such as service passwords are intentionally never displayed.
 */

function whmcsservicetools_config()
{
    return [
        'name' => 'WHMCS Service Tools',
        'description' => 'Admin-side service and server information lookup.',
        'version' => '1.0.0',
        'author' => 'Hasibul Islam Riad',
        'language' => 'english',
    ];
}

function whmcsservicetools_activate()
{
    return [
        'status' => 'success',
        'description' => 'WHMCS Service Tools activated successfully.',
    ];
}

function whmcsservicetools_deactivate()
{
    return [
        'status' => 'success',
        'description' => 'WHMCS Service Tools deactivated successfully.',
    ];
}

function whmcsservicetools_output($vars)
{
    $serviceId = isset($_REQUEST['serviceid']) ? (int) $_REQUEST['serviceid'] : 0;
    $csrfToken = isset($_REQUEST['token']) ? (string) $_REQUEST['token'] : '';

    echo '<h2>WHMCS Service Tools</h2>';
    echo '<p>Look up non-secret information for a WHMCS hosting service.</p>';

    echo '<form method="get" action="addonmodules.php" class="form-inline">';
    echo '<input type="hidden" name="module" value="whmcsservicetools">';
    echo '<input type="hidden" name="token" value="' . htmlspecialchars(generate_token('plain'), ENT_QUOTES, 'UTF-8') . '">';
    echo '<div class="form-group">';
    echo '<label for="serviceid">Service ID&nbsp;</label>';
    echo '<input id="serviceid" name="serviceid" type="number" min="1" class="form-control" value="' . htmlspecialchars((string) $serviceId, ENT_QUOTES, 'UTF-8') . '" required>'; 
    echo '</div>&nbsp;';
    echo '<button type="submit" class="btn btn-primary">Search</button>';
    echo '</form>';

    if ($serviceId <= 0) {
        return;
    }

    if ($csrfToken === '' || !check_token('WHMCS.admin.default')) {
        echo '<div class="alert alert-danger" style="margin-top:20px;">Invalid security token.</div>';
        return;
    }

    $service = Capsule::table('tblhosting as h')
        ->leftJoin('tblproducts as p', 'p.id', '=', 'h.packageid')
        ->leftJoin('tblservers as s', 's.id', '=', 'h.server')
        ->where('h.id', $serviceId)
        ->first([
            'h.id',
            'h.userid',
            'h.domain',
            'h.username',
            'h.domainstatus',
            'h.billingcycle',
            'h.nextduedate',
            'p.name as product_name',
            's.hostname as server_hostname',
            's.ipaddress as server_ip',
            's.nameserver1',
            's.nameserver2',
        ]);

    if (!$service) {
        echo '<div class="alert alert-warning" style="margin-top:20px;">Service not found.</div>';
        return;
    }

    $rows = [
        'Service ID' => $service->id,
        'Client ID' => $service->userid,
        'Product' => $service->product_name ?: 'N/A',
        'Domain' => $service->domain ?: 'N/A',
        'Username' => $service->username ?: 'N/A',
        'Status' => $service->domainstatus ?: 'N/A',
        'Billing Cycle' => $service->billingcycle ?: 'N/A',
        'Next Due Date' => $service->nextduedate ?: 'N/A',
        'Server Hostname' => $service->server_hostname ?: 'N/A',
        'Server IP' => $service->server_ip ?: 'N/A',
        'Nameserver 1' => $service->nameserver1 ?: 'N/A',
        'Nameserver 2' => $service->nameserver2 ?: 'N/A',
    ];

    echo '<hr>';
    echo '<h3>Service Information</h3>';
    echo '<table class="table table-striped table-bordered" style="max-width:900px;">';

    foreach ($rows as $label => $value) {
        echo '<tr>';
        echo '<th style="width:220px;">' . htmlspecialchars($label, ENT_QUOTES, 'UTF-8') . '</th>';
        echo '<td>' . htmlspecialchars((string) $value, ENT_QUOTES, 'UTF-8') . '</td>';
        echo '</tr>';
    }

    echo '</table>';
    echo '<div class="alert alert-info">Passwords and other decrypted secrets are intentionally excluded.</div>';
}
