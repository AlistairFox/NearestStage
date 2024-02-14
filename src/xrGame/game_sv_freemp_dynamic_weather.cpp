#include "StdAfx.h"
#include "game_sv_freemp.h"
#include <ui/UIInventoryUtilities.h>
#include <Level.h>


void game_sv_freemp::ServerEnvSaveUpdateFile()
{
	///////////////Server environment saving//////////////////////
	if (Level().game && Device.dwFrame % save_time3 == 0)
	{
		string_path save_game_time;
		FS.update_path(save_game_time, "$global_server_data$", "server_data.ltx");
		CInifile* global_server_data = xr_new<CInifile>(save_game_time, false, false);
		LPCSTR time = InventoryUtilities::GetGameTimeAsString(InventoryUtilities::etpTimeToSeconds).c_str();
		LPCSTR data = InventoryUtilities::GetDateAsString(GetGameTime(), InventoryUtilities::edpDateToNormal).c_str();
		global_server_data->w_string("server_env", "time", time);
		global_server_data->w_string("server_env", "data", data);
		global_server_data->w_string("server_env", "weather", g_pGamePersistent->Environment().CurrentWeatherName.c_str());
		global_server_data->save_as(save_game_time);
		xr_delete(global_server_data);
	}
	///////////////Server environment saving//////////////////////
}

void game_sv_freemp::ServerEnvSaveUpdateBin()
{
	///////////////Server environment saving//////////////////////
	if (Level().game && Device.dwFrame % save_time3 == 0)
	{
		string_path save_game_time;
		FS.update_path(save_game_time, "$global_server_data_bin$", "server_data.binsave");
		IWriter* env_writer = FS.w_open(save_game_time);
		env_writer->open_chunk(ENV_CHUNK);
		shared_str time = InventoryUtilities::GetGameTimeAsString(InventoryUtilities::etpTimeToSeconds);
		shared_str data = InventoryUtilities::GetDateAsString(GetGameTime(), InventoryUtilities::edpDateToNormal);
		shared_str weather = g_pGamePersistent->Environment().CurrentWeatherName;
		env_writer->w_stringZ(time);
		env_writer->w_stringZ(data);
		env_writer->w_stringZ(weather);
		env_writer->close_chunk();
		FS.w_close(env_writer);
	}
	///////////////Server environment saving//////////////////////
}

void game_sv_freemp::DynamicWeatherUpdate()
{
	/// calculate dynamic weather
	if (Level().game)
	{
		u32 nextday = InventoryUtilities::GetGameDay(GetGameTime());
		
		if (!first_update)
		{
			nowday = nextday;
			first_update = true;
		}

		if (!need_change_weather)
		{
			if (nowday != nextday)
			{
				nowday = nextday;
				bool	changeweather = false;
				float random = 0.0f;
				need_change_weather = true;
				random += Random.randF(0.f, 1.f);
				shared_str def;
				shared_str curr_weather = g_pGamePersistent->Environment().CurrentWeatherName;

				while (!changeweather)
				{
					if (random <= 0.25f && (xr_strcmp(curr_weather, "[main_cycle_multy]") == 0))
					{
						Msg("! !ERROR: overlap weather [main_cycle_multy]!");
						random = Random.randF(0.f, 1.f);
					}
					if (random <= 0.50f && (xr_strcmp(curr_weather, "[main_cycle_foggy_day]") == 0))
					{
						Msg("! !ERROR: overlap weather [main_cycle_foggy_day]!");
						random = Random.randF(0.f, 1.f);
					}
					if (random <= 0.75f && (xr_strcmp(curr_weather, "[main_cycle_sunny_day]") == 0))
					{
						Msg("! !ERROR: overlap weather [main_cycle_sunny_day]!");
						random = Random.randF(0.f, 1.f);
					}
					if (random > 0.75f && (xr_strcmp(curr_weather, "[main_cycle_thunder_year]") == 0))
					{
						Msg("! !ERROR: overlap weather [main_cycle_thunder_year]!");
						random = Random.randF(0.f, 1.f);
					}
					Msg("Calculate next weather: %s, rand: %f", def.c_str(), random);

					if (random <= 0.25f && (xr_strcmp(curr_weather, "[main_cycle_multy]") != 0) && !changeweather)
					{
						NET_Packet P;
						GenerateGameMessage(P);
						P.w_u32(GAME_EVENT_WEATHER_UPDATE);
						P.w_stringZ("Weather News");
						P.w_stringZ("По прогнозу на завтра: Переменная облачность");
						P.w_stringZ("ui_inGame2_Radiopomehi");
						server().SendBroadcast(BroadcastCID, P, net_flags(true, true));
						def = "[main_cycle_multy]";
						random = 0.0f;
						changeweather = true;
					}

					if (random <= 0.50f && (xr_strcmp(curr_weather, "[main_cycle_foggy_day]") != 0) && !changeweather)
					{
						NET_Packet P;
						GenerateGameMessage(P);
						P.w_u32(GAME_EVENT_WEATHER_UPDATE);
						P.w_stringZ("Weather News");
						P.w_stringZ("По прогнозу завтра сильная туманность!");
						P.w_stringZ("ui_inGame2_Radiopomehi");
						server().SendBroadcast(BroadcastCID, P, net_flags(true, true));
						def = "[main_cycle_foggy_day]";
						random = 0.0f;
						changeweather = true;
					}

					if (random <= 0.75f && (xr_strcmp(curr_weather, "[main_cycle_sunny_day]") != 0) && !changeweather)
					{
						NET_Packet P;
						GenerateGameMessage(P);
						P.w_u32(GAME_EVENT_WEATHER_UPDATE);
						P.w_stringZ("Weather News");
						P.w_stringZ("Завтрашний день обещает быть солнечным!");
						P.w_stringZ("ui_inGame2_Radiopomehi");
						server().SendBroadcast(BroadcastCID, P, net_flags(true, true));
						random = 0.0f;
						def = "[main_cycle_sunny_day]";
						changeweather = true;
					}

					if (random > 0.75f && (xr_strcmp(curr_weather, "[main_cycle_thunder_year]") != 0) && !changeweather)
					{
						NET_Packet P;
						GenerateGameMessage(P);
						P.w_u32(GAME_EVENT_WEATHER_UPDATE);
						P.w_stringZ("Weather News");
						P.w_stringZ("Внимание! Завтра будет сильная буря!");
						P.w_stringZ("ui_inGame2_Radiopomehi");
						server().SendBroadcast(BroadcastCID, P, net_flags(true, true));
						random = 0.0f;
						def = "[main_cycle_thunder_year]";
						changeweather = true;
					}
				}

				if (changeweather)
				{
					Msg("- Calculate end");
					g_pGamePersistent->Environment().SetWeather(def, true);
					changeweather = false;
					weather_will_change = true;
				}

			}
		}
	}

	if (need_change_weather && weather_will_change)
	{
			need_change_weather = false;
			weather_will_change = false;
	}
	/// calculate dynamic weather
}