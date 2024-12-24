#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <dlfcn.h>
#include <glib-object.h>
#include <link.h>

#include <jsc/jsc.h>

#include "execute.h"
#include "execute_i.h"
#include "execute_jsc.h"
#include "log.h"
#include "mozilla_js.h"
#include "net_util.h"
#include "util.h"

typedef struct g_proxy_execute_jsc_s {
    // JSCoreGTK module
    void *module;
    // Context functions
    JSCContext *(*jsc_context_new)(void);
    JSCValue *(*jsc_context_get_global_object)(JSCContext *context);
    JSCValue *(*jsc_context_evaluate)(JSCContext *context, const char *code, gssize length);
    JSCException *(*jsc_context_get_exception)(JSCContext *context);
    // Value functions
    gboolean (*jsc_value_is_string)(JSCValue *value);
    gboolean (*jsc_value_is_number)(JSCValue *value);
    gboolean (*jsc_value_is_object)(JSCValue *value);
    double (*jsc_value_to_double)(JSCValue *value);
    JSCValue *(*jsc_value_new_string)(JSCContext *context, const char *string);
    char *(*jsc_value_to_string)(JSCValue *value);
    JSCValue *(*jsc_value_new_function)(JSCContext *context, const char *name, GCallback callback, gpointer user_data,
                                        GDestroyNotify destroy_notify, GType return_type, guint n_params, ...);
    void (*jsc_value_object_set_property)(JSCValue *value, const char *name, JSCValue *property);
    JSCValue *(*jsc_value_object_get_property)(JSCValue *value, const char *name);
    // Exception functions
    char *(*jsc_exception_report)(JSCException *exception);
} g_proxy_execute_jsc_s;

g_proxy_execute_jsc_s g_proxy_execute_jsc;
static pthread_once_t g_proxy_execute_jsc_init_flag = PTHREAD_ONCE_INIT;

typedef struct proxy_execute_jsc_s {
    // Execute error
    int32_t error;
    // Proxy list
    char *list;
} proxy_execute_jsc_s;

static void js_print_exception(JSCContext *context, JSCException *exception) {
    if (!exception)
        return;

    char *report = g_proxy_execute_jsc.jsc_exception_report(exception);
    if (report) {
        printf("EXCEPTION: %s\n", report);
        free(report);
        return;
    }

    LOG_ERROR("Unable to print exception object\n");
}

static char *proxy_execute_jsc_dns_resolve(const char *host) {
    if (!host)
        return NULL;

    return dns_resolve(host, NULL);
}

static char *proxy_execute_jsc_dns_resolve_ex(const char *host) {
    if (!host)
        return NULL;

    return dns_resolve_ex(host, NULL);
}

static char *proxy_execute_jsc_my_ip_address(void) {
    return my_ip_address();
}

static char *proxy_execute_jsc_my_ip_address_ex(void) {
    return my_ip_address_ex();
}

