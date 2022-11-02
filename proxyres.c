#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "config.h"
#include "execute.h"
#include "log.h"
#include "resolver.h"

#ifdef _WIN32
#include <windows.h>
#endif

bool proxyres_init(void) {
#ifdef _WIN32
    WSADATA WsaData = {0};
    if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0) {
        LOG_ERROR("Failed to initialize winsock %d\n", WSAGetLastError());
        return false;
    }
#endif
    
    if (!proxy_config_init())
        LOG_WARN("Failed to initialization proxy config\n");
    if (!proxy_resolver_init())
        LOG_WARN("Failed to initialization proxy resolver\n");
    if (!proxy_execute_init())
        LOG_WARN("Failed to initialization proxy execute\n");
    return true;
}

bool proxyres_uninit(void) {
    proxy_config_uninit();
    proxy_resolver_uninit();
    proxy_execute_uninit();
    
#ifdef _WIN32
    WSACleanup();
#endif
    return true;
}
