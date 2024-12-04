#include "StdAfx.h"
#include "server_progress_saver.h"
#include <ui/UIInventoryUtilities.h>

CProgressSaver* CProgressSaver::m_Instance = NULL;
CProgressSaver::CProgressSaver(game_sv_freemp* Game) : fmp(Game)
{
	if (m_Instance)
		Debug.fatal(DEBUG_INFO, "Progress Saver Already Create!");

	m_Instance = this;

	SetThreadState(ThreadStop);

	string_path box_check_path;
	FS.update_path(box_check_path, "$mp_saves_logins$", "saving_box_list.ltx");

	bool Exist = FS.exist("$mp_saves_logins$", "saving_box_list.ltx") ? true : false;
	BoxCheckFile = xr_new<CInifile>(box_check_path, Exist ? true : false, true);

	if (!Exist)
	{
		BoxCheckFile->w_string("box_list", "example", "example", "это лишь пример записи!");
		BoxCheckFile->save_as(box_check_path);
	}

	ThreadStarter();
}

CProgressSaver::~CProgressSaver()
{
	xr_delete(BoxCheckFile);
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
		if (env_reader->open_chunk(ENV_CHUNK))
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

void CProgressSaver::SaveManagerUpdate()
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
				if (!entity.second.loaded)
				{
					fmp->inventory_boxes_cse[entity.first].loaded = true;
					BinnarLoadInvBox(box);
				}
				else if (BoxCheckFile->line_exist("box_list", box->name_replace()))
				{
					FillInvBoxBuffer(box);
				}
			}
		}
	}
}

void CProgressSaver::ThreadStarter()
{
	SetThreadState(ThreadStarting);
	Msg("$$ Saver Thread Was Started!");
	SaverThread = new std::thread([&]()
		{
			thread_name("Progress Saver Thread");
			ThreadWorker();
		});


	Msg("$$ Saver Thread Was Detach!");
	SaverThread->detach();
}

void CProgressSaver::ThreadWorker()
{

	while (SaveStageManager())
	{

		if (NeedStopThread)
		{
			NeedStopThread = false;
			break;
		}
		
	}

	SetThreadState(ThreadStop);
	Msg("!! Saver Thread Will Destroyed!");
}

void CProgressSaver::StopSaveThread()
{
	if (m_iThreadState == ThreadStop)
	{

		Msg("!! Thread Already Stop!!");
		return;
	}

	while (ThreadIsWorking())
	{
		if (m_iThreadState != ThreadWait)
		{
			Msg("!! Waiting Thread Timeout!");
			Sleep(10);
			continue;
		}
		else
		{
			NeedStopThread = true;
			Msg("-- Starting Thread Destroy Process!");
			break;
		}
	}


}

LPCSTR CProgressSaver::GetThreadStateAsString()
{
	switch (m_iThreadState)
	{
	case ThreadStop:
		return "ThreadStop";
	case ThreadStarting:
		return "ThreadStarting";
	case ThreadWait:
		return "ThreadWait";
	case ThreadSavePlayer:
		return "ThreadSavePlayer";
	case ThreadSaveInvBox:
		return "ThreadSaveInvBox";
	case ThreadSaveEnvData:
		return "ThreadSaveEnvData";
	case ThreadSaveOnDeath:
		return "ThreadSaveOnDeath";
	default:
		return "unknown error";
	}
}
