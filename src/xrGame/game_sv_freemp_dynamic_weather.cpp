#include "StdAfx.h"
#include "game_sv_freemp.h"
#include <ui/UIInventoryUtilities.h>
#include <Level.h>

void game_sv_freemp::DynamicWeatherUpdate()
{
	/// calculate dynamic weather
	if (Level().game)
	{
		shared_str envtime = InventoryUtilities::GetGameTimeAsString(InventoryUtilities::etpTimeToMinutes);

		if (!need_change_weather)
		{
			if (xr_strcmp(envtime, "01:00") == 0)
			{
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
						def = "[main_cycle_multy]";
						random = 0.0f;
						changeweather = true;
					}

					if (random <= 0.50f && (xr_strcmp(curr_weather, "[main_cycle_foggy_day]") != 0) && !changeweather)
					{
						def = "[main_cycle_foggy_day]";
						random = 0.0f;
						changeweather = true;
					}

					if (random <= 0.75f && (xr_strcmp(curr_weather, "[main_cycle_sunny_day]") != 0) && !changeweather)
					{
						random = 0.0f;
						def = "[main_cycle_sunny_day]";
						changeweather = true;
					}

					if (random > 0.75f && (xr_strcmp(curr_weather, "[main_cycle_thunder_year]") != 0) && !changeweather)
					{
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
				}

			}
		}
	}

	if (need_change_weather)
	{
		if (Level().game && Device.dwFrame % 4000 == 0)
		{
			need_change_weather = false;
		}
	}
	/// calculate dynamic weather
}