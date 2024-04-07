#include "stdafx.h"
#include "game_sv_freemp.h"
#include "Level.h"
#include "Actor.h"
#include "Inventory.h"
#include "Weapon.h"
#include "CustomDetector.h"
#include "xrServer_Objects_ALife.h"
#include "CustomOutfit.h"
#include "Torch.h"
#include "AnomalyDetector.h"
#include "PDA.h"

void game_sv_freemp::BinnarSavePlayer(game_PlayerState* ps, string_path& filepath)
{
	/*
	Очередность чтения файла:
	CHUNK: ACTOR_STATS_CHUNK
	u32: money
	u8: team

	CHUNK: ACTOR_POS
	u8: bool проверяем была ли засейвлена позиция
	if(true)
	fvector3: position
	fvector3: direction
	if(false)
	skip

	CHUNK: ACTOR_DEVICES_CHUNK

	u8: bool проверяем есть ли фонарик
	if true
	stringZ: item
	float: Condition
	if false
	skip

	u8: bool проверяем есть ли детектор
	if true
	stringZ: item
	float: Condition
	if false
	skip

	CHUNK: ACTOR_INV_ITEMS_CHUNK
	u32: items count
	stringZ: item section
	u16: slot
	float: condition
	u8: check ammo
	if true
	u16: ammo count in box
	if false
	skip
	u8: check weapon
	if true
	u16: ammo count in wpn
	u8: ammo type in wpn
	u8: addons
	u8: scope
	if false
	skip
	u8: check upgrades
	if true
	stringZ: upgrades
	if false
	skip

	*/
	CObject* obj = Level().Objects.net_Find(ps->GameID);
	CActor* actor = smart_cast<CActor*>(obj);
	CInventoryOwner* pInvOwner = smart_cast<CInventoryOwner*>(obj);

	IWriter* writer = FS.w_open(filepath);

	if (actor && actor->g_Alive())
	{
		writer->open_chunk(ACTOR_MONEY);
		writer->w_u32(ps->money_for_round); // Player Money
		writer->close_chunk();

		writer->open_chunk(ACTOR_STATS_CHUNK);
		writer->w_float(Players_condition[ps->getName()].satiety);
		writer->w_float(Players_condition[ps->getName()].thirst);
		writer->w_float(Players_condition[ps->getName()].radiation);
		writer->close_chunk();

		writer->open_chunk(ACTOR_TEAM);
		writer->w_u8(ps->team); // Player Team 
		writer->close_chunk();

		writer->open_chunk(ACTOR_POS);
		CSE_ALifeCreatureActor* actor_cse = smart_cast<CSE_ALifeCreatureActor*>(server().ID_to_entity(ps->GameID));

		if (actor_cse)
		{
			writer->w_u8(1); // checking save position
			writer->w_fvector3(actor_cse->o_Position); // Player Position
			writer->w_fvector3(actor_cse->o_Angle); // Player Direction
			writer->w_float(actor_cse->get_health());
		}
		else
			writer->w_u8(0); // cheking save position
		writer->close_chunk();


		writer->open_chunk(ACTOR_DEVICES_CHUNK);
		CTorch* pTorch = smart_cast<CTorch*>(actor->inventory().ItemFromSlot(TORCH_SLOT));
		//сохранение фонарика
		if (pTorch)
		{
			writer->w_u8(1); // cheking torch
			writer->w_stringZ(pTorch->m_section_id.c_str()); // torch section
			writer->w_float(pTorch->GetCondition()); // torch condition
		}
		else
			writer->w_u8(0); // cheking torch

		CCustomDetector* detector = smart_cast<CCustomDetector*>(actor->inventory().ItemFromSlot(DETECTOR_SLOT));
		//сохранение детектора
		if (detector)
		{
			writer->w_u8(1); // cheking detector
			writer->w_stringZ(detector->m_section_id.c_str()); // detector section
			writer->w_float(detector->GetCondition()); // detector condition
		}
		else
			writer->w_u8(0); // cheking torch


		CDetectorAnomaly* pAnDet = smart_cast<CDetectorAnomaly*>(actor->inventory().ItemFromSlot(DOSIMETER_SLOT));
		//сохранение детектора аномалий
		if (pAnDet)
		{
			writer->w_u8(1); // check anom detector
			writer->w_stringZ(pAnDet->m_section_id.c_str()); // anom detector section
			writer->w_float(pAnDet->GetCondition()); // anom detector condition
		}
		else
			writer->w_u8(0); // checking anom detector

		CPda* pPda = smart_cast<CPda*>(actor->inventory().ItemFromSlot(PDA_SLOT));
		//сохранения пда
		if (pPda)
		{
			writer->w_u8(1); // check pda
			writer->w_stringZ(pPda->m_section_id.c_str()); // pda section
		}
		else
			writer->w_u8(0); // check pda

		writer->close_chunk();


		writer->open_chunk(ACTOR_INV_ITEMS_CHUNK);
		TIItemContainer items;
		actor->inventory().AddAvailableItems(items, false);
		writer->w_u32(items.size()); // write item count

		for (auto item : items)
		{
			if (!xr_strcmp("mp_players_rukzak", item->m_section_id.c_str()))
				continue;

			if (item->BaseSlot() == DOSIMETER_SLOT)
				continue;

			writer->w_stringZ(item->m_section_id.c_str()); // write item section
			writer->w_u16(item->CurrValue()); // write item slot
			writer->w_float(item->GetCondition()); // write item condition
			if (item->cast_weapon_ammo())
			{
				writer->w_u8(1); // check wpn ammo
				CWeaponAmmo* ammo = smart_cast<CWeaponAmmo*>(item);
				writer->w_u16(ammo->m_boxCurr); // write ammo count in box
			}
			else
				writer->w_u8(0); // check wpn ammo

			if (item->cast_weapon())
			{
				writer->w_u8(1); // check weapon
				CWeapon* wpn = smart_cast<CWeapon*>(item);
				writer->w_u16(u16(wpn->GetAmmoElapsed())); // write ammo count in wpn
				writer->w_u8(wpn->m_ammoType); // write ammo type in wpn
				writer->w_u8(wpn->GetAddonsState()); // write addons
				writer->w_u8(wpn->m_cur_scope); // write scope
			}
			else
				writer->w_u8(0); // check weapon 

			if (item->has_any_upgrades())
			{
				writer->w_u8(1); // check upgrades
				string2048 upgrades;
				item->get_upgrades(upgrades);
				writer->w_stringZ(upgrades); // write upgrades
			}
			else
				writer->w_u8(0); // check upgrades
		}
		writer->close_chunk();
		FS.w_close(writer);
	}
}

