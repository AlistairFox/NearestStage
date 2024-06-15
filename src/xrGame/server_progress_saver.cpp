#include "StdAfx.h"
#include "server_progress_saver.h"
#include <ui/UIInventoryUtilities.h>

CProgressSaver::CProgressSaver(game_sv_freemp* Game)
{
	fmp = Game;
	FS.update_path(curr_invbox_name, "$mp_check_saves_invbox$", "save_box_list.ltx");
	curr_box_file = xr_new<CInifile>(curr_invbox_name, true);
	SaverThread = new std::thread([&]()
		{
			thread_name("Progress Saving Thread");
			SaveThreadWorker();
		});

	SaverThread->detach();
}

CProgressSaver::~CProgressSaver()
{
	xr_delete(curr_box_file);
}


void CProgressSaver::FillServerEnvBuffer()
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
		xr_strcpy(GSdata->Data, InventoryUtilities::GetDateAsString(Level().Server->game->GetGameTime(), InventoryUtilities::edpDateToNormal).c_str());
		xr_strcpy(GSdata->Weather, g_pGamePersistent->Environment().CurrentWeatherName.c_str());

		csSaving.Enter();
		ThreadTasks.push_back({ nullptr, nullptr, nullptr, GSdata });
		csSaving.Leave();
	}
	///////////////Server environment saving//////////////////////
}

bool CProgressSaver::LoadServerEnvironment(u32& hours, u32& minutes, u32& seconds, u32& days, u32& months, u32& years)
{
	bool SaveLoad = false;
	string_path save_game_time;
	FS.update_path(save_game_time, "$global_server_data_bin$", "server_data.binsave");
	if (FS.exist(save_game_time))
	{
		IReader* env_reader = FS.r_open(save_game_time);
		if (env_reader->open_chunk(0))
		{
			Msg("TIME SET");
			shared_str time;
			shared_str data;
			shared_str weather;
			env_reader->r_stringZ(time);
			env_reader->r_stringZ(data);
			env_reader->r_stringZ(weather);
			sscanf(time.c_str(), "%d:%d:%d", &hours, &minutes, &seconds);
			sscanf(data.c_str(), "%d.%d.%d", &days, &months, &years);
			g_pGamePersistent->Environment().SetWeather(weather.c_str());
			SaveLoad = true;
		}
		FS.r_close(env_reader);
	}
	return SaveLoad;
}

void CProgressSaver::SavingUpdate()
{
	if (Level().game && PlayerSaveTimer <= Device.dwTimeGlobal)
	{
		PlayerSaveTimer = Device.dwTimeGlobal + (save_time * 1000);
		for (const auto& player : Level().game->players)
		{
			if (player.second->testFlag(GAME_PLAYER_MP_SAVE_LOADED))
			{
				if (player.first == Level().Server->GetServerClient()->ID)
					continue;

				CObject* obj = Level().Objects.net_Find(player.second->GameID);
				CActor* actor = smart_cast<CActor*>(obj);
				if (!actor)
					continue;

				if (!actor->g_Alive())
				{
					RemovePlayerSave(player.second);
					continue;
				}

				FillPlayerBuffer(player.second);
			}
		}
	}

	FillServerEnvBuffer();

	if (Level().game && InvBoxFillTimer <= Device.dwTimeGlobal)
	{
		InvBoxFillTimer = Device.dwTimeGlobal + (save_time2 * 1000);
		
		for (const auto& entity : fmp->inventory_boxes_cse)
		{
			CSE_Abstract* abs = entity.second.entity;
			CSE_ALifeInventoryBox* box = smart_cast<CSE_ALifeInventoryBox*>(abs);
			if (box)
			{
				//check saving box or not
				LPCSTR box_name = box->name_replace();
				//
				if (!entity.second.loaded)
				{
					fmp->inventory_boxes_cse[entity.first].loaded = true;
					BinnarLoadInvBox(box);
				}
				else if (curr_box_file->line_exist("saving_boxes", box_name))
				{
					FillInvBoxBuffer(box);
				}
			}
		}
	}
}