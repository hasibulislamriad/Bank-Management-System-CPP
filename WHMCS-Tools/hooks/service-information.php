<?php

use WHMCS\View\Menu\Item as MenuItem;
use Illuminate\Database\Capsule\Manager as Capsule;

/**
 * WHMCS Client Area Service Information sidebar hook.
 *
 * Security note: this hook deliberately does NOT expose service passwords.
 */
add_hook('ClientAreaSecondarySidebar', 1, function (MenuItem $secondarySidebar) {
    $service = Menu::context('service');

    if (!$service) {
        return;
    }

    $username = trim((string) ($service->username ?? ''));
    $domain   = trim((string) ($service->domain ?? ''));
    $serverId = (int) ($service->server ?? 0);

    if ($username === '' && $domain === '' && $serverId === 0) {
        return;
    }

    $server = null;
    if ($serverId > 0) {
        $server = Capsule::table('tblservers')
            ->where('id', $serverId)
            ->first(['hostname', 'ipaddress', 'nameserver1', 'nameserver2']);
    }

    $panel = $secondarySidebar->addChild('service-information', [
        'label' => 'Service Information',
        'uri'   => '#',
        'icon'  => 'fas fa-server',
        'order' => 999,
    ]);

    $panel = $secondarySidebar->getChild('service-information');
    if (!$panel) {
        return;
    }

    $panel->moveToBack();

    if ($username !== '') {
        $panel->addChild('service-username', [
            'label' => $username,
            'icon'  => 'fas fa-user',
            'order' => 1,
        ]);
    }

    if ($domain !== '') {
        $panel->addChild('service-domain', [
            'label' => $domain,
            'icon'  => 'fas fa-globe',
            'order' => 2,
        ]);
    }

    if ($server) {
        $fields = [
            'service-ip' => ['Server IP', $server->ipaddress ?? '', 'fas fa-network-wired', 3],
            'service-hostname' => ['Hostname', $server->hostname ?? '', 'fas fa-server', 4],
            'service-ns1' => ['Nameserver 1', $server->nameserver1 ?? '', 'fas fa-info-circle', 5],
            'service-ns2' => ['Nameserver 2', $server->nameserver2 ?? '', 'fas fa-info-circle', 6],
        ];

        foreach ($fields as $id => [$label, $value, $icon, $order]) {
            $value = trim((string) $value);
            if ($value === '') {
                continue;
            }

            $panel->addChild($id, [
                'label' => $value,
                'icon'  => $icon,
                'order' => $order,
            ]);
        }
    }
});
