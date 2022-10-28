
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <errno.h>
#include <dlfcn.h>
#include <gio/gio.h>
#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif

#include "resolver.h"
#include "resolver_i.h"
#include "resolver_gnome3.h"

typedef struct g_proxy_resolver_gnome3_s {
    // GIO module handle
    void *gio_module;
    // GIO cancellable functions
    void (*g_object_unref)(gpointer object);
    GCancellable *(*g_cancellable_new)(void);
    void (*g_cancellable_cancel)(GCancellable *cancellable);
    // GProxy resolution functions
    gboolean (*g_proxy_resolver_is_supported)(GProxyResolver *resolver);
    GProxyResolver *(*g_proxy_resolver_get_default)(void);
    gchar **(*g_proxy_resolver_lookup)(GProxyResolver *resolver, const gchar *uri, GCancellable *cancellable,
                                       GError *error);
    void (*g_proxy_resolver_lookup_async)(GProxyResolver *resolver, const gchar *uri, GCancellable *cancellable,
                                          GAsyncReadyCallback callback, gpointer user_data);
    gchar **(*g_proxy_resolver_lookup_finish)(GProxyResolver *resolver, GAsyncResult *result, GError **error);
    // Glib module handle
    void *glib_module;
    // Glib functions
    gint (*g_strv_length)(gchar **str_array);
    void (*g_strfreev)(gchar **str_array);
    void (*g_error_free)(GError *error);
} g_proxy_resolver_gnome3_s;

g_proxy_resolver_gnome3_s g_proxy_resolver_gnome3;

typedef struct proxy_resolver_gnome3_s {
    // GProxy resolver
    GProxyResolver *resolver;
    // Last system error
    int32_t error;
    // Resolution pending
    bool pending;
    // Cancellable object
    GCancellable *cancellable;
    // Resolve callback
    void *user_data;
    proxy_resolver_resolved_cb callback;
    // Proxy list
    char *list;
#ifdef HAVE_PTHREADS
    // Thread variables
    pthread_t thread;
    char *url;
#endif
} proxy_resolver_gnome3_s;

static void proxy_resolver_gnome3_cleanup(proxy_resolver_gnome3_s *proxy_resolver) {
    free(proxy_resolver->list);
    proxy_resolver->list = NULL;
    free(proxy_resolver->url);
    proxy_resolver->url = NULL;

    if (proxy_resolver->cancellable) {
        g_proxy_resolver_gnome3.g_object_unref(proxy_resolver->cancellable);
        proxy_resolver->cancellable = NULL;
    }

    if (proxy_resolver->resolver) {
        g_proxy_resolver_gnome3.g_object_unref(proxy_resolver->resolver);
        proxy_resolver->resolver = NULL;
    }
}

static void proxy_resolver_gnome3_reset(proxy_resolver_gnome3_s *proxy_resolver) {
    proxy_resolver->pending = false;
    proxy_resolver->error = 0;

    proxy_resolver_gnome3_cleanup(proxy_resolver);
}

static bool proxy_resolver_gnome3_get_proxies(proxy_resolver_gnome3_s *proxy_resolver, char **proxies, GError *error) {
    int32_t proxy_count = g_proxy_resolver_gnome3.g_strv_length(proxies);
    int32_t max_list = (proxy_count + 1) * MAX_PROXY_URL;

    if (!proxies) {
        proxy_resolver->error = error->code;
        printf("Unable to get proxies for list (%d:%s)\n", proxy_resolver->error, error->message);
        return false;
    }

    // Allocate memory for proxy list
    proxy_resolver->list = calloc(max_list, sizeof(char));
    if (!proxy_resolver->list) {
        proxy_resolver->error = ENOMEM;
        printf("Unable to allocate memory for list (%d)\n", proxy_resolver->error);
        return false;
    }

    int32_t list_len = 0;

    for (int32_t i = 0; proxies[i] && i < proxy_count; i++) {
        if (strstr(proxies[i], "direct") == proxies[i]) {
            strncat(proxy_resolver->list, "DIRECT", max_list - list_len);
            list_len += 6;
        } else if (strstr(proxies[i], "http") == proxies[i]) {
            strncat(proxy_resolver->list, "PROXY ", max_list - list_len);
            proxy_resolver->list[max_list - 1] = 0;
            list_len += 6;
            strncat(proxy_resolver->list, proxies[i], max_list - list_len);
            list_len += strlen(proxies[i]);
        }
        proxy_resolver->list[max_list - 1] = 0;

        if (i != proxy_count - 1) {
            // Append semi-colon separator
            strncat(proxy_resolver->list, ";", max_list - list_len);
            proxy_resolver->list[max_list - 1] = 0;
            list_len++;
        }
    }

    return true;
}

