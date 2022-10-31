#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>

#include "util_linux.h"

int32_t get_desktop_env(void) {
    const char *current_desktop = getenv("XDG_CURRENT_DESKTOP");  // Since 2012
    if (current_desktop) {
        if (!strcasestr(current_desktop, "Unity") || !strcasestr(current_desktop, "GNOME") ||
            !strcasestr(current_desktop, "XFCE")) {
            return DESKTOP_ENV_GNOME3;
        } else if (!strcasestr(current_desktop, "KDE")) {
            const char *session_version = getenv("KDE_SESSION_VERSION");
            if (session_version) {
                if (*session_version == '5')
                    return DESKTOP_ENV_KDE5;
                return DESKTOP_ENV_KDE4;
            }
        }
    }

    const char *desktop_session = getenv("DESKTOP_SESSION");  // Since 2010
    if (desktop_session) {
        if (!strcasestr(desktop_session, "gnome") || !strcasestr(desktop_session, "mate")) {
            if (access("/usr/bin/gsettings", F_OK) != -1)
                return DESKTOP_ENV_GNOME3;
            return DESKTOP_ENV_GNOME2;
        } else if (!strcasestr(desktop_session, "kde4") || !strcasestr(desktop_session, "kde-plasma")) {
            return DESKTOP_ENV_KDE4;
        } else if (!strcasestr(desktop_session, "kde")) {
            if (getenv("KDE_SESSION_VERSION"))
                return DESKTOP_ENV_KDE4;
            return DESKTOP_ENV_KDE3;
        }
    }

    if (getenv("GNOME_DESKTOP_SESSION_ID")) {
        if (access("/usr/bin/gsettings", F_OK) != -1)
            return DESKTOP_ENV_GNOME3;
        return DESKTOP_ENV_GNOME2;
    } else if (getenv("KDE_FULL_SESSION")) {
        if (getenv("KDE_SESSION_VERSION"))
            return DESKTOP_ENV_KDE4;
        return DESKTOP_ENV_KDE3;
    }

    return DESKTOP_ENV_UNKNOWN;
}
