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
		std::string InvPath = player->PlayerPath;
		InvPath += player->PlayerName;
		InvPath += "_inventory.binsave";

		IWriter* writer = FS.w_open(InvPath.c_str());

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

		writer->open_chunk(ACTOR_MONEY);
		writer->w_s32(dis->Items.PlayerMoney);
		writer->close_chunk();

		writer->open_chunk(ACTOR_TEAM);
		writer->w_u8(dis->Items.Team);
		writer->close_chunk();

		writer->open_chunk(ACTOR_POS);
		writer->w_u8(0);
		writer->close_chunk();

		writer->open_chunk(ACTOR_INV_ITEMS_CHUNK);
		u32 i = 0;
		if (dis->Items.detector)
			i++;
		if (dis->Items.Outfit)
			i++;
		if (dis->Items.helm)
			i++;
		if (dis->Items.weapon1)
			i++;
		if (dis->Items.weapon2)
			i++;
		writer->w_u32(i);

		if (dis->Items.detector)
		{
			writer->w_stringZ(dis->Items.DetectorName);
			writer->w_u16(dis->Items.DetectorSlot);
			writer->w_float(dis->Items.DetectorCond);

			writer->w_u32(SItem::ItemTypes::InventoryItem);
			writer->w_u8(0);
		}

		if (dis->Items.Outfit)
		{
			writer->w_stringZ(dis->Items.OutfitName);
			writer->w_u16(dis->Items.OutfitSlot);
			writer->w_float(dis->Items.OutfitCond);
			writer->w_u32(SItem::ItemTypes::InventoryItem);
			writer->w_u8(0);
			if (dis->Items.OutfUpg)
			{
				writer->w_u8(1);
				writer->w_stringZ(dis->Items.OutfitUpgrades);
			}
			else
				writer->w_u8(0);
		}

		if (dis->Items.helm)
		{
			writer->w_stringZ(dis->Items.HelmetName);
			writer->w_u16(dis->Items.OutfitSlot);
			writer->w_float(dis->Items.HelmetCond);
			writer->w_u32(SItem::ItemTypes::InventoryItem);
			writer->w_u8(0);
			if (dis->Items.HelmUpg)
			{
				writer->w_u8(1);
				writer->w_stringZ(dis->Items.HelmetUpgrades);
			}
			else
				writer->w_u8(0);
		}

		if (dis->Items.weapon1)
		{
			writer->w_stringZ(dis->Items.Weapon1Sect);
			writer->w_u16(dis->Items.Weapon1Slot);
			writer->w_float(dis->Items.Weapon1Cond);
			writer->w_u32(SItem::ItemTypes::Weapon);
			writer->w_u16(0);
			writer->w_u8(0);
			writer->w_u8(dis->Items.Weapon1AddonState);
			writer->w_u8(dis->Items.Weapon1CurScope);
			if (dis->Items.weapon1Upgr)
			{
				writer->w_u8(1);
				writer->w_stringZ(dis->Items.Weapon1Upgrades);
			}
			else
				writer->w_u8(0);

		}

		if (dis->Items.weapon2)
		{
			writer->w_stringZ(dis->Items.Weapon2Sect);
			writer->w_u16(dis->Items.Weapon2Slot);
			writer->w_float(dis->Items.Weapon2Cond);
			writer->w_u32(SItem::ItemTypes::Weapon);
			writer->w_u16(0);
			writer->w_u8(0);
			writer->w_u8(dis->Items.Weapon2AddonState);
			writer->w_u8(dis->Items.Weapon2CurScope);
			if (dis->Items.weapon2Upgr)
			{
				writer->w_u8(1);
				writer->w_stringZ(dis->Items.Weapon2Upgrades);
			}
			else
				writer->w_u8(0);

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
