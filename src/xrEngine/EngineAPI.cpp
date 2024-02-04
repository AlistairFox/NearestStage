// EngineAPI.cpp: implementation of the CEngineAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EngineAPI.h"
#include "../xrcdb/xrXRC.h"

#include "securom_api.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "vfw32.lib")
#pragma comment(lib, "nvapi.lib")
#pragma comment(lib, "xrgame.lib")
#pragma comment(lib, "xrphysics.lib")
#pragma comment(lib, "ode.lib")
#pragma comment(lib, "GameNetworkingSockets.lib")
#ifdef DEDICATED_SERVER

#pragma comment(lib, "xrServerRender.lib")

#else
#pragma comment(lib, "xrRender_R4.lib")
#pragma comment(lib, "GFSDK_SSAO_D3D11.win64.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d10.lib")

#endif // DEDICATED_SERVER



extern xr_token* vid_quality_token;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void __cdecl dummy		(void)	{
};
CEngineAPI::CEngineAPI	()
{
	hGame			= 0;
	hRender			= 0;
	hTuner			= 0;
	pCreate			= 0;
	pDestroy		= 0;
	tune_pause		= dummy	;
	tune_resume		= dummy	;
}

CEngineAPI::~CEngineAPI()
{
	// destroy quality token here
	xr_free(vid_quality_token);
}

ENGINE_API bool is_enough_address_space_available	()
{
	SYSTEM_INFO		system_info;

	SECUROM_MARKER_HIGH_SECURITY_ON(12)

	GetSystemInfo	( &system_info );

	SECUROM_MARKER_HIGH_SECURITY_OFF(12)

	return			(*(u32*)&system_info.lpMaximumApplicationAddress) > 0x90000000;	
}


#ifndef DEDICATED_SERVER
extern BOOL DllMainXrRenderR4(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);
#define DLL_MAIN_RENDER DllMainXrRenderR4
#else
extern BOOL DllMainXrServerRender(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);
#define DLL_MAIN_RENDER DllMainXrServerRender
#endif // !DEDICATED_SERVER


LPCSTR r1_name = "xrServerRender";
LPCSTR r4_name = "xrRender_R4";

#ifndef DEDICATED_SERVER
void CEngineAPI::InitializeNotDedicated()
{
	//if (psDeviceFlags.test(rsR4))
	{
		// try to initialize R4
		psDeviceFlags.set(rsR2, FALSE);
		Log("Loading DLL:", r4_name);
		DllMainXrRenderR4(NULL, DLL_PROCESS_ATTACH, NULL);
		//hRender = LoadLibrary(r4_name);
	//if (0 == hRender)
	//{
	//    // try to load R1
	//    Msg("! ...Failed - incompatible hardware/pre-Vista OS.");
	//    psDeviceFlags.set(rsR2, TRUE);
		//}
	}
}
#endif // DEDICATED_SERVER

extern bool DllMainXrGame(HANDLE hModule, u32 ul_reason_for_call, LPVOID lpReserved);
extern "C"
DLL_Pure *  xrFactory_Create(CLASS_ID clsid);
extern "C"
void  xrFactory_Destroy(DLL_Pure * O);
void CEngineAPI::Initialize(void)
{
	//////////////////////////////////////////////////////////////////////////
	// render

	#ifdef DEDICATED_SERVER
		// try to load R1
		psDeviceFlags.set(rsR4, FALSE);
		//psDeviceFlags.set	(rsR3,FALSE);
		psDeviceFlags.set(rsR2, FALSE);

		Log("Loading DLL:", r1_name);
		DllMainXrServerRender(NULL, DLL_PROCESS_ATTACH, NULL);
	#else
		InitializeNotDedicated();
	#endif // DEDICATED_SERVER

	Device.ConnectToRender();

	// game
	{
		LPCSTR g_name = "xrGame.dll";
		Log("Loading DLL:", g_name);
		//hGame = LoadLibrary(g_name);
		DllMainXrGame(NULL, DLL_PROCESS_ATTACH, NULL);
		//if (0 == hGame) R_CHK(GetLastError());
		//R_ASSERT2(hGame, "Game DLL raised exception during loading or there is no game DLL at all");
		//pCreate = (Factory_Create*)GetProcAddress(hGame, "xrFactory_Create");
		pCreate = xrFactory_Create;
		R_ASSERT(pCreate);
		//pDestroy = (Factory_Destroy*)GetProcAddress(hGame, "xrFactory_Destroy");
		pDestroy = xrFactory_Destroy;
		R_ASSERT(pDestroy);
	}

	//////////////////////////////////////////////////////////////////////////
	// vTune
	tune_enabled		= FALSE;
	if (strstr(Core.Params,"-tune"))	{
		LPCSTR			g_name	= "vTuneAPI.dll";
		Log				("Loading DLL:",g_name);
		hTuner			= LoadLibrary	(g_name);
		if (0==hTuner)	R_CHK			(GetLastError());
		R_ASSERT2		(hTuner,"Intel vTune is not installed");
		tune_enabled	= TRUE;
		tune_pause		= (VTPause*)	GetProcAddress(hTuner,"VTPause"		);	R_ASSERT(tune_pause);
		tune_resume		= (VTResume*)	GetProcAddress(hTuner,"VTResume"	);	R_ASSERT(tune_resume);
	}
}

