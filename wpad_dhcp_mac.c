#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <CoreFoundation/CoreFoundation.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <SystemConfiguration/SCDynamicStoreCopyDHCPInfo.h>

#include "net_adapter.h"

char *wpad_dhcp_adapter_mac(uint8_t bind_ip[4], net_adapter_s *adapter, int32_t timeout_sec) {
    CFDictionaryRef dhcp_info = NULL;
    CFDataRef dhcp_wpad_url = NULL;
    char *wpad = NULL;

    // Get DHCP info for the primary adapter
    dhcp_info = SCDynamicStoreCopyDHCPInfo(NULL, NULL);
    if (!dhcp_info)
        return NULL;

    // Get the WPAD url from the DHCP server
    dhcp_wpad_url = DHCPInfoGetOptionData(dhcp_info, 252);
    if (dhcp_wpad_url)
        wpad = strdup((const char *)CFDataGetBytePtr(dhcp_wpad_url));
    CFRelease(dhcp_info);
    return wpad;
}
