#include "StdAfx.h"
#include "game_sv_freemp.h"
#include <ui/UIInventoryUtilities.h>
#include <Level.h>

void game_sv_freemp::FillServerEnvBuffer()
{
	///////////////Server environment saving//////////////////////
	if (Level().game && SaveWeatherTimer <= Device.dwTimeGlobal)
	{
		SaveWeatherTimer = Device.dwTimeGlobal + (save_time3 * 1000);
		GlobalServerData* GSdata = xr_new<GlobalServerData>();

		string_path save_game_time;
		FS.update_path(save_game_time, "$global_server_data_bin$", "server_data.binsave");
		xr_strcpy(GSdata->GSDPath, save_game_time);

		xr_strcpy(GSdata->Time, InventoryUtilities::GetGameTimeAsString(InventoryUtilities::etpTimeToSeconds).c_str());
		xr_strcpy(GSdata->Data, InventoryUtilities::GetDateAsString(GetGameTime(), InventoryUtilities::edpDateToNormal).c_str());
		xr_strcpy(GSdata->Weather, g_pGamePersistent->Environment().CurrentWeatherName.c_str());

		csSaving.Enter();
		ThreadTasks.push_back({ nullptr, nullptr, nullptr, GSdata });
		csSaving.Leave();
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
				shared_str def = "";
				shared_str curr_weather = g_pGamePersistent->Environment().CurrentWeatherName;
				shared_str News_Text = "";
				shared_str Icon = "";

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
						News_Text = "По прогнозу на завтра: Переменная облачность";
						Icon = "ui_inGame2_Radiopomehi";
						def = "[main_cycle_multy]";
						random = 0.0f;
						changeweather = true;
					}

					if (random <= 0.50f && (xr_strcmp(curr_weather, "[main_cycle_foggy_day]") != 0) && !changeweather)
					{
						News_Text = "По прогнозу завтра сильная туманность!";
						Icon = "ui_inGame2_Radiopomehi";
						def = "[main_cycle_foggy_day]";
						random = 0.0f;
						changeweather = true;
					}

					if (random <= 0.75f && (xr_strcmp(curr_weather, "[main_cycle_sunny_day]") != 0) && !changeweather)
					{
						News_Text = "Завтрашний день обещает быть солнечным!";
						Icon = "ui_inGame2_Radiopomehi";
						random = 0.0f;
						def = "[main_cycle_sunny_day]";
						changeweather = true;
					}

					if (random > 0.75f && (xr_strcmp(curr_weather, "[main_cycle_thunder_year]") != 0) && !changeweather)
					{
						News_Text = "Внимание! Завтра будет сильная буря!";
						Icon = "ui_inGame2_Radiopomehi";
						random = 0.0f;
						def = "[main_cycle_thunder_year]";
						changeweather = true;
					}
				}

				if (changeweather)
				{
					Msg("- Calculate end");

					NET_Packet P;
					GenerateGameMessage(P);
					P.w_u32(GAME_EVENT_WEATHER_UPDATE);
					P.w_stringZ("Weather News");
					P.w_stringZ(News_Text);
					P.w_stringZ(Icon);
					server().SendBroadcast(BroadcastCID, P, net_flags(true, true));
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