void CEngineAPI::Destroy	(void)
{
	//if (hGame) { FreeLibrary(hGame); hGame = 0; }
	DllMainXrGame(NULL, DLL_PROCESS_DETACH, NULL);
	//if (hRender) { FreeLibrary(hRender); hRender = 0; }
	DLL_MAIN_RENDER(NULL, DLL_PROCESS_DETACH, NULL);
	pCreate = 0;
	pDestroy = 0;
	Engine.Event._destroy();
	XRC.r_clear_compact();
}

extern "C" {
	typedef bool __cdecl SupportsAdvancedRenderingREF(void);
	typedef bool /*_declspec(dllexport)*/ SupportsDX10RenderingREF();
	typedef bool /*_declspec(dllexport)*/ SupportsDX11RenderingREF();
};

extern "C" {

	bool /*_declspec(dllexport)*/ SupportsDX11Rendering();

};

void CEngineAPI::CreateRendererList()
{
#ifdef DEDICATED_SERVER
	vid_quality_token = xr_alloc<xr_token>(2);

	vid_quality_token[0].id = 0;
	vid_quality_token[0].name = xr_strdup("renderer_r1");

	vid_quality_token[1].id = -1;
	vid_quality_token[1].name = NULL;
#else
	
	// TODO: ask renderers if they are supported!
	if (vid_quality_token != NULL) return;
	bool bSupports_r2 = false;
	bool bSupports_r2_5 = false;
	bool bSupports_r3 = false;
	bool bSupports_r4 = false;

	LPCSTR r2_name = "xrRender_R2.dll";
	LPCSTR r3_name = "xrRender_R3.dll";
	LPCSTR r4_name = "xrRender_R4.dll";

	if (strstr(Core.Params, "-perfhud_hack"))
	{
		bSupports_r2 = true;
		bSupports_r2_5 = true;
		bSupports_r3 = true;
		bSupports_r4 = true;
	}
	else
	{



		// try to initialize R4
		Log("Loading DLL:", r4_name);
		// Hide "d3d10.dll not found" message box for XP
		SetErrorMode(SEM_FAILCRITICALERRORS);
		//hRender = LoadLibrary(r4_name);
		DllMainXrRenderR4(NULL, DLL_PROCESS_ATTACH, NULL);
		// Restore error handling
		SetErrorMode(0);
		//if (hRender)
		{
			//SupportsDX11RenderingREF* test_dx11_rendering = (SupportsDX11RenderingREF*)GetProcAddress(hRender, "SupportsDX11Rendering");
			SupportsDX11RenderingREF* test_dx11_rendering = SupportsDX11Rendering;
			R_ASSERT(test_dx11_rendering);
			bSupports_r4 = test_dx11_rendering();
			//FreeLibrary(hRender);
		}
	}

	//hRender = 0;
	bool proceed = true;
	xr_vector<LPCSTR> _tmp;
	if (proceed &= bSupports_r4, proceed)
		_tmp.push_back("renderer_r4");

	R_ASSERT2(_tmp.size() != 0, "No valid renderer found, please use a render system that's supported by your PC");

	u32 _cnt = _tmp.size() + 1;
	vid_quality_token = xr_alloc<xr_token>(_cnt);

	vid_quality_token[_cnt - 1].id = -1;
	vid_quality_token[_cnt - 1].name = NULL;

	//#ifdef DEBUG
	Msg("Available render modes[%d]:", _tmp.size());
	//#endif // DEBUG
	for (u32 i = 0; i < _tmp.size(); ++i)
	{
		vid_quality_token[i].id = i;
		vid_quality_token[i].name = _tmp[i];
		//#ifdef DEBUG
		Msg("[%s]", _tmp[i]);
		//#endif // DEBUG
	}
#endif //#ifndef DEDICATED_SERVER
}