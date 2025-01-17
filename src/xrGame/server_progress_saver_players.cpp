﻿#include "stdafx.h"
#include "server_progress_saver.h"

void CProgressSaver::FillPlayerBuffer(game_PlayerState* ps)
{
	CObject* obj = Level().Objects.net_Find(ps->GameID);
	CActor* actor = smart_cast<CActor*>(obj);

	if (actor && actor->g_Alive())
	{
		Players* pl = xr_new<Players>(ps);

#ifdef	PLAYERONDEATH_SAVING
		if (MPlayersOnDeath.find(ps->GetStaticID()) != MPlayersOnDeath.end())
			MPlayersOnDeath.erase(ps->GetStaticID());

		SPlayersOnDeathBuff buff;

		CCustomOutfit* pOutfit = smart_cast<CCustomOutfit*>(actor->inventory().ItemFromSlot(OUTFIT_SLOT));
		if (pOutfit)
		{
			SItem Item(pOutfit);
			buff.OnDeathItems.push_back(Item);
		}

		CHelmet* pHelm = smart_cast<CHelmet*>(actor->inventory().ItemFromSlot(HELMET_SLOT));
		if (pHelm)
		{
			SItem Item(pHelm);
			buff.OnDeathItems.push_back(Item);
		}

		CCustomDetector* pDet = smart_cast<CCustomDetector*>(actor->inventory().ItemFromSlot(DETECTOR_SLOT));
		if (pDet)
		{
			SItem Item(pDet);
			buff.OnDeathItems.push_back(Item);
		}

		CWeapon* pWpn1 = smart_cast<CWeapon*>(actor->inventory().ItemFromSlot(INV_SLOT_2));
		if (pWpn1)
		{
			SItem Item(pWpn1);
			buff.OnDeathItems.push_back(Item);
		}

		CWeapon* pWpn2 = smart_cast<CWeapon*>(actor->inventory().ItemFromSlot(INV_SLOT_3));
		if (pWpn2)
		{
			SItem Item(pWpn2);
			buff.OnDeathItems.push_back(Item);
		}


		MPlayersOnDeath[ps->GetStaticID()] = buff;
#endif


		TIItemContainer items;
		AddSaveAvailableItems(actor, items);

		for (const auto itm : items)
		{
			SItem pItem(itm);
			pl->Items.push_back(pItem);
		}


		csSaving.Enter();
		ThreadTasks.push_back(SThreadTask(pl));
		csSaving.Leave();
	}

}

bool CProgressSaver::BinnarLoadPlayer(game_PlayerState* ps)
{
	string256 PlayerRootDir;
	sprintf(PlayerRootDir, "%s\\%s", ps->getName(), ps->getName());

	string_path PlayerStatsDir;
	string256 PlayerStatsFile;
	strcpy(PlayerStatsFile, PlayerRootDir);
	strcat(PlayerStatsFile, STATS_STR_FORMAT);
	FS.update_path(PlayerStatsDir, PLAYERSAVE_DIRECTORY, PlayerStatsFile);
	if (FS.exist(PlayerStatsDir))
	{
		if (af_debug_loggining)
			Msg("AFPROGRESSAVER: Opening player stats file: %s", PlayerStatsDir);

		IReader* reader = FS.r_open(PlayerStatsDir);
		LoadPlayerStats(reader, ps);
		FS.r_close(reader);
	}

	string_path PlayerInventoryDir;
	string256 PlayerInventoryFile;
	strcpy(PlayerInventoryFile, PlayerRootDir);
	strcat(PlayerInventoryFile, INVENTORY_STR_FORMAT);
	FS.update_path(PlayerInventoryDir, PLAYERSAVE_DIRECTORY, PlayerInventoryFile);
	if (FS.exist(PlayerInventoryDir))
	{
		if (af_debug_loggining)
			Msg("AFPROGRESSAVER: Opening player inventory file: %s", PlayerInventoryDir);

		IReader* reader = FS.r_open(PlayerInventoryDir);
		LoadPlayerInventory(reader, ps);
		FS.r_close(reader);
	}

#ifdef INFO_PORTIONS_SAVING
	string_path PlayerDialogsDir;
	string256 PlayerDialogsFile;
	strcpy(PlayerDialogsFile, PlayerRootDir);
	strcat(PlayerDialogsFile, DIALOGS_STR_FORMAT);
	FS.update_path(PlayerDialogsDir, PLAYERSAVE_DIRECTORY, PlayerDialogsFile);
	if (FS.exist(PlayerDialogsDir))
	{
		if (af_debug_loggining)
			Msg("AFPROGRESSAVER: Opening player dialogs file: %s", PlayerDialogsDir);
		IReader* reader = FS.r_open(PlayerDialogsDir);
		LoadPlayerDialogs(reader, ps);
		FS.r_close(reader);
	}
#endif
	return true;
}

