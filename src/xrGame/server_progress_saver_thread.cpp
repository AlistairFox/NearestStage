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

	ThreadWorkerTimer.Start();

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

	TWT = ThreadWorkerTimer.GetElapsed_ticks();

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

		writer->w_u16(OutBox->Items.size());

		for (auto& id : OutBox->Items)
		{
			id.OutputItem(writer);
		}
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
			writer->w_u8(0);
			writer->w_u8(0);
			writer->w_u8(0);
		}

		if (dis->Items.Outfit)
		{
			writer->w_stringZ(dis->Items.OutfitName);
			writer->w_u16(dis->Items.OutfitSlot);
			writer->w_float(dis->Items.OutfitCond);
			writer->w_u8(0);
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
			writer->w_u8(0);
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
			writer->w_u8(0);
			writer->w_u8(1);
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
			writer->w_u8(0);
			writer->w_u8(1);
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
