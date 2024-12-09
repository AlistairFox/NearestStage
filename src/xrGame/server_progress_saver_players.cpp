#include "stdafx.h"
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

		buff.PlayerMoney = ps->money_for_round;
		buff.Team = ps->team;

		CCustomOutfit* pOutfit = smart_cast<CCustomOutfit*>(actor->inventory().ItemFromSlot(OUTFIT_SLOT));
		if (pOutfit)
		{
			buff.Outfit = true;
			buff.OutfitName = pOutfit->m_section_id.c_str();
			buff.OutfitCond = pOutfit->GetCondition();
			buff.OutfitSlot = pOutfit->CurrValue();
			if (pOutfit->has_any_upgrades())
			{
				buff.OutfUpg = true;
				pOutfit->get_upgrades(buff.OutfitUpgrades);
			}
			else
				buff.OutfUpg = false;
		}
		else
			buff.Outfit = false;

		CHelmet* pHelm = smart_cast<CHelmet*>(actor->inventory().ItemFromSlot(HELMET_SLOT));
		if (pHelm)
		{
			buff.helm = true;
			buff.HelmetName = pHelm->m_section_id.c_str();
			buff.HelmetCond = pHelm->GetCondition();
			buff.HelmSlot = pHelm->CurrValue();
			if (pHelm->has_any_upgrades())
			{
				buff.HelmUpg = true;
				pHelm->get_upgrades(buff.HelmetUpgrades);
			}
			else
				buff.HelmUpg = false;
		}
		else
			buff.helm = false;

		CCustomDetector* pDet = smart_cast<CCustomDetector*>(actor->inventory().ItemFromSlot(DETECTOR_SLOT));
		if (pDet)
		{
			buff.detector = true;
			buff.DetectorName = pDet->m_section_id.c_str();
			buff.DetectorCond = pDet->GetCondition();
			buff.DetectorSlot = pDet->CurrValue();
		}
		else
			buff.detector = false;

		CWeapon* pWpn1 = smart_cast<CWeapon*>(actor->inventory().ItemFromSlot(INV_SLOT_2));
		if (pWpn1)
		{
			buff.weapon1 = true;
			buff.Weapon1Sect = pWpn1->m_section_id.c_str();
			buff.Weapon1Cond = pWpn1->GetCondition();
			buff.Weapon1CurScope = pWpn1->m_cur_scope;
			buff.Weapon1AddonState = pWpn1->GetAddonsState();
			buff.Weapon1Slot = pWpn1->CurrValue();

			if (pWpn1->has_any_upgrades())
			{
				buff.weapon1Upgr = true;
				pWpn1->get_upgrades(buff.Weapon1Upgrades);
			}
			else
				buff.weapon1Upgr = false;
		}
		else
			buff.weapon1 = false;

		CWeapon* pWpn2 = smart_cast<CWeapon*>(actor->inventory().ItemFromSlot(INV_SLOT_3));
		if (pWpn2)
		{
			buff.weapon2 = true;
			buff.Weapon2Sect = pWpn2->m_section_id.c_str();
			buff.Weapon2Cond = pWpn2->GetCondition();
			buff.Weapon2CurScope = pWpn2->m_cur_scope;
			buff.Weapon2AddonState = pWpn2->GetAddonsState();
			buff.Weapon2Slot = pWpn2->CurrValue();

			if (pWpn2->has_any_upgrades())
			{
				buff.weapon2Upgr = true;
				pWpn2->get_upgrades(buff.Weapon2Upgrades);
			}
			else
				buff.weapon2Upgr = false;
		}
		else
			buff.weapon2 = false;

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
		ThreadTasks.push_back({ nullptr, pl, nullptr, nullptr });
		csSaving.Leave();
	}

}