#ifndef HAVE_PTHREADS
void proxy_resolver_gnome3_async_ready_callback(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    proxy_resolver_gnome3_s *proxy_resolver = (proxy_resolver_gnome3_s *)user_data;
    GError *error = NULL;
    char *list = NULL;
    char **proxies;

    // Get list of proxies from resolver
    proxies = g_proxy_resolver_gnome3.g_proxy_resolver_lookup_finish(proxy_resolver->resolver, res, &error);
    proxy_resolver_gnome3_read_proxies(proxy_resolver, proxies);

gnome3_done:
    proxy_resolver->pending = false;

    // Trigger resolve callback
    if (proxy_resolver->callback)
        proxy_resolver->callback(proxy_resolver, proxy_resolver->user_data, proxy_resolver->error,
                                 proxy_resolver->list);

    if (error)
        g_proxy_resolver_gnome3.g_error_free(error);
    if (proxies)
        g_proxy_resolver_gnome3.g_strfreev(proxies);

    proxy_resolver_gnome3_cleanup(proxy_resolver);
}

bool proxy_resolver_gnome3_get_proxies_for_url(void *ctx, const char *url) {
    proxy_resolver_gnome3_s *proxy_resolver = (proxy_resolver_gnome3_s *)ctx;

    proxy_resolver_gnome3_reset(proxy_resolver);

    // Get reference to the default proxy resolver
    proxy_resolver->resolver = g_proxy_resolver_gnome3.g_proxy_resolver_get_default();
    if (!proxy_resolver->resolver) {
        proxy_resolver->error = ENOMEM;
        printf("Unable to create resolver object (%d)\n", proxy_resolver->error);
        goto gnome3_error;
    }

    // Create cancellable object in case we need to cancel operation
    proxy_resolver->cancellable = g_proxy_resolver_gnome3.g_cancellable_new();
    if (!proxy_resolver->cancellable) {
        proxy_resolver->error = ENOMEM;
        printf("Unable to create cancellable object (%d)\n", proxy_resolver->error);
        goto gnome3_error;
    }

    proxy_resolver->pending = true;
    g_proxy_resolver_gnome3.g_proxy_resolver_lookup(proxy_resolver->resolver, url, proxy_resolver->cancellable, NULL);

    // Start async proxy resolution
    g_proxy_resolver_gnome3.g_proxy_resolver_lookup_async(proxy_resolver->resolver, url, proxy_resolver->cancellable,
                                                          proxy_resolver_gnome3_async_ready_callback, proxy_resolver);

    return true;

gnome3_error:

    // Trigger resolve callback
    if (proxy_resolver->callback)
        proxy_resolver->callback(proxy_resolver, proxy_resolver->user_data, proxy_resolver->error,
                                 proxy_resolver->list);

    proxy_resolver_gnome3_cleanup(proxy_resolver);
    return false;
}
#else
static void *proxy_resolver_gnome3_get_proxies_for_url_thread(void *user_data) {
    proxy_resolver_gnome3_s *proxy_resolver = (proxy_resolver_gnome3_s *)user_data;
    GError *error = NULL;
    char **proxies = NULL;

    proxies = g_proxy_resolver_gnome3.g_proxy_resolver_lookup(proxy_resolver->resolver, proxy_resolver->url,
                                                              proxy_resolver->cancellable, &error);
    proxy_resolver_gnome3_get_proxies(proxy_resolver, proxies, error);

    if (error)
        g_proxy_resolver_gnome3.g_error_free(error);
    if (proxies)
        g_proxy_resolver_gnome3.g_strfreev(proxies);

    proxy_resolver->pending = false;
    proxy_resolver->thread = 0;

    // Trigger resolve callback
    if (proxy_resolver->callback)
        proxy_resolver->callback(proxy_resolver, proxy_resolver->user_data, proxy_resolver->error,
                                 proxy_resolver->list);

    proxy_resolver_gnome3_cleanup(proxy_resolver);
}

