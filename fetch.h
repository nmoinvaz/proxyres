#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Downloads a PAC script
char *fetch_get(const char *url, int32_t *error);

// Initialize URL fetching
bool fetch_init(void);

// Uninitialize URL fetching
bool fetch_uninit(void);

#ifdef __cplusplus
}
#endif
