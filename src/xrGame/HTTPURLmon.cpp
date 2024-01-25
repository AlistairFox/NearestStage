#include "StdAfx.h"
#include "HTTPURLmon.h"


HRESULT __stdcall CDownloadCallback::OnStartBinding(DWORD dwReserved, __RPC__in_opt IBinding* pib)
{
	return E_NOTIMPL;
}
HRESULT __stdcall CDownloadCallback::GetPriority(__RPC__out LONG* pnPriority)
{
	return E_NOTIMPL;
}

HRESULT __stdcall CDownloadCallback::OnLowResource(DWORD reserved)
{
	return E_NOTIMPL;
}

extern u32 download_progress = 0;
extern u32 download_progress_max = 0;
HRESULT __stdcall CDownloadCallback::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, __RPC__in_opt LPCWSTR szStatusText)
{
	download_progress = ulProgress;
	download_progress_max = ulProgressMax;
	//Msg("%u", ulProgress);
	return E_NOTIMPL;
}

HRESULT __stdcall CDownloadCallback::OnStopBinding(HRESULT hresult, __RPC__in_opt LPCWSTR szError)
{
	return E_NOTIMPL;
}

HRESULT __stdcall CDownloadCallback::GetBindInfo(DWORD* grfBINDF, BINDINFO* pbindinfo)
{
	return E_NOTIMPL;
}

HRESULT __stdcall CDownloadCallback::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
	return E_NOTIMPL;
}

HRESULT __stdcall CDownloadCallback::OnObjectAvailable(__RPC__in REFIID riid, __RPC__in_opt IUnknown* punk)
{
	return E_NOTIMPL;
}