bool CProgressSaver::LoadPlayerStats(IReader* reader, game_PlayerState* ps)
{
	if (reader->open_chunk(ACTOR_MONEY))
		ps->money_for_round = reader->r_u32();// money

#ifdef PLAYER_STATS_SAVING
	if (reader->open_chunk(ACTOR_STATS_CHUNK))
	{
		NET_Packet P;
		Level().Server->game->u_EventGen(P, GE_PLAYER_LOAD_CONDITIONS, ps->GameID);
		float satiety, thirst, radiation;
		satiety = reader->r_float();
		thirst = reader->r_float();
		radiation = reader->r_float();
		P.w_float(satiety);
		P.w_float(thirst);
		P.w_float(radiation);
		Level().Server->game->u_EventSend(P);
	}
#endif

	return true;
}

bool CProgressSaver::LoadPlayerInventory(IReader* reader, game_PlayerState* ps)
{
	if (reader->open_chunk(ACTOR_INV_ITEMS_CHUNK))
	{
		shared_str itm_sect;
		u32 count = reader->r_u32();

		for (u32 i = 0; i != count; i++)
		{
			reader->r_stringZ(itm_sect);

			if (itm_sect.size() < 2)
				break;

			u16 slot = reader->r_u16();
			float cond = reader->r_float();

			u32 ItemType = reader->r_u32();


			CSE_Abstract* E = Level().Server->game->spawn_begin(itm_sect.c_str());

			E->ID_Parent = ps->GameID;

			CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
			item->m_fCondition = cond;
			item->slot = slot;

			if (ItemType == SItem::ItemTypes::WeaponAmmo)
			{
				CSE_ALifeItemAmmo* ammo = smart_cast<CSE_ALifeItemAmmo*>(item);
				u16 ammo_cnt = reader->r_u16();
				ammo->a_elapsed = ammo_cnt;
			}

			if (ItemType == SItem::ItemTypes::Weapon)
			{
				CSE_ALifeItemWeapon* wpn = smart_cast<CSE_ALifeItemWeapon*>(item);

				u16 ammo_count = reader->r_u16();
				u8 ammo_type = reader->r_u8();
				u8 addon_state = reader->r_u8();
				u8 cur_scope = reader->r_u8();
				wpn->a_elapsed = ammo_count;
				wpn->ammo_type = ammo_type;
				wpn->m_addon_flags.flags = addon_state;
				wpn->m_cur_scope = cur_scope;
			}

			bool CheckUpgrades = reader->r_u8();

			if (CheckUpgrades)
			{
				shared_str upgrades;
				reader->r_stringZ(upgrades);
				u32 upgrCount = _GetItemCount(upgrades.c_str(), ',');

				for (u32 id = 0; id != upgrCount; id++)
				{
					string64 upgrade;
					_GetItem(upgrades.c_str(), id, upgrade, ',');
					item->add_upgrade(upgrade);
				}
			}

			Level().Server->game->spawn_end(E, Level().Server->GetServerClient()->ID);
		}
	}

	return true;
}

bool CProgressSaver::LoadPlayerDialogs(IReader* reader, game_PlayerState* ps)
{
	if (reader->open_chunk(INFO_PORTIONS_CHUNK))
	{
		u32 size = reader->r_u32();

		for (u32 i = 0; i != size; i++)
		{
			shared_str SingleInfo;
			reader->r_stringZ(SingleInfo);
			Player_portions[ps->GetStaticID()].push_back(SingleInfo);
		}

		NET_Packet P;
		Level().Server->game->u_EventGen(P, GE_GET_SAVE_PORTIONS, ps->GameID);
		save_data(Player_portions[ps->GetStaticID()], P);
		P.w_u8(true);
		Level().Server->game->u_EventSend(P);
	}
	return true;
}