bool proxy_resolver_gnome3_get_proxies_for_url(void *ctx, const char *url) {
    proxy_resolver_gnome3_s *proxy_resolver = (proxy_resolver_gnome3_s *)ctx;

    if (proxy_resolver->thread)
        return false;

    proxy_resolver_gnome3_reset(proxy_resolver);

    // Get reference to the default proxy resolver
    proxy_resolver->resolver = g_proxy_resolver_gnome3.g_proxy_resolver_get_default();
    if (!proxy_resolver->resolver) {
        proxy_resolver->error = ENOMEM;
        printf("Unable to create resolver object (%d)\n", proxy_resolver->error);
        goto gnome3_error;
    }

    // Create cancellable object in case we need to cancel operation
    proxy_resolver->cancellable = g_proxy_resolver_gnome3.g_cancellable_new();
    if (!proxy_resolver->cancellable) {
        proxy_resolver->error = ENOMEM;
        printf("Unable to create cancellable object (%d)\n", proxy_resolver->error);
        goto gnome3_error;
    }

    proxy_resolver->pending = true;
    proxy_resolver->url = strdup(url);

    pthread_create(&proxy_resolver->thread, NULL, proxy_resolver_gnome3_get_proxies_for_url_thread, proxy_resolver);
    return true;

gnome3_error:

    // Trigger resolve callback
    if (proxy_resolver->callback)
        proxy_resolver->callback(proxy_resolver, proxy_resolver->user_data, proxy_resolver->error,
                                 proxy_resolver->list);

    proxy_resolver_gnome3_cleanup(proxy_resolver);
    return false;
}
#endif

bool proxy_resolver_gnome3_get_list(void *ctx, char **list) {
    proxy_resolver_gnome3_s *proxy_resolver = (proxy_resolver_gnome3_s *)ctx;
    if (!proxy_resolver || !list)
        return false;
    *list = proxy_resolver->list;
    return (*list != NULL);
}

bool proxy_resolver_gnome3_get_error(void *ctx, int32_t *error) {
    proxy_resolver_gnome3_s *proxy_resolver = (proxy_resolver_gnome3_s *)ctx;
    if (!proxy_resolver || !error)
        return false;
    *error = proxy_resolver->error;
    return true;
}

bool proxy_resolver_gnome3_is_pending(void *ctx) {
    proxy_resolver_gnome3_s *proxy_resolver = (proxy_resolver_gnome3_s *)ctx;
    if (!proxy_resolver)
        return false;
    return proxy_resolver->pending;
}

bool proxy_resolver_gnome3_cancel(void *ctx) {
    proxy_resolver_gnome3_s *proxy_resolver = (proxy_resolver_gnome3_s *)ctx;
    if (!proxy_resolver)
        return false;
    if (proxy_resolver->cancellable)
        g_proxy_resolver_gnome3.g_cancellable_cancel(proxy_resolver->cancellable);
    return true;
}

bool proxy_resolver_gnome3_set_resolved_callback(void *ctx, void *user_data, proxy_resolver_resolved_cb callback) {
    proxy_resolver_gnome3_s *proxy_resolver = (proxy_resolver_gnome3_s *)ctx;
    if (!proxy_resolver)
        return false;
    proxy_resolver->user_data = user_data;
    proxy_resolver->callback = callback;
    return true;
}

bool proxy_resolver_gnome3_create(void **ctx) {
    proxy_resolver_gnome3_s *proxy_resolver = (proxy_resolver_gnome3_s *)calloc(1, sizeof(proxy_resolver_gnome3_s));
    if (!proxy_resolver)
        return false;
    *ctx = proxy_resolver;
    return true;
}

bool proxy_resolver_gnome3_delete(void **ctx) {
    proxy_resolver_gnome3_s *proxy_resolver;
    if (ctx == NULL)
        return false;
    proxy_resolver = (proxy_resolver_gnome3_s *)*ctx;
    if (!proxy_resolver)
        return false;
    proxy_resolver_cancel(ctx);
    proxy_resolver_gnome3_cleanup(proxy_resolver);
    free(proxy_resolver);
    return true;
}