bool game_sv_freemp::BinnarLoadPlayer(game_PlayerState* ps, string_path& filepath)
{
	if (FS.exist(filepath))
	{
		IReader* reader = FS.r_open(filepath);

		if(reader->open_chunk(ACTOR_MONEY))
			ps->money_for_round = reader->r_u32();// money

		if (reader->open_chunk(ACTOR_STATS_CHUNK))
		{
			NET_Packet P;
			u_EventGen(P, GE_PLAYER_LOAD_CONDITIONS, ps->GameID);
			float satiety, thirst, radiation;
			satiety = reader->r_float();
			thirst = reader->r_float();
			radiation = reader->r_float();
			P.w_float(satiety);
			P.w_float(thirst);
			P.w_float(radiation);
			u_EventSend(P);
		}

		if (reader->open_chunk(ACTOR_TEAM))
		{
			u8 team = reader->r_u8();
		}

		if (reader->open_chunk(ACTOR_POS))
		{
			Fvector3 pos, dir;
			reader->r_fvector3(pos);
			reader->r_fvector3(dir);
			reader->r_float();
		}

		if (reader->open_chunk(ACTOR_DEVICES_CHUNK))
		{
			bool CheckTorch = reader->r_u8();

			if (CheckTorch)
			{
				shared_str sect;
				reader->r_stringZ(sect);
				float cond = reader->r_float();


				CSE_Abstract* E = spawn_begin(sect.c_str());
				E->ID_Parent = ps->GameID;
				CSE_ALifeInventoryItem* item = smart_cast<CSE_ALifeInventoryItem*>(E);

				item->m_fCondition = cond;
				spawn_end(E, m_server->GetServerClient()->ID);
			}

			bool CheckDetector = reader->r_u8();

			if (CheckDetector)
			{
				shared_str sect;
				reader->r_stringZ(sect);
				float cond = reader->r_float();

				CSE_Abstract* E = spawn_begin(sect.c_str());
				E->ID_Parent = ps->GameID;
				CSE_ALifeInventoryItem* item = smart_cast<CSE_ALifeInventoryItem*>(E);

				item->m_fCondition = cond;
				spawn_end(E, m_server->GetServerClient()->ID);
			}

			bool CheckAnomDetector = reader->r_u8();

			if (CheckAnomDetector)
			{
				shared_str sect;
				reader->r_stringZ(sect);
				float cond = reader->r_float();

				CSE_Abstract* E = spawn_begin(sect.c_str());
				E->ID_Parent = ps->GameID;
				CSE_ALifeInventoryItem* item = smart_cast<CSE_ALifeInventoryItem*>(E);

				item->m_fCondition = cond;
				spawn_end(E, m_server->GetServerClient()->ID);
			}

			bool CheckPda = reader->r_u8();

			if (CheckPda)
			{
				shared_str sect;
				reader->r_stringZ(sect);

				CSE_Abstract* E = spawn_begin(sect.c_str());
				E->ID_Parent = ps->GameID;
				CSE_ALifeInventoryItem* item = smart_cast<CSE_ALifeInventoryItem*>(E);
				spawn_end(E, m_server->GetServerClient()->ID);
			}
		}

		if (reader->open_chunk(ACTOR_INV_ITEMS_CHUNK))
		{
			shared_str itm_sect;
			u32 count = reader->r_u32();

			for (u32 i = 0; i != count; i++)
			{
				reader->r_stringZ(itm_sect);
				u16 slot = reader->r_u16();
				float cond = reader->r_float();
				CSE_Abstract* E = spawn_begin(itm_sect.c_str());

				E->ID_Parent = ps->GameID;

				CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
				item->m_fCondition = cond;
				item->slot = slot;

				bool CheckAmmo = reader->r_u8();
				if (CheckAmmo)
				{
					CSE_ALifeItemAmmo* ammo = smart_cast<CSE_ALifeItemAmmo*>(item);
					u16 ammo_cnt = reader->r_u16();
					ammo->a_elapsed = ammo_cnt;
				}

				bool CheckWpn = reader->r_u8();

				if (CheckWpn)
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
				spawn_end(E, m_server->GetServerClient()->ID);
			}

		}

		reader->close();
		return true;
	}
	return false;
}

