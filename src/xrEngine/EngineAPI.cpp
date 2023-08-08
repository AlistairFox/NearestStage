// EngineAPI.cpp: implementation of the CEngineAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EngineAPI.h"
#include "../xrcdb/xrXRC.h"

#include "securom_api.h"

extern xr_token* vid_quality_token;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void __cdecl dummy(void) {
};
CEngineAPI::CEngineAPI()
{
	hGame = 0;
	hRender = 0;
	hTuner = 0;
	pCreate = 0;
	pDestroy = 0;
	tune_pause = dummy;
	tune_resume = dummy;
}

CEngineAPI::~CEngineAPI()
{
	// destroy quality token here
	if (vid_quality_token)
	{
		for (int i = 0; vid_quality_token[i].name; i++)
		{
			xr_free(vid_quality_token[i].name);
		}
		xr_free(vid_quality_token);
		vid_quality_token = NULL;
	}
}

extern u32 renderer_value; //con cmd
ENGINE_API int g_current_renderer = 0;

ENGINE_API bool is_enough_address_space_available()
{
	SYSTEM_INFO		system_info;

	SECUROM_MARKER_HIGH_SECURITY_ON(12)

		GetSystemInfo(&system_info);

	SECUROM_MARKER_HIGH_SECURITY_OFF(12)

		return			(*(u32*)&system_info.lpMaximumApplicationAddress) > 0x90000000;
}

LPCSTR r1_name = "xrRender_R1.dll";
LPCSTR r2_name = "xrRender_R2.dll";
LPCSTR r4_name = "xrRender_R4.dll";

#ifndef DEDICATED_SERVER
void CEngineAPI::InitializeNotDedicated()
{
	SECUROM_MARKER_HIGH_SECURITY_ON(2)

		// initialize R4
		if (psDeviceFlags.test(rsR4))
		{
			Log("Loading DLL: ", r4_name);
			hRender = LoadLibrary(r4_name);
			if (0 == hRender)
			{
				// try to load R1
				Msg("! ...Failed - incompatible hardware/pre-Vista OS.");
				renderer_value = 3;
				psDeviceFlags.set(rsR2, TRUE);
				psDeviceFlags.set(rsR4, FALSE);
			}
		}

	if (0 == hRender)
	{
		// try to initialize R2
		psDeviceFlags.set(rsR2, TRUE);
		psDeviceFlags.set(rsR4, FALSE);
		//psDeviceFlags.set	(rsR3,FALSE);
		Log("Loading DLL: ", r2_name);
		hRender = LoadLibrary(r2_name);
		if (0 == hRender)
		{
			Log("! ...Failed - incompatible hardware. Need to support DX9C!");
			R_CHK(GetLastError());
			R_ASSERT(hRender, "! ...Failed - incompatible hardware. Need to support DX9C!");
		}
	}

	g_current_renderer = 2;

	if (renderer_value == 1)
		Msg("Current Renderer: %s", vid_quality_token[0].name);
	else if (renderer_value == 3)
		Msg("Current Renderer: %s", vid_quality_token[1].name);
	else if (renderer_value == 5)
		Msg("Current Renderer: %s", vid_quality_token[2].name);
	else
		Msg("Ты че долбаеб? ты какой рендер запустил?");


	Log("Render_value: ", renderer_value);

	SECUROM_MARKER_HIGH_SECURITY_OFF(2)
}
#endif // DEDICATED_SERVER

void CEngineAPI::Initialize(void)
{
	//////////////////////////////////////////////////////////////////////////
	// render

#ifndef DEDICATED_SERVER
	InitializeNotDedicated();
#else
	if (0 == hRender)
	{
		Msg("Ты че долбаеб?? выруби сервер!");
		// try to load R1
		psDeviceFlags.set(rsR4, FALSE);
		//psDeviceFlags.set	(rsR3,FALSE);
		psDeviceFlags.set(rsR2, FALSE);

		Log("Loading DLL:", r1_name);
		hRender = LoadLibrary(r1_name);

		if (0 == hRender)
			R_CHK(GetLastError());
		R_ASSERT(hRender);

		g_current_renderer = 1;
	}
#endif // DEDICATED_SERVER

	Device.ConnectToRender();

	// game	
	{
		LPCSTR			g_name = "xrGame.dll";
		Log("Loading DLL: ", g_name);
		hGame = LoadLibrary(g_name);
		if (0 == hGame)	R_CHK(GetLastError());
		R_ASSERT2(hGame, "Game DLL raised exception during loading or there is no game DLL at all");
		pCreate = (Factory_Create*)GetProcAddress(hGame, "xrFactory_Create");	R_ASSERT(pCreate);
		pDestroy = (Factory_Destroy*)GetProcAddress(hGame, "xrFactory_Destroy");	R_ASSERT(pDestroy);
	}

	//////////////////////////////////////////////////////////////////////////
	// vTune
	tune_enabled = FALSE;
	if (strstr(Core.Params, "-tune")) {
		LPCSTR			g_name = "vTuneAPI.dll";
		Log("Loading DLL:", g_name);
		hTuner = LoadLibrary(g_name);
		if (0 == hTuner)	R_CHK(GetLastError());
		R_ASSERT2(hTuner, "Intel vTune is not installed");
		tune_enabled = TRUE;
		tune_pause = (VTPause*)GetProcAddress(hTuner, "VTPause");	R_ASSERT(tune_pause);
		tune_resume = (VTResume*)GetProcAddress(hTuner, "VTResume");	R_ASSERT(tune_resume);
	}
}

void CEngineAPI::Destroy(void)
{
	if (hGame) { FreeLibrary(hGame);	hGame = 0; }
	if (hRender) { FreeLibrary(hRender); hRender = 0; }
	pCreate = 0;
	pDestroy = 0;
	Engine.Event._destroy();
	XRC.r_clear_compact();
}

extern "C" {
	typedef bool __cdecl SupportsAdvancedRendering(void);
	//typedef bool _declspec(dllexport) SupportsDX10Rendering();
	typedef bool _declspec(dllexport) SupportsDX11Rendering();
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
	// проверяем поддержку R4
	bool bSupports_r4 = false;
	Log("Try loading DLL: ", r4_name);
	SetErrorMode(SEM_FAILCRITICALERRORS);
	hRender = LoadLibrary(r4_name);
	SetErrorMode(0);
	if (hRender)
	{
		SupportsDX11Rendering* test_dx11_rendering = (SupportsDX11Rendering*)GetProcAddress(hRender, "SupportsDX11Rendering");
		R_ASSERT(test_dx11_rendering);
		bSupports_r4 = test_dx11_rendering();
		FreeLibrary(hRender);
	}
	Log("* supports: ", bSupports_r4);

	hRender = 0;

	// создаем список 
	u32 size = 3; // r2a, r2.5

	if (bSupports_r4) // r4
		size += 1;

	vid_quality_token = xr_alloc<xr_token>(size);

	vid_quality_token[0].id = 1;
	vid_quality_token[0].name = xr_strdup("renderer_r2a");

	vid_quality_token[1].id = 3;
	vid_quality_token[1].name = xr_strdup("renderer_r2.5");

	if (bSupports_r4)
	{
		vid_quality_token[2].id = 5;
		vid_quality_token[2].name = xr_strdup("renderer_r4");
	}

	vid_quality_token[size - 1].id = -1;
	vid_quality_token[size - 1].name = NULL;

	Msg("Available render modes[%d]:", size - 1);
	for (u32 i = 0; i < size -1; ++i)
		Msg("[%s]", vid_quality_token[i].name);
#endif
}