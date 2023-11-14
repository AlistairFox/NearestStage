////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_time_manager.cpp
//	Created 	: 05.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALfie time manager class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "alife_time_manager.h"
#include "date_time.h"
#include "../xrEngine/IGame_Persistent.h"

CALifeTimeManager::CALifeTimeManager	(LPCSTR section)
{
	init						(section);
}

CALifeTimeManager::~CALifeTimeManager	()
{
}

void CALifeTimeManager::init			(LPCSTR section)
{
	u32							years,months,days,hours,minutes,seconds;

	string_path save_game_time;
	FS.update_path(save_game_time, "$global_server_data$", "server_data.ltx");
	CInifile* global_server_data = xr_new<CInifile>(save_game_time, true);

	if (global_server_data->line_exist("server_env", "time"))
	{
		Msg("TIME SET");
		sscanf(global_server_data->r_string("server_env", "time"), "%d:%d:%d", &hours, &minutes, &seconds);
	}
	else
	{
		sscanf(pSettings->r_string(section, "start_time"), "%d:%d:%d", &hours, &minutes, &seconds);
	}

	if (global_server_data->line_exist("server_env", "data"))
	{
		sscanf(global_server_data->r_string("server_env", "data"), "%d.%d.%d", &days, &months, &years);
	}
	else
	sscanf						(pSettings->r_string(section,"start_date"),"%d.%d.%d",&days,&months,&years);

	if (global_server_data->line_exist("server_env", "weather"))
	{
		g_pGamePersistent->Environment().SetWeather(global_server_data->r_string("server_env", "weather"));
	}


	xr_delete(global_server_data);

	m_start_game_time			= generate_time(years,months,days,hours,minutes,seconds);
	m_time_factor				= pSettings->r_float(section,"time_factor");
	m_normal_time_factor		= pSettings->r_float(section,"normal_time_factor");
	m_game_time					= m_start_game_time;
	m_start_time				= Device.dwTimeGlobal;
}

void CALifeTimeManager::save			(IWriter	&memory_stream)
{
	m_game_time					= game_time();
	m_start_time				= Device.dwTimeGlobal;
	memory_stream.open_chunk	(GAME_TIME_CHUNK_DATA);
	memory_stream.w				(&m_game_time,		sizeof(m_game_time));
	memory_stream.w_float		(m_time_factor);
	memory_stream.w_float		(m_normal_time_factor);
	memory_stream.close_chunk	();
};

void CALifeTimeManager::load			(IReader	&file_stream)
{
	R_ASSERT2					(file_stream.find_chunk(GAME_TIME_CHUNK_DATA),"Can't find chunk GAME_TIME_CHUNK_DATA!");
	file_stream.r				(&m_game_time,		sizeof(m_game_time));
	m_time_factor				= file_stream.r_float();
	m_normal_time_factor		= file_stream.r_float();
	m_start_time				= Device.dwTimeGlobal;
};
