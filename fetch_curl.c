#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>

#include "curl/curl.h"

#include "log.h"

typedef struct script_s {
    char *buffer;
    size_t size;
} script_s;

static size_t fetch_write_script(void *contents, size_t size, size_t nscriptb, void *userp) {
    script_s *script = (script_s *)userp;
    size_t realsize = size * nscriptb;

    char *ptr = realloc(script->buffer, script->size + realsize + 1);
    if (!ptr) {
        LOG_ERROR("Not enough memory to realloc\n");
        return 0;
    }

    script->buffer = ptr;
    memcpy(script->buffer + script->size, contents, realsize);
    script->size += realsize;
    script->buffer[script->size] = 0;

    return realsize;
}

// Fetch proxy auto configuration using CURL
char *fetch_get(const char *url, int32_t *error) {
    script_s script = {(char *)calloc(1, sizeof(char)), 0};

    CURL *curl_handle = curl_easy_init();
    if (!curl_handle) {
        LOG_ERROR("Unable to initialize curl handle\n");
        return NULL;
    }

    // Disable proxy when fetching PAC file
    curl_easy_setopt(curl_handle, CURLOPT_PROXY, "");

    // Add Accept header with PAC mime-type
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/x-ns-proxy-autoconfig");
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

    // Setup url to fetch
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

    // Write to memory buffer callback
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, fetch_write_script);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&script);

    CURLcode res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK) {
        LOG_ERROR("Unable to download URL: %s (%d)\n", url, res);
        free(script.buffer);
        script.buffer = NULL;
    }

    if (error)
        *error = res;

    curl_easy_cleanup(curl_handle);
    return script.buffer;
}

bool fetch_init(void) {
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if (res != CURLE_OK) {
        LOG_ERROR("Unable to initialize curl\n");
        return false;
    }
    return true;
}

bool fetch_uninit(void) {
    curl_global_cleanup();
    return true;
}
