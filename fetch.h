#pragma once

#ifdef __cplusplus
extern "C" {
#endif

char *fetch_get(const char *url, int32_t *error);

bool fetch_init(void);
bool fetch_uninit(void);

#ifdef __cplusplus
}
#endif
