#include <stdint.h>
#include <stdbool.h>

#include <windows.h>
#include <dhcpcsdk.h>

#include "net_adapter.h"

// Request WPAD configuration using DHCP using a particular network adapter
char *wpad_dhcp_adapter(uint8_t bind_ip[4], net_adapter_s *adapter, int32_t timeout_sec) {
    DHCPCAPI_PARAMS wpad_params = {0, 252, false, NULL, 0};
    DHCPCAPI_PARAMS_ARRAY request_params = {1, &wpad_params};
    DHCPCAPI_PARAMS_ARRAY send_params = {0, NULL};
    uint8_t buffer[4096];
    DWORD buffer_len = sizeof(buffer);

    wchar_t *adapter_wide = utf8_dup_to_wchar(adapter->name);
    if (!adapter_wide)
        return NULL;

    DWORD err = DhcpRequestParams(DHCPCAPI_REQUEST_SYNCHRONOUS, NULL, adapter_wide, NULL, send_params, request_params,
                               buffer, &buffer_len, NULL);
    free(adapter_wide);

    if (err != NO_ERROR || wpad_params.nBytesData) {
        LOG_DEBUG("Error requesting WPAD from DHCP server (%d)\n", err);
        return NULL;
    }

    // Copy wpad url from buffer
    char *wpad = calloc(wpad_params.nBytesData + 1, sizeof(char));
    if (!wpad)
        return NULL;
    strncat(wpad, (char *)wpad_params.Data, wpad_params.nBytesData);

    // Remove any trailing new line character that some DHCP servers send
    str_trim_end(wpad, "\n");

    if (!*wpad) {
        free(wpad);
        return NULL;
    }

    return wpad;
}