void CProgressSaver::OnPlayerDeath(game_PlayerState* ps)
{
	RemovePlayerSave(ps);
}

bool CProgressSaver::HasBinnarSaveFile(game_PlayerState* ps)
{
	if (ps->GameID == Level().Server->game->get_id(Level().Server->GetServerClient()->ID)->GameID)
		return false;


	string256 PlayerRootDir;
	sprintf(PlayerRootDir, "%s\\%s", ps->getName(), ps->getName());


	string_path PlayerTeamDir;
	string256 PlayerTeamFile;
	strcpy(PlayerTeamFile, PlayerRootDir);
	strcat(PlayerTeamFile, TEAMDATA_STR_FORMAT);
	FS.update_path(PlayerTeamDir, PLAYERSAVE_DIRECTORY, PlayerTeamFile);
	bool exist = false;
	if (FS.exist(PlayerTeamDir))
	{
		IReader* reader = FS.r_open(PlayerTeamDir);

		if (reader->open_chunk(ACTOR_TEAM))
		{
			exist = true;
			u8 player_team = reader->r_u8();
			if (af_debug_loggining)
				Msg("AFPROGRESSAVER: Set PlayerTeam %d", player_team);
			ps->team = player_team;
		}
		FS.r_close(reader);
	}
	return exist;
}

bool CProgressSaver::load_position_RP_Binnar(game_PlayerState* ps, Fvector& pos, Fvector& angle)
{
	string_path PlayerPosPath;
	string256 PlayerDir;
	sprintf(PlayerDir, "%s\\%s%s", ps->getName(), ps->getName(), POSITION_STR_FORMAT);
	FS.update_path(PlayerPosPath, PLAYERSAVE_DIRECTORY, PlayerDir);

	if (FS.exist(PlayerPosPath))
	{
		if (af_debug_loggining)
			Msg("AFPROGRESSAVER: Opening player position file: %s", PlayerPosPath);

		IReader* reader = FS.r_open(PlayerPosPath);

		if (reader->open_chunk(ACTOR_POS))
		{
			bool ActorPossitionCheck = reader->r_u8();
			if (ActorPossitionCheck)
			{
				if (af_debug_loggining)
					Msg("AFPROGRESSAVER: Set Player Position!");

				reader->r_fvector3(pos);
				reader->r_fvector3(angle);
				FS.r_close(reader);
				return true;
			}
			FS.r_close(reader);
			return false;
		}
		FS.r_close(reader);
		return false;
	}
	return false;
}

void CProgressSaver::SavePlayersConditions(float satiety, float thirst, float radiation, game_PlayerState* ps)
{
#ifdef PLAYER_STATS_SAVING
	if (!ps)
		return;

	Players_condition[ps->GetStaticID()].satiety = satiety;
	Players_condition[ps->GetStaticID()].thirst = thirst;
	Players_condition[ps->GetStaticID()].radiation = radiation;
#endif
}

void CProgressSaver::LoadPlayerPortions(game_PlayerState* ps)
{
#ifdef INFO_PORTIONS_SAVING
	NET_Packet P;
	Level().Server->game->u_EventGen(P, GE_GET_SAVE_PORTIONS, ps->GameID);
	save_data(Player_portions[ps->GetStaticID()], P);
	P.w_u8(false);
	Level().Server->game->u_EventSend(P);
#endif // INFO_PORTIONS_SAVING
}


