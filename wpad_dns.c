#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "fetch.h"
#include "log.h"
#include "util.h"
#include "util_socket.h"

char *wpad_dns(const char *fqdn) {
    char hostname[HOST_MAX] = {0};
    char wpad_url[HOST_MAX + 32] = {0};
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
        snprintf(wpad_url, sizeof(wpad_url), "http://wpad.%s/wpad.dat", name);
        LOG_DEBUG("Checking next WPAD DNS url: %s\n", wpad_url);

        char *script = fetch_get(wpad_url, &error);
        if (script)
            return script;

        LOG_DEBUG("No wpad.dat found at %s (%d)\n", wpad_url, error);
        name = next_part;
    } while (true);

    return NULL;
}