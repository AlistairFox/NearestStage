#include "StdAfx.h"
#include "server_progress_saver.h"

void  CProgressSaver::SaveThreadWorker()
{
	while (true)
	{

		csSaving.Enter();
		if (ThreadTasks.size() == 0)
		{
			csSaving.Leave();
			Sleep(save_time4 * 1000);
			continue;
		}

		SThreadTask task = ThreadTasks.back();
		ThreadTasks.pop_back();
		csSaving.Leave();

		if (InvBox* OutBox = task.box)
		{
			IWriter* writer = FS.w_open(OutBox->box_path);

			writer->open_chunk(INVBOX_ITEMS_CHUNK);

			writer->w_u16(OutBox->Items.size());

			for (const auto& id : OutBox->Items)
			{
				writer->w_stringZ(id.ItemSect);
				writer->w_float(id.ItemCond);
				if (id.IsWeaponAmmo)
				{
					writer->w_u8(1);
					writer->w_u16(id.AmmoBoxCurr);
				}
				else
					writer->w_u8(0);

				if (id.IsWeapon)
				{
					writer->w_u8(1);
					writer->w_u16(id.AmmoElapsed);
					writer->w_u8(id.AmmoType);
					writer->w_u8(id.AddonState);
					writer->w_u8(id.CurrScope);
				}
				else
					writer->w_u8(0);

				if (id.HasUpgr)
				{
					writer->w_u8(1);
					writer->w_stringZ(id.Uphrades);
				}
				else
					writer->w_u8(0);

			}
			writer->close_chunk();
			FS.w_close(writer);
			xr_delete(OutBox);

		}

		if (Players* player = task.players)
		{
			std::string InvPath = player->PlayerPath;
			InvPath += player->PlayerName;
			InvPath += "_inventory.binsave";
			IWriter* writer = FS.w_open(InvPath.c_str());
			writer->open_chunk(ACTOR_MONEY);
			writer->w_s32(player->Stats.money);
			writer->close_chunk();

			writer->open_chunk(ACTOR_STATS_CHUNK);
			writer->w_float(player->Stats.satiety);
			writer->w_float(player->Stats.thirst);
			writer->w_float(player->Stats.radiation);
			writer->close_chunk();

			writer->open_chunk(ACTOR_TEAM);
			writer->w_u8(player->Stats.team);
			writer->close_chunk();

			writer->open_chunk(ACTOR_INV_ITEMS_CHUNK);
			writer->w_u32(player->Items.size());

			for (const auto& item : player->Items)
			{
				writer->w_stringZ(item.ItemSect);
				writer->w_u16(item.ItemSlot);
				writer->w_float(item.ItemCond);
				if (item.IsWeaponAmmo)
				{
					writer->w_u8(1);
					writer->w_u16(item.AmmoBoxCurr);
				}
				else
					writer->w_u8(0);

				if (item.IsWeapon)
				{
					writer->w_u8(1);
					writer->w_u16(item.AmmoElapsed);
					writer->w_u8(item.AmmoType);
					writer->w_u8(item.AddonState);
					writer->w_u8(item.CurrScope);
				}
				else
					writer->w_u8(0);

				if (item.HasUpgr)
				{
					writer->w_u8(1);
					writer->w_stringZ(item.Uphrades);
				}
				else
					writer->w_u8(0);
			}
			writer->close_chunk();
			FS.w_close(writer);

			std::string DialogsPath = player->PlayerPath;
			DialogsPath += player->PlayerName;
			DialogsPath += "_dialogs.binsave";
			writer = FS.w_open(DialogsPath.c_str());
			writer->open_chunk(INFO_PORTIONS_CHUNK);
			writer->w_u32(player->InfoPortions.size());
			for (const auto& Info : player->InfoPortions)
			{
				writer->w_stringZ(Info);
			}
			writer->close_chunk();

			std::string PossPath = player->PlayerPath;
			PossPath += player->PlayerName;
			PossPath += "_position.binsave";

			writer = FS.w_open(PossPath.c_str());
			writer->open_chunk(ACTOR_POS);
			if (player->Stats.SetPossition)
			{
				writer->w_u8(1);
				writer->w_fvector3(player->Stats.pos);
				writer->w_fvector3(player->Stats.angle);
			}
			else
				writer->w_u8(0);
			writer->close_chunk();

			xr_delete(player);
		}

		if (OnDeathDisconnect* dis = task.DisconnectBuf)
		{
			IWriter* writer = FS.w_open(dis->PlayerPath);

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

		if (GlobalServerData* GSData = task.ServerData)
		{
			IWriter* env_writer = FS.w_open(GSData->GSDPath);
			env_writer->open_chunk(ENV_CHUNK);
			env_writer->w_stringZ(GSData->Time);
			env_writer->w_stringZ(GSData->Data);
			env_writer->w_stringZ(GSData->Weather);
			env_writer->close_chunk();
			FS.w_close(env_writer);
			xr_delete(GSData);
		}

	}
}