void CProgressSaver::SavePlayerPortions(ClientID sender, shared_str info_id, bool add)
{
#ifdef INFO_PORTIONS_SAVING
	game_PlayerState* ps = Level().Server->game->get_id(sender);

	if (ps)
	{
		auto it = std::find_if(Player_portions[ps->GetStaticID()].begin(), Player_portions[ps->GetStaticID()].end(), [&](shared_str& data)
			{
				return data.equal(info_id);
			});

		if (add)
		{
			if (it == Player_portions[ps->GetStaticID()].end())
			{
				if (af_debug_loggining)
					Msg("AFPROGRESSAVER: Player: %s get portion: %s", ps->getName(), info_id.c_str());
				Player_portions[ps->GetStaticID()].push_back(info_id);
			}
			else
				return;
		}
		else
		{
			if (it != Player_portions[ps->GetStaticID()].end())
			{
				if (af_debug_loggining)
					Msg("AFPROGRESSAVER: Player: %s lost portion: %s", ps->getName(), info_id.c_str());
				Player_portions[ps->GetStaticID()].erase(it);
			}
			else
				return;
		}
	}

#endif
}

bool CProgressSaver::RemovePlayerSave(game_PlayerState* ps)
{
	bool WillRemove = false;
	string_path PlayerSavePath;
	string256 PlayerSaveDir;
	sprintf(PlayerSaveDir, "%s\\%s%s", ps->getName(), ps->getName(), INVENTORY_STR_FORMAT);
	FS.update_path(PlayerSavePath, PLAYERSAVE_DIRECTORY, PlayerSaveDir);
	FillRemoveFilesList(PlayerSavePath);

	string_path PlayerPosPath;
	string256 PlayerPossDir;
	sprintf(PlayerPossDir, "%s\\%s%s", ps->getName(), ps->getName(),POSITION_STR_FORMAT);
	FS.update_path(PlayerPosPath, PLAYERSAVE_DIRECTORY, PlayerPossDir);
	FillRemoveFilesList(PlayerPosPath);

	return WillRemove;
}

void CProgressSaver::FillRemoveFilesList(string_path path)
{
	if (!FS.exist(path))
		return;

	if (af_debug_loggining)
		Msg("AFPROGRESSAVER: File Add to Remove Task: %s", path);


	FileToDelete* ftd = xr_new<FileToDelete>();
	strcpy(ftd->PPath,path);

	csSaving.Enter();
	ThreadTasks.push_back(SThreadTask(ftd));
	csSaving.Leave();
}

void CProgressSaver::OnPlayerRespawn(game_PlayerState* ps)
{
	if (!ps)
		return;


	if (ps->testFlag(GAME_PLAYER_MP_SAVE_LOADED))
	{
		LoadPlayerPortions(ps);
		LoadPlayersOnDeath(ps);
		
		ps->money_for_round /= Random.randF(1.f, 1.2f);
	}
	else
	{
		fmp->SpawnItemToActor(ps->GameID, "wpn_binoc");

		BinnarLoadPlayer(ps);

		ps->setFlag(GAME_PLAYER_MP_SAVE_LOADED);
	}
}

void CProgressSaver::OnPlayerDisconnect(LPSTR Name, u16 StaticID)
{
	string_path PlayerSavePath;
	string256 PlayerSaveDir;
	sprintf(PlayerSaveDir, "%s\\%s%s", Name, Name, INVENTORY_STR_FORMAT);
	FS.update_path(PlayerSavePath, PLAYERSAVE_DIRECTORY, PlayerSaveDir);
	if (!FS.exist(PlayerSavePath))
		FillPlayerOnDisconnect(StaticID, PlayerSavePath, Name);

	ClearPlayersOnDeathBuffer(StaticID);

#ifdef INFO_PORTIONS_SAVING
	Player_portions[StaticID].clear();
#endif
}

void CProgressSaver::AddSaveAvailableItems(CActor* actor, TIItemContainer& items_container) const
{
	for (TIItemContainer::const_iterator it = actor->inventory().m_ruck.begin(); actor->inventory().m_ruck.end() != it; ++it)
	{
		PIItem pIItem = *it;
		items_container.push_back(pIItem);
	}


	for (TIItemContainer::const_iterator it = actor->inventory().m_belt.begin(); actor->inventory().m_belt.end() != it; ++it)
	{
		PIItem pIItem = *it;
		items_container.push_back(pIItem);
	}

	u16 I = actor->inventory().FirstSlot();
	u16 E = actor->inventory().LastSlot();
	for (; I <= E; ++I)
	{
		if (I == BOLT_SLOT)
			continue;
		PIItem item = actor->inventory().ItemFromSlot(I);
		if (item)
		{
			items_container.push_back(item);
		}
	}
}