bool proxy_execute_jsc_get_proxies_for_url(void *ctx, const char *script, const char *url) {
    proxy_execute_jsc_s *proxy_execute = (proxy_execute_jsc_s *)ctx;
    JSCContext *global = NULL;
    JSCException *exception = NULL;
    char find_proxy[4096];
    bool is_ok = false;

    if (!proxy_execute)
        goto jscgtk_execute_cleanup;

    global = g_proxy_execute_jsc.jsc_context_new();
    if (!global) {
        LOG_ERROR("Failed to create global JS context\n");
        goto jscgtk_execute_cleanup;
    }

    // Register native functions with JavaScript engine
    JSCValue *global_object = g_proxy_execute_jsc.jsc_context_get_global_object(global);
    JSCValue *function = g_proxy_execute_jsc.jsc_value_new_function(
        global, "dnsResolve", G_CALLBACK(proxy_execute_jsc_dns_resolve), NULL, NULL, G_TYPE_STRING, 1, G_TYPE_STRING);
    if (!function) {
        LOG_ERROR("Unable to hook native function for %s\n", "dnsResolve");
        goto jscgtk_execute_cleanup;
    }
    g_proxy_execute_jsc.jsc_value_object_set_property(global_object, "dnsResolve", function);
    function =
        g_proxy_execute_jsc.jsc_value_new_function(global, "dnsResolveEx", G_CALLBACK(proxy_execute_jsc_dns_resolve_ex),
                                                   NULL, NULL, G_TYPE_STRING, 1, G_TYPE_STRING);
    if (!function) {
        LOG_ERROR("Unable to hook native function for %s\n", "dnsResolveEx");
        goto jscgtk_execute_cleanup;
    }
    g_proxy_execute_jsc.jsc_value_object_set_property(global_object, "dnsResolveEx", function);
    function = g_proxy_execute_jsc.jsc_value_new_function(
        global, "myIpAddress", G_CALLBACK(proxy_execute_jsc_my_ip_address), NULL, NULL, G_TYPE_STRING, 0);
    if (!function) {
        LOG_ERROR("Unable to hook native function for %s\n", "myIpAddress");
        goto jscgtk_execute_cleanup;
    }
    g_proxy_execute_jsc.jsc_value_object_set_property(global_object, "myIpAddress", function);
    function = g_proxy_execute_jsc.jsc_value_new_function(
        global, "myIpAddressEx", G_CALLBACK(proxy_execute_jsc_my_ip_address_ex), NULL, NULL, G_TYPE_STRING, 0);
    if (!function) {
        LOG_ERROR("Unable to hook native function for %s\n", "myIpAddressEx");
        goto jscgtk_execute_cleanup;
    }
    g_proxy_execute_jsc.jsc_value_object_set_property(global_object, "myIpAddressEx", function);

    // Load Mozilla's JavaScript PAC utilities to help process PAC files
    g_proxy_execute_jsc.jsc_context_evaluate(global, MOZILLA_PAC_JAVASCRIPT, -1);
    exception = g_proxy_execute_jsc.jsc_context_get_exception(global);
    if (exception) {
        LOG_ERROR("Unable to execute Mozilla's JavaScript PAC utilities\n");
        js_print_exception(global, exception);
        goto jscgtk_execute_cleanup;
    }

    // Load PAC script
    g_proxy_execute_jsc.jsc_context_evaluate(global, script, -1);
    exception = g_proxy_execute_jsc.jsc_context_get_exception(global);
    if (exception) {
        LOG_ERROR("Unable to execute PAC script\n");
        js_print_exception(global, exception);
        goto jscgtk_execute_cleanup;
    }

    // Construct the call FindProxyForURL
    char *host = get_url_host(url);
    snprintf(find_proxy, sizeof(find_proxy), "FindProxyForURL(\"%s\", \"%s\");", url, host ? host : url);
    free(host);

    // Execute the call to FindProxyForURL
    JSCValue *proxy_value = g_proxy_execute_jsc.jsc_context_evaluate(global, find_proxy, -1);
    exception = g_proxy_execute_jsc.jsc_context_get_exception(global);
    if (exception) {
        LOG_ERROR("Unable to execute FindProxyForURL\n");
        js_print_exception(global, exception);
        goto jscgtk_execute_cleanup;
    }

    if (!g_proxy_execute_jsc.jsc_value_is_string(proxy_value)) {
        LOG_ERROR("Incorrect return type from FindProxyForURL\n");
        goto jscgtk_execute_cleanup;
    }

    // Get the result of the call to FindProxyForURL
    if (proxy_value) {
        proxy_execute->list = g_proxy_execute_jsc.jsc_value_to_string(proxy_value);
        is_ok = true;
    }

jscgtk_execute_cleanup:

    if (global)
        g_object_unref(global);

    return is_ok;
}

const char *proxy_execute_jsc_get_list(void *ctx) {
    proxy_execute_jsc_s *proxy_execute = (proxy_execute_jsc_s *)ctx;
    return proxy_execute->list;
}

int32_t proxy_execute_jsc_get_error(void *ctx) {
    proxy_execute_jsc_s *proxy_execute = (proxy_execute_jsc_s *)ctx;
    return proxy_execute->error;
}

