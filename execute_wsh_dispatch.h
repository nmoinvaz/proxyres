#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <windows.h>

#include "net_adapter.h"
#include "util_win.h"

typedef struct script_dispatch_s {
    IDispatch dispatch;
    ULONG ref_count;
} script_dispatch_s;

#define SCRIPT_DISPATCH_DNS_RESOLVE_ID   1
#define SCRIPT_DISPATCH_MY_IP_ADDRESS_ID 2

#define cast_from_dispatch_interface(this) \
  (script_dispatch_s *)((uint8_t *)this - offsetof(script_dispatch_s, dispatch))

static ULONG STDMETHODCALLTYPE script_dispatch_add_ref(IDispatch *dispatch) {
    script_dispatch_s *this = cast_from_dispatch_interface(dispatch);
    return ++this->ref_count;
}

static ULONG STDMETHODCALLTYPE script_dispatch_release(IDispatch *dispatch) {
    script_dispatch_s *this = cast_from_dispatch_interface(dispatch);
    if (--this->ref_count == 0)
        return 0;
    return this->ref_count;
}

static HRESULT STDMETHODCALLTYPE script_dispatch_query_interface(IDispatch *dispatch, REFIID riid, void **ppv) {
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDispatch)) {
        script_dispatch_add_ref(dispatch);
        *ppv = dispatch;
        return NOERROR;
    }
    *ppv = 0;
    return E_NOINTERFACE;
}

static HRESULT STDMETHODCALLTYPE script_dispatch_get_type_info_count(IDispatch *dispatch, unsigned int *pctinfo) {
    if (!pctinfo)
        return E_INVALIDARG;
    *pctinfo = 0;
    return NOERROR;
}

static HRESULT STDMETHODCALLTYPE script_dispatch_get_type_info(IDispatch *dispatch, unsigned int type, LCID lcid,
                                                               ITypeInfo **type_info) {
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE script_dispatch_get_ids_of_names(IDispatch *dispatch, REFIID riid, OLECHAR **names,
                                                                  unsigned names_count, LCID lcid,
                                                                  DISPID *ret_disp_id) {
    HRESULT result = S_OK;

    // Return the ID of the function
    for (unsigned i = 0; i < names_count; i++) {
        if (wcscmp(names[i], OLESTR("dnsResolve")) == 0) {
            ret_disp_id[i] = SCRIPT_DISPATCH_DNS_RESOLVE_ID;
        } else if (wcscmp(names[i], OLESTR("myIpAddress")) == 0) {
            ret_disp_id[i] = SCRIPT_DISPATCH_MY_IP_ADDRESS_ID;
        } else {
            ret_disp_id[i] = DISPID_UNKNOWN;
            result = DISP_E_UNKNOWNNAME;
        }
    }

    return result;
}

static HRESULT STDMETHODCALLTYPE script_dispatch_invoke(IDispatch *dispatch, DISPID disp_id, REFIID riid, LCID lcid,
                                                        WORD flags, DISPPARAMS *disp_params, VARIANT *result_ptr,
                                                        EXCEPINFO *excep_info, UINT *arg_err) {
    // Check ID of the function and return the result
    if (disp_id == SCRIPT_DISPATCH_DNS_RESOLVE_ID) {
        if (disp_params->cArgs != 1)
            return DISP_E_BADPARAMCOUNT;
        if (disp_params->cNamedArgs != 0)
            return DISP_E_NONAMEDARGS;
        if (disp_params->rgvarg[0].vt != VT_BSTR) {
            arg_err = 0;
            return DISP_E_TYPEMISMATCH;
        }

        char *host_utf8 = wchar_dup_to_utf8(disp_params->rgvarg[0].bstrVal);
        if (!host_utf8)
            return E_OUTOFMEMORY;

        // Resolve hostname to IP string
        char *ip = dns_resolve(host_utf8, NULL);
        free(host_utf8);
        if (!ip)
            return E_FAIL;

        wchar_t *ip_wchar = utf8_dup_to_wchar(ip);
        free(ip);
        if (!ip_wchar)
            return E_OUTOFMEMORY;

        VariantInit(result_ptr);

        // Return IP string as BSTR
        result_ptr->vt = VT_BSTR;
        result_ptr->bstrVal = SysAllocString(ip_wchar);

        free(ip_wchar);
        return S_OK;
    } else if (disp_id == SCRIPT_DISPATCH_MY_IP_ADDRESS_ID) {
        if (disp_params->cArgs != 0)
            return DISP_E_BADPARAMCOUNT;
        if (disp_params->cNamedArgs != 0)
            return DISP_E_NONAMEDARGS;

        // Resolve local hostname to IP string
        char *ip = dns_resolve(NULL, NULL);
        if (!ip)
            return E_FAIL;

        wchar_t *ip_wchar = utf8_dup_to_wchar(ip);
        free(ip);
        if (!ip_wchar)
            return E_OUTOFMEMORY;

        VariantInit(result_ptr);

        // Return IP string as BSTR
        result_ptr->vt = VT_BSTR;
        result_ptr->bstrVal = SysAllocString(ip_wchar);

        free(ip_wchar);
        return S_OK;
    }

    return DISP_E_MEMBERNOTFOUND;
}

static const IDispatchVtbl script_dispatch_vtbl = {script_dispatch_query_interface, script_dispatch_add_ref,
                                                   script_dispatch_release,         script_dispatch_get_type_info_count,
                                                   script_dispatch_get_type_info,   script_dispatch_get_ids_of_names,
                                                   script_dispatch_invoke};

IDispatchVtbl *script_dispatch_get_vtbl() {
    return (IDispatchVtbl *)&script_dispatch_vtbl;
}
