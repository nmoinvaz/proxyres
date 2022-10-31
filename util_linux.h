#pragma once

enum DESKTOP_ENV {
    DESKTOP_ENV_UNKNOWN,
    DESKTOP_ENV_KDE3,
    DESKTOP_ENV_KDE4,
    DESKTOP_ENV_KDE5,
    DESKTOP_ENV_GNOME2,
    DESKTOP_ENV_GNOME3,
    DESKTOP_ENV_XFCE,
    DESKTOP_ENV_WIN,
    DESKTOP_ENV_MACOS,
};

#ifdef __cplusplus
extern "C" {
#endif

int32_t get_desktop_env(void);

#ifdef __cplusplus
}
#endif