void proxy_execute_jsc_delayed_init(void) {
    if (g_proxy_execute_jsc.module)
        return;

    const char *library_names[] = {"libjavascriptcoregtk-6.0.so.1", "libjavascriptcoregtk-4.1.so.0",
                                   "libjavascriptcoregtk-4.0.so.18"};
    const size_t library_names_size = sizeof(library_names) / sizeof(library_names[0]);

    // Use existing JavaScriptCoreGTK if already loaded
    struct link_map *map = NULL;
    void *current_process = dlopen(0, RTLD_LAZY);
    if (!current_process)
        return;
    if (dlinfo(current_process, RTLD_DI_LINKMAP, &map) == 0) {
        while (map && !g_proxy_execute_jsc.module) {
            for (size_t i = 0; i < library_names_size; i++) {
                if (strstr(map->l_name, library_names[i])) {
                    g_proxy_execute_jsc.module = dlopen(map->l_name, RTLD_NOLOAD | RTLD_LAZY | RTLD_LOCAL);
                    break;
                }
            }
            map = map->l_next;
        }
    }
    dlclose(current_process);

    // Load the first available version of the JavaScriptCoreGTK
    for (size_t i = 0; !g_proxy_execute_jsc.module && i < library_names_size; i++) {
        g_proxy_execute_jsc.module = dlopen(library_names[i], RTLD_LAZY | RTLD_LOCAL);
    }

    if (!g_proxy_execute_jsc.module)
        return;

    // Context functions
    g_proxy_execute_jsc.jsc_context_new = dlsym(g_proxy_execute_jsc.module, "jsc_context_new");
    if (!g_proxy_execute_jsc.jsc_context_new)
        goto jsc_init_error;
    g_proxy_execute_jsc.jsc_context_get_global_object =
        dlsym(g_proxy_execute_jsc.module, "jsc_context_get_global_object");
    if (!g_proxy_execute_jsc.jsc_context_get_global_object)
        goto jsc_init_error;
    g_proxy_execute_jsc.jsc_context_evaluate = dlsym(g_proxy_execute_jsc.module, "jsc_context_evaluate");
    if (!g_proxy_execute_jsc.jsc_context_evaluate)
        goto jsc_init_error;
    g_proxy_execute_jsc.jsc_context_get_exception = dlsym(g_proxy_execute_jsc.module, "jsc_context_get_exception");
    if (!g_proxy_execute_jsc.jsc_context_get_exception)
        goto jsc_init_error;
    // Value functions
    g_proxy_execute_jsc.jsc_value_is_string = dlsym(g_proxy_execute_jsc.module, "jsc_value_is_string");
    if (!g_proxy_execute_jsc.jsc_value_is_string)
        goto jsc_init_error;
    g_proxy_execute_jsc.jsc_value_is_number = dlsym(g_proxy_execute_jsc.module, "jsc_value_is_number");
    if (!g_proxy_execute_jsc.jsc_value_is_number)
        goto jsc_init_error;
    g_proxy_execute_jsc.jsc_value_is_object = dlsym(g_proxy_execute_jsc.module, "jsc_value_is_object");
    if (!g_proxy_execute_jsc.jsc_value_is_object)
        goto jsc_init_error;
    g_proxy_execute_jsc.jsc_value_to_double = dlsym(g_proxy_execute_jsc.module, "jsc_value_to_double");
    if (!g_proxy_execute_jsc.jsc_value_to_double)
        goto jsc_init_error;
    g_proxy_execute_jsc.jsc_value_new_string = dlsym(g_proxy_execute_jsc.module, "jsc_value_new_string");
    if (!g_proxy_execute_jsc.jsc_value_new_string)
        goto jsc_init_error;
    g_proxy_execute_jsc.jsc_value_to_string = dlsym(g_proxy_execute_jsc.module, "jsc_value_to_string");
    if (!g_proxy_execute_jsc.jsc_value_to_string)
        goto jsc_init_error;
    g_proxy_execute_jsc.jsc_value_new_function = dlsym(g_proxy_execute_jsc.module, "jsc_value_new_function");
    if (!g_proxy_execute_jsc.jsc_value_new_function)
        goto jsc_init_error;
    g_proxy_execute_jsc.jsc_value_object_get_property =
        dlsym(g_proxy_execute_jsc.module, "jsc_value_object_get_property");
    if (!g_proxy_execute_jsc.jsc_value_object_get_property)
        goto jsc_init_error;
    g_proxy_execute_jsc.jsc_value_object_set_property =
        dlsym(g_proxy_execute_jsc.module, "jsc_value_object_set_property");
    if (!g_proxy_execute_jsc.jsc_value_object_set_property)
        goto jsc_init_error;
    // Exception functions
    g_proxy_execute_jsc.jsc_exception_report = dlsym(g_proxy_execute_jsc.module, "jsc_exception_report");
    if (!g_proxy_execute_jsc.jsc_exception_report)
        goto jsc_init_error;

    return;

jsc_init_error:
    proxy_execute_jsc_global_cleanup();
}

void *proxy_execute_jsc_create(void) {
    pthread_once(&g_proxy_execute_jsc_init_flag, proxy_execute_jsc_delayed_init);
    if (!g_proxy_execute_jsc.module)
        return NULL;
    proxy_execute_jsc_s *proxy_execute = (proxy_execute_jsc_s *)calloc(1, sizeof(proxy_execute_jsc_s));
    return proxy_execute;
}

bool proxy_execute_jsc_delete(void **ctx) {
    if (!ctx)
        return false;
    proxy_execute_jsc_s *proxy_execute = (proxy_execute_jsc_s *)*ctx;
    if (!proxy_execute)
        return false;
    free(proxy_execute->list);
    free(proxy_execute);
    *ctx = NULL;
    return true;
}

/*********************************************************************/

bool proxy_execute_jsc_global_init(void) {
    // JSCoreGTK will be initialized with a delay to avoid conflicts with the
    // loaded JSCoreGTK in a user application after this function.
    return true;
}

bool proxy_execute_jsc_global_cleanup(void) {
    if (g_proxy_execute_jsc.module)
        dlclose(g_proxy_execute_jsc.module);

    memset(&g_proxy_execute_jsc, 0, sizeof(g_proxy_execute_jsc));
    g_proxy_execute_jsc_init_flag = PTHREAD_ONCE_INIT;
    return true;
}

proxy_execute_i_s *proxy_execute_jsc_get_interface(void) {
    static proxy_execute_i_s proxy_execute_jsc_i = {proxy_execute_jsc_get_proxies_for_url,
                                                    proxy_execute_jsc_get_list,
                                                    proxy_execute_jsc_get_error,
                                                    proxy_execute_jsc_create,
                                                    proxy_execute_jsc_delete,
                                                    proxy_execute_jsc_global_init,
                                                    proxy_execute_jsc_global_cleanup};
    return &proxy_execute_jsc_i;
}
