#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Request WPAD configuration using DHCP using a particular network adapter
char *wpad_dhcp_adapter_win(uint8_t bind_ip[4], net_adapter_s *adapter, int32_t timeout_sec);

#ifdef __cplusplus
}
#endif