bool CProgressSaver::BinnarLoadPlayer(game_PlayerState* ps)
{
	string_path PlayerSavePath;
	string256 PlayerDir;
	sprintf(PlayerDir, "%s\\%s_inventory.binsave", ps->getName(), ps->getName());
	FS.update_path(PlayerSavePath, "$mp_saves_players_bin$", PlayerDir);
	if (af_debug_loggining)
		Msg("AFPROGRESSAVER: InvPath: %s", PlayerSavePath);
	if (FS.exist(PlayerSavePath))
	{
		if (af_debug_loggining)
			Msg("AFPROGRESSAVER: read file path = %s", PlayerSavePath);
		IReader* reader = FS.r_open(PlayerSavePath);

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

		if (reader->open_chunk(ACTOR_TEAM))
		{
			u8 team = reader->r_u8();
		}

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
		reader->close();
	}

#ifdef INFO_PORTIONS_SAVING
	string_path PlayerDialogsPath;
	string256 PlayerDirDialogs;
	sprintf(PlayerDirDialogs, "%s\\%s_dialogs.binsave", ps->getName(), ps->getName());
	FS.update_path(PlayerDialogsPath, "$mp_saves_players_bin$", PlayerDirDialogs);

	if (af_debug_loggining)
		Msg("AFPROGRESSAVER: DialogsPath: %s", PlayerDialogsPath);
	if (FS.exist(PlayerDialogsPath))
	{
		if (af_debug_loggining)
			Msg("AFPROGRESSAVER: read file path = %s", PlayerDialogsPath);
		IReader* reader = FS.r_open(PlayerDialogsPath);
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

		reader->close();
	}
#endif
	return true;
}

bool CProgressSaver::HasBinnarSaveFile(game_PlayerState* ps)
{
	if (ps->GameID == Level().Server->game->get_id(Level().Server->GetServerClient()->ID)->GameID)
		return false;

	string_path PlayerSavePath;
	string256 PlayerDir;
	sprintf(PlayerDir, "%s\\%s_inventory.binsave", ps->getName(), ps->getName());
	FS.update_path(PlayerSavePath, "$mp_saves_players_bin$", PlayerDir);
	bool exist = false;
	if (FS.exist(PlayerSavePath))
	{
		IReader* reader = FS.r_open(PlayerSavePath);

		if (reader->open_chunk(ACTOR_TEAM))
		{
			exist = true;
			u8 player_team = reader->r_u8();
			if (af_debug_loggining)
				Msg("AFPROGRESSAVER: %d", player_team);
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
	sprintf(PlayerDir, "%s\\%s_position.binsave", ps->getName(), ps->getName());
	FS.update_path(PlayerPosPath, "$mp_saves_players_bin$", PlayerDir);
	if (af_debug_loggining)
		Msg("AFPROGRESSAVER: PossPath: %s", PlayerPosPath);
	if (FS.exist(PlayerPosPath))
	{
		if (af_debug_loggining)
			Msg("AFPROGRESSAVER: read player pos: %s", PlayerPosPath);
		IReader* reader = FS.r_open(PlayerPosPath);

		if (reader->open_chunk(ACTOR_POS))
		{
			bool ActorPossitionCheck = reader->r_u8();
			if (ActorPossitionCheck)
			{
				if (af_debug_loggining)
					Msg("AFPROGRESSAVER: Read player pos");
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
	sprintf(PlayerSaveDir, "%s\\%s_inventory.binsave", ps->getName(), ps->getName());
	FS.update_path(PlayerSavePath, "$mp_saves_players_bin$", PlayerSaveDir);
	if (FS.exist(PlayerSavePath))
	{
		WillRemove = true;
		FS.file_delete(PlayerSavePath);
	}

	string_path PlayerPosPath;
	string256 PlayerPossDir;
	sprintf(PlayerPossDir, "%s\\%s_position.binsave", ps->getName(), ps->getName());
	FS.update_path(PlayerPosPath, "$mp_saves_players_bin$", PlayerPossDir);
	if (FS.exist(PlayerPosPath))
	{
		FS.file_delete(PlayerPosPath);
	}

	return WillRemove;
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
	sprintf(PlayerSaveDir, "%s\\%s_inventory.binsave", Name, Name);
	FS.update_path(PlayerSavePath, "$mp_saves_players_bin$", PlayerSaveDir);
	if (!FS.exist(PlayerSavePath))
		FillPlayerOnDisconnect(StaticID, PlayerSavePath);

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