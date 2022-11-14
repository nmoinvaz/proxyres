#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "dns.h"
#include "fetch.h"
#include "log.h"
#include "util.h"
#include "util_socket.h"

char *wpad_dns(const char *fqdn) {
    char hostname[HOST_MAX] = {0};
    char wpad_host[HOST_MAX] = {0};
    int32_t error = 0;

    if (!fqdn) {
        // Get local hostname
        if (gethostname(hostname, sizeof(hostname)) == -1) {
            LOG_ERROR("Unable to get hostname (%d)\n", socketerr);
            return NULL;
        }
        hostname[sizeof(hostname) - 1] = 0;

        // Get hostent for local hostname
        struct hostent *localent = gethostbyname(hostname);
        if (!localent) {
            LOG_ERROR("Unable to get hostent for %s (%d)\n", hostname, socketerr);
            return NULL;
        }

        // Canonical name found in h_name
        fqdn = localent->h_name;

        // Validate canonical name
        if (!fqdn || !strlen(fqdn) || !strcmp(fqdn, "localhost") || isdigit(*fqdn)) {
            LOG_DEBUG("Unable to get canonical name for %s (%d)\n", hostname, socketerr);
            return NULL;
        }
    }

    // Enumerate through each part of the FQDN
    const char *name = fqdn;
    char *next_part;
    do {
        next_part = strchr(name, '.');
        if (!next_part)
            break;
        next_part++;

        // Construct WPAD url with next part of FQDN
        snprintf(wpad_host, sizeof(wpad_host), "wpad.%s", name);
        LOG_DEBUG("Checking next WPAD hostname: %s\n", wpad_host);

        if (dns_resolve(wpad_host, &error)) {
            char *wpad_url = (char *)calloc(HOST_MAX + 32, sizeof(char));
            if (wpad_url)
                snprintf(wpad_url, sizeof(wpad_url), "http://%s/wpad.dat", wpad_host);
            return wpad_url;
        }

        LOG_DEBUG("No server found at %s (%d)\n", wpad_host, error);
        name = next_part;
    } while (true);

    return NULL;
}