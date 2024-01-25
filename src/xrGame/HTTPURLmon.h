#pragma once
#pragma comment(lib, "urlmon.lib")
#include <urlmon.h>


class CDownloadCallback : public IBindStatusCallback
{
    virtual HRESULT STDMETHODCALLTYPE OnStartBinding(DWORD dwReserved, __RPC__in_opt IBinding* pib);

    virtual HRESULT STDMETHODCALLTYPE GetPriority(__RPC__out LONG* pnPriority);

    virtual HRESULT STDMETHODCALLTYPE OnLowResource(DWORD reserved);


    virtual HRESULT STDMETHODCALLTYPE OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, __RPC__in_opt LPCWSTR szStatusText);

    virtual HRESULT STDMETHODCALLTYPE OnStopBinding(HRESULT hresult, __RPC__in_opt LPCWSTR szError);

    virtual HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD* grfBINDF, BINDINFO* pbindinfo);

    virtual HRESULT STDMETHODCALLTYPE OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed);

    virtual HRESULT STDMETHODCALLTYPE OnObjectAvailable(__RPC__in REFIID riid, __RPC__in_opt IUnknown* punk);


    HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) { return E_NOTIMPL; }
    ULONG __stdcall AddRef() { return 0; }
    ULONG __stdcall Release() { return 0; }

};
