#include "StdAfx.h"
#include "server_progress_saver.h"
#include <ui/UIInventoryUtilities.h>
#include <DbgHelp.h>

CProgressSaver* CProgressSaver::m_Instance = NULL;
CProgressSaver::CProgressSaver(game_sv_freemp* Game) : fmp(Game)
{
	if (m_Instance)
		Debug.fatal(DEBUG_INFO, "AFPROGRESSAVER: Progress Saver Already Create!");

	m_Instance = this;

#ifndef INFO_PORTIONS_SAVING
	Msg("!!AFPROGRESSAVER: InfoPortions Saving DISABLED!!");
#endif
#ifndef CHECK_BOX_FILE
	Msg("!!AFPROGRESSAVER: PreCheck Box file DISABLED!!");
#endif
#ifndef PLAYER_STATS_SAVING
	Msg("!!AFPROGRESSAVER: Player Stats Saving DISABLED!!");
#endif
#ifndef SERVER_ENV_SAVING
	Msg("!!AFPROGRESSAVER: Server Environment Saving DISABLED!!");
#endif
#ifndef PLAYERONDEATH_SAVING
	Msg("!!AFPROGRESSAVER: PlayerOnDeath Saving DISABLED!!");
#endif
#ifndef FRACTIONUPGRADE_SAVING
	Msg("!!AFPROGRESSAVER: FractionUpgrade Saving DISABLED!!");
#endif
#ifndef PRIVATE_INVBOX_SAVING
	Msg("!!AFPROGRESSAVER: PrivateInvBox Saving DISABLED!!");
#endif // !PRIVATE_INVBOX_SAVING
#ifndef LOGINSYSTEM_WRITER
	Msg("!!AFPROGRESSAVER: LoginSystem Writer DISABLED!!");
#endif


	SetThreadState(ThreadStop);

#ifdef CHECK_BOX_FILE
	string_path box_check_path;
	FS.update_path(box_check_path, "$mp_saves_logins$", "saving_box_list.ltx");
	bool Exist = FS.exist("$mp_saves_logins$", "saving_box_list.ltx") ? true : false;
	BoxCheckFile = xr_new<CInifile>(box_check_path, Exist ? true : false, true);

	if (!Exist)
	{
		BoxCheckFile->w_string("box_list", "example", "example", "это лишь пример записи!");
		BoxCheckFile->save_as(box_check_path);
	}
#endif // CHECK_BOX_FILE

	ThreadStarter();
}

CProgressSaver::~CProgressSaver()
{
#ifdef CHECK_BOX_FILE
	xr_delete(BoxCheckFile);
#endif
}

#ifdef FRACTIONUPGRADE_SAVING
void CProgressSaver::FillFractionUpgrades()
{
	FractionUpgradeTask* task = xr_new<FractionUpgradeTask>();

	FS.update_path(task->team_path, "$fraction_upgrade$", "team_upgrades.ltx");
	task->MFractUpgradeTask = CServerCommunityUpgrade::Get()->MUpgrades;

	csSaving.Enter();
	ThreadTasks.push_back({ nullptr, nullptr, nullptr, nullptr, task });
	csSaving.Leave();
}
#endif

void CProgressSaver::FillServerEnvBuffer()
{
#ifdef SERVER_ENV_SAVING
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
#endif // SERVER_ENV_SAVING
}

void CProgressSaver::LoadServerEnvironment(u32& hours, u32& minutes, u32& seconds, u32& days, u32& months, u32& years, LPCSTR section)
{
	bool SaveLoad = false;
#ifdef SERVER_ENV_SAVING
	string_path save_game_time;
	FS.update_path(save_game_time, "$global_server_data_bin$", "server_data.binsave");
	if (FS.exist(save_game_time))
	{
		IReader* env_reader = FS.r_open(save_game_time);
		if (env_reader->open_chunk(ENV_CHUNK))
		{
			Msg("--AFPROGRESSAVER: Server Environment Save Loaded!");
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
#endif
	if (!SaveLoad)
	{
		Msg("!! AFPROGRESSAVER: Can't Find Server Environment Save File!");
		sscanf(pSettings->r_string(section, "start_time"), "%d:%d:%d", &hours, &minutes, &seconds);
		sscanf(pSettings->r_string(section, "start_date"), "%d.%d.%d", &days, &months, &years);
	}
}

void CProgressSaver::SaveManagerUpdate()
{
	if (!ThreadIsWorking())
		return;


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
#ifdef CHECK_BOX_FILE
				else if (BoxCheckFile->line_exist("box_list", box->name_replace()))
#else
				else
#endif
				{
					FillInvBoxBuffer(box);
				}
			}
		}
	}

#ifdef FRACTIONUPGRADE_SAVING
	if (Level().game && FractionUpgradeFillTimer <= Device.dwTimeGlobal)
	{
		FractionUpgradeFillTimer = Device.dwTimeGlobal + (save_time5 * 1000);
		FillFractionUpgrades();
	}
#endif
}

void CProgressSaver::ThreadStarter()
{
	SetThreadState(ThreadStarting);
	Msg("$$AFPROGRESSAVER: Saver Thread Was Started!");
	SaverThread = new std::thread([&]()
		{
			thread_name("Progress Saver Thread");
			ThreadWorker();
		});


	Msg("$$AFPROGRESSAVER: Saver Thread Was Detach!");
	SaverThread->detach();
}

void CProgressSaver::ThreadWorker()
{

	while (true)
	{
		try
		{
			if (!SaveStageManager())
			{
				Msg("!!AFPROGRESSAVER: ERROR: SaveStageManager!");
				break;
			}
		}
		catch (...)
		{
			Msg("! AFPROGRESSAVER: !!!CRITICAL ERROR!!! Stopping Save Thread");
			break;
		}

		if (NeedStopThread)
		{
			NeedStopThread = false;
			break;
		}
	}

	csSaving.Enter();

	for (auto task : ThreadTasks)
	{
		xr_delete(task.box);
		xr_delete(task.DisconnectBuf);
		xr_delete(task.players);
		xr_delete(task.ServerData);
	}
	ThreadTasks.clear();
	Msg("!!AFPROGRESSAVER: ThreadTasks Clear!");
	csSaving.Leave();

	SetThreadState(ThreadStop);
	Msg("!!AFPROGRESSAVER: Saver Thread Will Destroyed! Need Reload SaveThread!");
}

void CProgressSaver::StopSaveThread()
{
	if (m_iThreadState == ThreadStop)
	{

		Msg("!!AFPROGRESSAVER: Thread Already Stop!!");
		return;
	}

	while (ThreadIsWorking())
	{
		if (m_iThreadState != ThreadWait)
		{
			Msg("!!AFPROGRESSAVER: Waiting Thread Timeout!");
			Sleep(10);
			continue;
		}
		else
		{
			NeedStopThread = true;
			Msg("--AFPROGRESSAVER: Starting Thread Destroy Process!");
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
	case ThreadSaveFractionUpgr:
		return "ThreadSaveFractionUpgr";
	default:
		return "unknown error";
	}
}

void CProgressSaver::AddConsoleDebugInfo(CServerInfo* si)
{
	string128 threadstate;
	sprintf(threadstate, "SaverThreadState: %s", GetThreadStateAsString());
	si->AddItem("AFPROGRESSAVER:", threadstate, RGB(120, 0, 255));
}