bool proxy_resolver_gnome3_init(void) {
    g_proxy_resolver_gnome3.gio_module = dlopen("libgio-2.0.so.0", RTLD_LAZY | RTLD_LOCAL);
    if (!g_proxy_resolver_gnome3.gio_module)
        goto gnome3_init_error;
    g_proxy_resolver_gnome3.glib_module = dlopen("libglib-2.0.so.0", RTLD_LAZY | RTLD_LOCAL);
    if (!g_proxy_resolver_gnome3.glib_module)
        goto gnome3_init_error;

    // Glib functions
    g_proxy_resolver_gnome3.g_error_free = dlsym(g_proxy_resolver_gnome3.glib_module, "g_error_free");
    if (!g_proxy_resolver_gnome3.g_error_free)
        goto gnome3_init_error;
    g_proxy_resolver_gnome3.g_strv_length = dlsym(g_proxy_resolver_gnome3.glib_module, "g_strv_length");
    if (!g_proxy_resolver_gnome3.g_strv_length)
        goto gnome3_init_error;
    g_proxy_resolver_gnome3.g_strfreev = dlsym(g_proxy_resolver_gnome3.glib_module, "g_strfreev");
    if (!g_proxy_resolver_gnome3.g_strfreev)
        goto gnome3_init_error;

    // GIO cancellable functions
    g_proxy_resolver_gnome3.g_object_unref = dlsym(g_proxy_resolver_gnome3.gio_module, "g_object_unref");
    if (!g_proxy_resolver_gnome3.g_object_unref)
        goto gnome3_init_error;
    g_proxy_resolver_gnome3.g_cancellable_new = dlsym(g_proxy_resolver_gnome3.gio_module, "g_cancellable_new");
    if (!g_proxy_resolver_gnome3.g_cancellable_new)
        goto gnome3_init_error;
    g_proxy_resolver_gnome3.g_cancellable_cancel = dlsym(g_proxy_resolver_gnome3.gio_module, "g_cancellable_cancel");
    if (!g_proxy_resolver_gnome3.g_cancellable_cancel)
        goto gnome3_init_error;

    // GProxyResolver functions
    g_proxy_resolver_gnome3.g_proxy_resolver_is_supported =
        dlsym(g_proxy_resolver_gnome3.gio_module, "g_proxy_resolver_is_supported");
    if (!g_proxy_resolver_gnome3.g_proxy_resolver_is_supported)
        goto gnome3_init_error;
    g_proxy_resolver_gnome3.g_proxy_resolver_get_default =
        dlsym(g_proxy_resolver_gnome3.gio_module, "g_proxy_resolver_get_default");
    if (!g_proxy_resolver_gnome3.g_proxy_resolver_get_default)
        goto gnome3_init_error;
    g_proxy_resolver_gnome3.g_proxy_resolver_lookup =
        dlsym(g_proxy_resolver_gnome3.gio_module, "g_proxy_resolver_lookup");
    if (!g_proxy_resolver_gnome3.g_proxy_resolver_lookup)
        goto gnome3_init_error;
    g_proxy_resolver_gnome3.g_proxy_resolver_lookup_async =
        dlsym(g_proxy_resolver_gnome3.gio_module, "g_proxy_resolver_lookup_async");
    if (!g_proxy_resolver_gnome3.g_proxy_resolver_lookup_async)
        goto gnome3_init_error;
    g_proxy_resolver_gnome3.g_proxy_resolver_lookup_finish =
        dlsym(g_proxy_resolver_gnome3.gio_module, "g_proxy_resolver_lookup_finish");
    if (!g_proxy_resolver_gnome3.g_proxy_resolver_lookup_finish)
        goto gnome3_init_error;

    return true;

gnome3_init_error:
    proxy_resolver_gnome3_uninit();
    return false;
}

bool proxy_resolver_gnome3_uninit(void) {
    if (g_proxy_resolver_gnome3.gio_module)
        dlclose(g_proxy_resolver_gnome3.gio_module);
    if (g_proxy_resolver_gnome3.glib_module)
        dlclose(g_proxy_resolver_gnome3.glib_module);

    memset(&g_proxy_resolver_gnome3, 0, sizeof(g_proxy_resolver_gnome3));
    return true;
}

proxy_resolver_i_s *proxy_resolver_gnome3_get_interface(void) {
    static proxy_resolver_i_s proxy_resolver_gnome3_i = {proxy_resolver_gnome3_get_proxies_for_url,
                                                         proxy_resolver_gnome3_get_list,
                                                         proxy_resolver_gnome3_get_error,
                                                         proxy_resolver_gnome3_is_pending,
                                                         proxy_resolver_gnome3_cancel,
                                                         proxy_resolver_gnome3_set_resolved_callback,
                                                         proxy_resolver_gnome3_create,
                                                         proxy_resolver_gnome3_delete,
                                                         proxy_resolver_gnome3_init,
                                                         proxy_resolver_gnome3_uninit};
    return &proxy_resolver_gnome3_i;
}
