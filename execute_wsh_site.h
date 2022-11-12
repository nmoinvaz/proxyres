#include <windows.h>
#include <activscp.h>

#include "execute_wsh_dispatch.h"

#define WSH_SCRIPT_NAME OLESTR("proxy_execute_wsh")

typedef struct active_script_site_s {
    IActiveScriptSite site;
    ULONG ref_count;
    script_dispatch_s script_dispatch;
} active_script_site_s;

#define cast_from_site_interface(this) (active_script_site_s *)((uint8_t *)this - offsetof(active_script_site_s, site))

static ULONG STDMETHODCALLTYPE active_script_site_add_ref(IActiveScriptSite *site) {
    active_script_site_s *this = cast_from_site_interface(site);
    return ++this->ref_count;
}

static ULONG STDMETHODCALLTYPE active_script_site_release(IActiveScriptSite *site) {
    active_script_site_s *this = cast_from_site_interface(site);
    if (--this->ref_count == 0)
        return 0;
    return this->ref_count;
}

static HRESULT STDMETHODCALLTYPE active_script_site_query_interface(IActiveScriptSite *site, REFIID riid, void **ppv) {
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IActiveScriptSite)) {
        *ppv = site;
        active_script_site_add_ref(site);
        return NOERROR;
    }
    *ppv = 0;
    return E_NOINTERFACE;
}

static HRESULT STDMETHODCALLTYPE active_script_site_get_lcid(IActiveScriptSite *site, LCID *plcid) {
    return plcid ? E_POINTER : E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE active_script_site_get_item_info(IActiveScriptSite *site, LPCOLESTR name,
                                                                  DWORD return_mask, IUnknown **unknown_item,
                                                                  ITypeInfo **type_info) {
    if (return_mask & SCRIPTINFO_IUNKNOWN) {
        if (!unknown_item)
            return E_INVALIDARG;

        if (wcscmp(name, WSH_SCRIPT_NAME) == 0) {
            active_script_site_s *this = cast_from_site_interface(site);

            // Return our IDispatch interface to handle native PAC functions
            this->script_dispatch.ref_count = 0;
            this->script_dispatch.dispatch.lpVtbl = script_dispatch_get_vtbl();

            script_dispatch_add_ref(&this->script_dispatch.dispatch);

            *unknown_item = (IUnknown *)&this->script_dispatch.dispatch;
            return S_OK;
        }

        *unknown_item = NULL;
    }
    if (return_mask & SCRIPTINFO_ITYPEINFO) {
        if (!type_info)
            return E_INVALIDARG;
        *type_info = NULL;
    }
    return TYPE_E_ELEMENTNOTFOUND;
}

static HRESULT STDMETHODCALLTYPE active_script_site_get_doc_version_string(IActiveScriptSite *site,
                                                                           BSTR *version_string) {
    return !version_string ? E_POINTER : E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE active_script_site_on_script_terminate(IActiveScriptSite *site,
                                                                        const VARIANT *result_ptr,
                                                                        const EXCEPINFO *exception_info) {
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE active_script_site_on_state_change(IActiveScriptSite *site, SCRIPTSTATE script_state) {
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE active_script_site_on_script_error(IActiveScriptSite *site,
                                                                    IActiveScriptError *error) {
    EXCEPINFO exception = {0};

    // Print exception information to log
    HRESULT result = IActiveScriptError_GetExceptionInfo(error, &exception);
    if (FAILED(result)) {
        LOG_ERROR("Failed to get active script error (0x%08x)\n", result);
        return S_OK;
    }

    printf("EXCEPTION: ");
    char *exception_description = wchar_dup_to_utf8(exception.bstrDescription);
    if (exception_description) {
        printf("%s\n", exception_description);
        free(exception_description);
    }

    BSTR source_line_text;
    result = IActiveScriptError_GetSourceLineText(error, &source_line_text);
    if (FAILED(result)) {
        LOG_ERROR("Failed to get exception source line text (0x%08x)\n", result);
        return S_OK;
    }

    char *exception_source_line = wchar_dup_to_utf8(source_line_text);
    if (exception_source_line) {
        printf("  %s\n", exception_source_line);
        free(exception_source_line);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE active_script_site_on_enter_script(IActiveScriptSite *site) {
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE active_script_site_on_leave_script(IActiveScriptSite *site) {
    return S_OK;
}

static const IActiveScriptSiteVtbl active_script_site_vtbl = {active_script_site_query_interface,
                                                              active_script_site_add_ref,
                                                              active_script_site_release,
                                                              active_script_site_get_lcid,
                                                              active_script_site_get_item_info,
                                                              active_script_site_get_doc_version_string,
                                                              active_script_site_on_script_terminate,
                                                              active_script_site_on_state_change,
                                                              active_script_site_on_script_error,
                                                              active_script_site_on_enter_script,
                                                              active_script_site_on_leave_script};

IActiveScriptSiteVtbl *active_script_site_get_vtbl() {
    return (IActiveScriptSiteVtbl *)&active_script_site_vtbl;
}
