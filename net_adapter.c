#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "net_adapter.h"

static inline void print_ip(const char *name, uint8_t ip[4]) {
    printf("  %s: %d.%d.%d.%d\n", name, ip[0], ip[1], ip[2], ip[3]);
}

void net_adapter_print(net_adapter_s *adapter) {
    printf("adapter: %s\n", adapter->name);
    printf("  description: %s\n", adapter->description);
    if (adapter->mac_length) {
        printf("  mac: ");
        for (int32_t i = 0; i < adapter->mac_length; i++)
            printf("%02x", adapter->mac[i]);
        printf("\n");
    }
    print_ip("ip", adapter->ip);
    print_ip("netmask", adapter->netmask);
    print_ip("gateway", adapter->gateway);
    print_ip("primary dns", adapter->primary_dns);
    print_ip("secondary dns", adapter->secondary_dns);

    if (adapter->is_dhcp_v4)
        print_ip("dhcp", adapter->dhcp);
    if (adapter->is_connected)
        printf("  connected\n");
    if (adapter->is_loopback)
        printf("  loopback\n");
    if (adapter->is_ethernet)
        printf("  ethernet\n");
}
