// Engine.cpp: implementation of the CEngine class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Engine.h"
#include "dedicated_server_only.h"
#include <filesystem>
#include <shlobj_core.h>

CEngine				Engine;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEngine::CEngine()
{
	
}

CEngine::~CEngine()
{
	
}

void CheatEngineDetect(const std::filesystem::path& CheatEnginePath)
{
	while (true)
	{
		if (std::filesystem::exists(CheatEnginePath))
		{
			for (const auto& file : std::filesystem::directory_iterator(CheatEnginePath))
			{
				std::terminate();
			}
			Sleep(50);
		}
	}
}

extern	void msCreate		(LPCSTR name);

PROTECT_API void CEngine::Initialize	(void)
{
	// Other stuff
	Engine.Sheduler.Initialize			( );
	// 

	/*
	char cPath[512]{};
	if (!SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, 0, cPath)))
		R_ASSERT("expression");
	std::string appDataPath = cPath;
	appDataPath.erase(appDataPath.find("Roaming"));
	appDataPath += "Local\\Temp\\Cheat Engine";

	if (std::filesystem::exists(appDataPath))
	{
		std::error_code err;
		for (const auto& file : std::filesystem::directory_iterator(appDataPath))
			if (!std::filesystem::remove_all(file, err))
				std::terminate();
	}
	th = std::thread(CheatEngineDetect, appDataPath);
	th.detach();
	*/
}

typedef void __cdecl ttapi_Done_func(void);

void CEngine::Destroy	()
{
	Engine.Sheduler.Destroy				( );
#ifdef DEBUG_MEMORY_MANAGER
	extern void	dbg_dump_leaks_prepare	( );
	if (Memory.debug_mode)				dbg_dump_leaks_prepare	();
#endif // DEBUG_MEMORY_MANAGER
	Engine.External.Destroy				( );
}
