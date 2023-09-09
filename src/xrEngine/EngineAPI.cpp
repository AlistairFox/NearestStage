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

extern u32 renderer_value; //con cmd
ENGINE_API int g_current_renderer = 0; // R1 or R2, used in xrGame

ENGINE_API bool is_enough_address_space_available	()
{
	SYSTEM_INFO		system_info;

	SECUROM_MARKER_HIGH_SECURITY_ON(12)

	GetSystemInfo	( &system_info );

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

	// try to initialize R4
	if (psDeviceFlags.test(rsR4))
	{
		Log				("Loading DLL:",	r4_name);
		hRender			= LoadLibrary		(r4_name);
		g_current_renderer = 2;

		// R4 not supported
		if (NULL == hRender)
		{
			psDeviceFlags.set(rsR4, FALSE);
			psDeviceFlags.set(rsR2, TRUE);
			renderer_value = 3;
		}
	}

	// try to initialize R2
	if (psDeviceFlags.test(rsR2) || NULL == hRender)
	{
		Log("Loading DLL:", r2_name);
		hRender = LoadLibrary(r2_name);
		g_current_renderer = 2;

		// R2 not supported
		if (NULL == hRender)
		{
			psDeviceFlags.set(rsR2, FALSE);
			renderer_value = 0;
		}
	}

	// try to initialize R1
	if (NULL == hRender)
	{
		Log("Loading DLL:", r1_name);
		hRender = LoadLibrary(r1_name);
		g_current_renderer = 1;
		renderer_value = 0;
	}

	SECUROM_MARKER_HIGH_SECURITY_OFF(2)
}
#endif // DEDICATED_SERVER

void CEngineAPI::Initialize(void)
{
	//////////////////////////////////////////////////////////////////////////
	// render

	#ifdef DEDICATED_SERVER
		// try to load R1
		psDeviceFlags.set(rsR4, FALSE);
		//psDeviceFlags.set	(rsR3,FALSE);
		psDeviceFlags.set(rsR2, FALSE);
		renderer_value = 0; //con cmd

		Log("Loading DLL:", r1_name);
		hRender = LoadLibrary(r1_name);
		if (0 == hRender)	R_CHK(GetLastError());
		R_ASSERT(hRender);
		g_current_renderer = 1;
	#else
		InitializeNotDedicated();
		// try to initialize upper renderers
		if (NULL == hRender)
		{
			renderer_value = 5; //con cmd
			psDeviceFlags.set(rsR4, TRUE);
			psDeviceFlags.set(rsR2, FALSE);
			InitializeNotDedicated();
		}
		// all renderers not supported
		if (NULL == hRender)
		{
			R_CHK(GetLastError());
			R_ASSERT(hRender);
		}
	#endif // DEDICATED_SERVER

	Device.ConnectToRender();

	// game	
	{
		LPCSTR			g_name	= "xrGame.dll";
		Log				("Loading DLL:",g_name);
		hGame			= LoadLibrary	(g_name);
		if (0==hGame)	R_CHK			(GetLastError());
		R_ASSERT2		(hGame,"Game DLL raised exception during loading or there is no game DLL at all");
		pCreate			= (Factory_Create*)		GetProcAddress(hGame,"xrFactory_Create"		);	R_ASSERT(pCreate);
		pDestroy		= (Factory_Destroy*)	GetProcAddress(hGame,"xrFactory_Destroy"	);	R_ASSERT(pDestroy);
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
	if (hGame)				{ FreeLibrary(hGame);	hGame	= 0; }
	if (hRender)			{ FreeLibrary(hRender); hRender = 0; }
	pCreate					= 0;
	pDestroy				= 0;
	Engine.Event._destroy	();
	XRC.r_clear_compact		();
}

extern "C" {
	typedef bool __cdecl SupportsAdvancedRendering	(void);
	//typedef bool _declspec(dllexport) SupportsDX10Rendering();
	typedef bool _declspec(dllexport) SupportsDX11Rendering();
};

void add_renderer_mode(LPCSTR name, int id)
{
	static int i = 0;
	vid_quality_token[i].id = id;
	vid_quality_token[i].name = name;
	i++;
	return;
}

void CEngineAPI::CreateRendererList()
{
#ifdef DEDICATED_SERVER
	vid_quality_token = xr_alloc<xr_token>(2);

	vid_quality_token[0].id = 0;
	vid_quality_token[0].name = xr_strdup("renderer_r1");

	vid_quality_token[1].id = -1;
	vid_quality_token[1].name = NULL;
#else
	BOOL bSupports_r1 = FALSE;
	Log("Try loading DLL:", r1_name);
	hRender = LoadLibrary(r1_name);
	if (hRender)
	{
		FreeLibrary(hRender);
		bSupports_r1 = TRUE;
	}
	Log("* supports: ", bSupports_r1);
	
	BOOL bSupports_r2 = FALSE;
	Log("Try loading DLL:", r2_name);
	hRender = LoadLibrary(r2_name);
	if (hRender)
	{
		SupportsAdvancedRendering* test_rendering = (SupportsAdvancedRendering*)GetProcAddress(hRender, "SupportsAdvancedRendering");
		R_ASSERT(test_rendering);
		bSupports_r2 = test_rendering();
		FreeLibrary(hRender);
	}
	Log("* supports: ", bSupports_r2);

	BOOL bSupports_r4 = FALSE;
	Log("Try loading DLL:", r4_name);
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

	u32 size = 1;

	if (bSupports_r1) // r1
		size += 1; 
	
	if (bSupports_r2) // r2a, r2, r2.5
		size += 3; 

	if (bSupports_r4) // r4
		size += 1; 

	if(!bSupports_r1 && !bSupports_r2 && !bSupports_r4)
		CHECK_OR_EXIT(!FAILED(E_FAIL), 
			make_string("Ты зачем рендер украл?\n%s:%s\n%s:%s\n%s:%s\n%s:%s\n%s:%s", r1_name, "украдено", r2_name, "спизженно", r4_name, "потеряно", "xrRender_R5.dll", "coming soon!", "xrRender_R6.dll", "cumming soon!!"));

	vid_quality_token = xr_alloc<xr_token>(size);

	if (bSupports_r1)
	{
		//add_renderer_mode("renderer_r1", 0);
	}
	
	if (bSupports_r2)
	{
		add_renderer_mode("renderer_r2a", 1);
		//add_renderer_mode("renderer_r2", 2);
		add_renderer_mode("renderer_r2.5", 3);
	}

	if (bSupports_r4)
	{
		add_renderer_mode("renderer_r4", 5);
	}

	vid_quality_token[size - 1].id = -1;
	vid_quality_token[size - 1].name = NULL;

	Msg("Available render modes[%d]:", size - 1);
	for (u32 i = 0; i < size - 1; ++i)
	{
		Msg("[%s]", vid_quality_token[i].name);
	}
#endif
}