#include "StdAfx.h"
#include "server_progress_saver.h"

bool CProgressSaver::SaveStageManager()
{
	csSaving.Enter();
	if (ThreadTasks.size() == 0)
	{
		SetThreadState(ThreadWait);
		csSaving.Leave();
		Sleep(save_time4 * 1000);
		return true;
	}

	SThreadTask task = ThreadTasks.back();
	ThreadTasks.pop_back();
	csSaving.Leave();

	if (!PlayerSaveStage(&task))
		return false;
	if (!InvBoxSaveStage(&task))
		return false;
	if (!GSDSaveStage(&task))
		return false;
	if (!OnDeathSaveStage(&task))
		return false;
	if (!FractionUpgradeSaveStage(&task))
		return false;
	if (!RemoveFileStage(&task))
		return false;

	return true;
}

bool CProgressSaver::PlayerSaveStage(SThreadTask* task)
{
	if (Players* player = task->players)
	{
		SetThreadState(ThreadSavePlayer);
		std::string PlayerStatsPath = player->PlayerPath;
		PlayerStatsPath += player->PlayerName;
		PlayerStatsPath += STATS_STR_FORMAT;

		IWriter* writer = FS.w_open(PlayerStatsPath.c_str());

		if (!writer)
			return false;

		player->PlayerSaveOutput(writer);

		xr_delete(player);
	}

	return true;
}

bool CProgressSaver::InvBoxSaveStage(SThreadTask* task)
{
	if (InvBox* OutBox = task->box)
	{

		SetThreadState(ThreadSaveInvBox);
		IWriter* writer = FS.w_open(OutBox->box_path);

		if (!writer)
			return false;

		writer->open_chunk(INVBOX_ITEMS_CHUNK);

		OutBox->OutputItems(writer);

		writer->close_chunk();
		FS.w_close(writer);
		xr_delete(OutBox);

	}

	return true;
}

bool CProgressSaver::GSDSaveStage(SThreadTask* task)
{
	if (GlobalServerData* GSData = task->ServerData)
	{
		SetThreadState(ThreadSaveEnvData);
		IWriter* env_writer = FS.w_open(GSData->GSDPath);

		if (!env_writer)
			return false;

		env_writer->open_chunk(ENV_CHUNK);
		env_writer->w_stringZ(GSData->Time);
		env_writer->w_stringZ(GSData->Data);
		env_writer->w_stringZ(GSData->Weather);
		env_writer->close_chunk();
		FS.w_close(env_writer);
		xr_delete(GSData);
	}

	return true;
}

bool CProgressSaver::OnDeathSaveStage(SThreadTask* task)
{
	if (OnDeathDisconnect* dis = task->DisconnectBuf)
	{
		SetThreadState(ThreadSaveOnDeath);

		IWriter* writer = FS.w_open(dis->PlayerPath);
		if (!writer)
			return false;

		writer->open_chunk(ACTOR_INV_ITEMS_CHUNK);
		u32 itemscount = dis->Items.OnDeathItems.size();

		writer->w_u32(itemscount);

		for (auto& TItem : dis->Items.OnDeathItems)
		{
			TItem.OutputItem(writer);
		}

		writer->close_chunk();
		FS.w_close(writer);
		xr_delete(dis);

	}

	return true;
}

bool CProgressSaver::FractionUpgradeSaveStage(SThreadTask* task)
{
#ifdef FRACTIONUPGRADE_SAVING
	if (FractionUpgradeTask* MUpgrade = task->FractUpgr)
	{
		SetThreadState(ThreadSaveFractionUpgr);
		CInifile* file = xr_new<CInifile>(MUpgrade->team_path, false, false);

		if (!file)
			return false;

		for (const auto& team : MUpgrade->MFractUpgradeTask)
		{
			char TeamSect[64];
			sprintf(TeamSect, "team_%d", team.first);
			file->w_float(TeamSect, "current_exp", team.second.m_iCurrentExp, "текущий опыт");
			file->w_u8(TeamSect, "upgrade_points", team.second.m_iUpgradePoints, "кол-во очков прокачки");
			file->w_u16(TeamSect, "current_level", team.second.m_iCurrentLevel, "текущий уровень");

			file->w_u32(TeamSect, "money_in_bank", team.second.MoneyInBank, "деньги в банке");
			file->w_u8(TeamSect, "money_level", team.second.m_iCurrMoneyLevel, "Уровень заработка");
			file->w_u8(TeamSect, "money_count_level", team.second.m_iCurrMoneyCountLevel, "Уровень кол-ва заработка");

			file->w_u8(TeamSect, "invbox_capacity_level", team.second.m_iCurrInvBoxCapacityLevel, "Уровень вместимости ящика");
			file->w_u8(TeamSect, "inv_box_level", team.second.m_iCurrInvBoxLevel, "Уровень ящика");

			file->w_u8(TeamSect, "exp_level", team.second.m_iCurrExpLevel, "Уровень бонуса опыта");

			file->w_u8(TeamSect, "bank_level", team.second.m_iCurrBankLevel, "Уровень банка");

			file->w_u8(TeamSect, "box_provide_level", team.second.m_iCurrBoxProvideLevel, "Уровень получаеммой провизии");
			file->w_u8(TeamSect, "expadd_level", team.second.m_iCurrExpUpgrLevel, "Уровень способов получения опыта");

			file->w_u8(TeamSect, "expselling_level", team.second.m_iCurrExpSellingLevel, "Уровень получения опыта при продаже предметов");

			file->w_u8(TeamSect, "terr1_level", team.second.m_iCurrTerr1Level, "Уровень территории 1");
			file->w_u8(TeamSect, "terr2_level", team.second.m_iCurrTerr2Level, "Уровень территории 2");
			file->w_u8(TeamSect, "terr3_level", team.second.m_iCurrTerr3Level, "Уровень территории 3");
		}

		file->save_as(MUpgrade->team_path);
		xr_delete(file);
		xr_delete(MUpgrade);
	}
#endif
	return true;
}

bool CProgressSaver::RemoveFileStage(SThreadTask* task)
{
	if (FileToDelete* fd = task->FDelete)
	{
		SetThreadState(ThreadRemoveFiles);

		FS.file_delete(fd->PPath);

		xr_delete(fd);
	}
	return true;
}