bool game_sv_freemp::HasBinnarSaveFile(game_PlayerState* ps)
{
	if (ps->GameID == get_id(server().GetServerClient()->ID)->GameID)
		return false;

	string_path path;
	string32 filename;
	xr_strcpy(filename, ps->getName());
	xr_strcat(filename, ".binsave");
	FS.update_path(path, "$mp_saves_players_bin$", filename);
	bool exist = false;
	if (FS.exist(path))
	{
		IReader* reader = FS.r_open(path);

		if (reader->open_chunk(ACTOR_TEAM))
		{
			exist = true;
			u8 player_team = reader->r_u8();
			Msg("%d", player_team);
			ps->team = player_team;
		}
		FS.r_close(reader);
	}
	return exist;
}

bool game_sv_freemp::load_position_RP_Binnar(game_PlayerState* ps, Fvector& pos, Fvector& angle, float& health)
{
	string_path p;
	string32 filename;
	xr_strcpy(filename, ps->getName());
	xr_strcat(filename, ".binsave");

	FS.update_path(p, "$mp_saves_players_bin$", filename);
	if (FS.exist(p))
	{
		IReader* reader = FS.r_open(p);

		if (reader->open_chunk(ACTOR_POS))
		{
			bool ActorPossitionCheck = reader->r_u8();
			if (ActorPossitionCheck)
			{
				Msg("Read player pos");
				reader->r_fvector3(pos);
				reader->r_fvector3(angle);
				health = reader->r_float();
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

void game_sv_freemp::SavePlayersConditions(float satiety, float thirst, float radiation, game_PlayerState* ps)
{
	if (!ps)
		return;

	Players_condition[ps->getName()].satiety = satiety;
	Players_condition[ps->getName()].thirst = thirst;
	Players_condition[ps->getName()].radiation = radiation;
}

void game_sv_freemp::LoadPlayerPortions(game_PlayerState* ps, bool first)
{

	if (!first)
	{
		NET_Packet P;
		u_EventGen(P, GE_GET_SAVE_PORTIONS, ps->GameID);
		save_data(Player_portions[ps->getName()], P);
		P.w_u8(false);
		u_EventSend(P);
	}
	else
	{
		string_path path;
		string32 file_name;
		xr_strcpy(file_name, ps->getName());
		xr_strcat(file_name, ".binsave");
		FS.update_path(path, "$mp_saves_info$", file_name);

		if (FS.exist(path))
		{
			IReader* reader = FS.r_open(path);

			if (reader->open_chunk(INFO_PORTIONS_CHUNK))
			{
				u32 count = reader->r_u32();


				for (int i = 0; i < count; i++)
				{
					shared_str item;
					reader->r_stringZ(item);
					Player_portions[ps->getName()].push_back(item);
				}

				NET_Packet P;
				u_EventGen(P, GE_GET_SAVE_PORTIONS, ps->GameID);
				save_data(Player_portions[ps->getName()], P);
				P.w_u8(true);
				u_EventSend(P);

			}

			FS.r_close(reader);
		}
	}
}


void game_sv_freemp::SavePlayerPortions(ClientID sender, shared_str info_id, bool add)
{
	game_PlayerState* ps = get_id(sender);

	if (ps)
	{
		auto it = std::find_if(Player_portions[ps->getName()].begin(), Player_portions[ps->getName()].end(), [&](shared_str& data)
			{
				return data.equal(info_id);
			});

		if (add)
		{
			if (it == Player_portions[ps->getName()].end())
				Player_portions[ps->getName()].push_back(info_id);
			else
				return;
		}
		else
		{
			if (it != Player_portions[ps->getName()].end())
				Player_portions[ps->getName()].erase(it);
			else
				return;
		}


		string_path path;
		string32 file_name;
		xr_strcpy(file_name, ps->getName());
		xr_strcat(file_name, ".binsave");
		FS.update_path(path, "$mp_saves_info$", file_name);
		IWriter* writer = FS.w_open(path);
		writer->open_chunk(INFO_PORTIONS_CHUNK);
		writer->w_u32(Player_portions[ps->getName()].size());
		for (const auto& info : Player_portions[ps->getName()])
		{
			writer->w_stringZ(info);
		}
		writer->close_chunk();
		FS.w_close(writer);
		Msg("Sender: %s, portion: %s", ps->getName(), info_id.c_str());
	}


}