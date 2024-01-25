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
		writer->open_chunk(ACTOR_STATS_CHUNK);
		writer->w_u32(ps->money_for_round); // Player Money
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

void game_sv_freemp::SavePlayer(game_PlayerState* ps, CInifile* file)
{
	CObject* obj = Level().Objects.net_Find(ps->GameID);
	CActor* actor = smart_cast<CActor*>(obj);
	CInventoryOwner* pInvOwner = smart_cast<CInventoryOwner*>(obj);
  
	if (actor && actor->g_Alive())
	{
		TIItemContainer items;
		actor->inventory().AddAvailableItems(items, false);
		u32 id = 0;
		
		for (auto item : items)
		{
			if (!xr_strcmp("mp_players_rukzak", item->m_section_id.c_str()))
				continue;

			if (item->BaseSlot() == DOSIMETER_SLOT)
				continue;

			id += 1;
			string32 itemID;
			xr_sprintf(itemID, "item_%d", id);
 
			file->w_string(itemID, "section", item->m_section_id.c_str());
			file->w_u16(itemID, "slot", item->CurrValue());
			
			if (item->GetCondition() < 1)
				file->w_float(itemID, "condition", item->GetCondition());
			
			if (item->cast_weapon_ammo())
			{
				CWeaponAmmo* ammo = smart_cast<CWeaponAmmo*>(item);
				file->w_u16(itemID, "ammo_count", ammo->m_boxCurr);
			}

			if (item->cast_weapon())
			{
				CWeapon* wpn = smart_cast<CWeapon*>(item);
				file->w_u16(itemID, "ammo_count", u16(wpn->GetAmmoElapsed()));
				file->w_u8(itemID, "ammo_type", wpn->m_ammoType);
				file->w_u8(itemID, "addon_State", wpn->GetAddonsState());
				file->w_u8(itemID, "cur_scope", wpn->m_cur_scope);
			}

			if (item->has_any_upgrades())
			{
				string2048 upgrades;
				item->get_upgrades(upgrades);
				file->w_string(itemID, "upgrades", upgrades);
			}
		}

		CTorch* pTorch = smart_cast<CTorch*>(actor->inventory().ItemFromSlot(TORCH_SLOT));
		//сохранение фонарика
		if (pTorch)
		{
			file->w_string("player_device", "torch_section", pTorch->m_section_id.c_str());
			file->w_float("player_device", "torch_charge", pTorch->GetCondition());
		}

		CCustomDetector* detector = smart_cast<CCustomDetector*>(actor->inventory().ItemFromSlot(DETECTOR_SLOT));
		//сохранение детектора
		if (detector)
		{
			file->w_string("player_device", "detector_section", detector->m_section_id.c_str());
			file->w_float("player_device", "detector_charge", detector->GetCondition());
		}

		CDetectorAnomaly* pAnDet = smart_cast<CDetectorAnomaly*>(actor->inventory().ItemFromSlot(DOSIMETER_SLOT));
		//сохранение детектора аномалий
		if (pAnDet)
		{
			file->w_string("player_device", "anomaly_detector_section", pAnDet->m_section_id.c_str());
			file->w_float("player_device", "anomaly_detector_charge", pAnDet->GetCondition());
		}

		CPda* pPda = smart_cast<CPda*>(actor->inventory().ItemFromSlot(PDA_SLOT));
		//сохранения пда
		if (pPda)
		{
			file->w_string("player_device", "pda_section", pPda->m_section_id.c_str());
		}

		file->w_u32("actor", "items_count", id);
		file->w_u32("actor", "money", ps->money_for_round);
		file->w_u8("actor", "team", ps->team);

		CSE_ALifeCreatureActor* actor_cse = smart_cast<CSE_ALifeCreatureActor*>(server().ID_to_entity(ps->GameID));

		if (actor_cse)
		{
			file->w_fvector3("actor_position", "pos", actor_cse->o_Position);
			file->w_fvector3("actor_position", "angle", actor_cse->o_Angle);
		}
	}

}

void game_sv_freemp::SavePlayerOutfits(game_PlayerState* ps, CInifile* outfsFile)
{
	CObject* obj = Level().Objects.net_Find(ps->GameID);
	CActor* actor = smart_cast<CActor*>(obj);
	if (!actor)
		return;

	if (smart_cast<CInventoryOwner*>(obj))
	{
		CCustomOutfit* pOutfit = smart_cast <CCustomOutfit*>(actor->inventory().ItemFromSlot(OUTFIT_SLOT));
		if (pOutfit)
		{
			outfits data;
			data.player_name = ps->getName();
			data.outfit_cond = pOutfit->GetCondition();
			data.outfit_name = pOutfit->m_section_id.c_str();
			
			auto PS = std::find_if(save_outfits.begin(), save_outfits.end(), [&](outfits data)
				{
					if (strstr(data.player_name, ps->getName()))
						return true;
					else
						return false;
				});

			if (PS == save_outfits.end())
				save_outfits.push_back(data);
			else
				(*PS) = data;
		}
		else
		{
			auto it =
			std::find_if(save_outfits.begin(), save_outfits.end(), [&](outfits data)
				{
					if (strstr(data.player_name, ps->getName()))
						return true;
					else
						return false;
				});
			if (it != save_outfits.end())
				save_outfits.erase(it);
		}
	}
}

void game_sv_freemp::SavePlayerDetectors(game_PlayerState* ps, CInifile* detsFile)
{
	CObject* obj = Level().Objects.net_Find(ps->GameID);
	CActor* actor = smart_cast<CActor*>(obj);
	if (!actor)
		return;
	if (smart_cast<CInventoryOwner*>(obj))
	{
		CCustomDetector* pDet = smart_cast<CCustomDetector*>(actor->inventory().ItemFromSlot(DETECTOR_SLOT));
		if (pDet)
		{
			detectors data;
			data.player_name = ps->getName();
			data.detector_cond = pDet->GetCondition();
			data.detector_name = pDet->m_section_id.c_str();

			auto DS = std::find_if(save_detectors.begin(), save_detectors.end(), [&](detectors data)
				{
					if (strstr(data.player_name, ps->getName()))
						return true;
					else
						return false;
				});

			if (DS == save_detectors.end())
				save_detectors.push_back(data);
			else
				(*DS) = data;
		}
		else
		{
			auto name = 
			std::find_if(save_detectors.begin(), save_detectors.end(), [&](detectors data)
				{

					if (strstr(data.player_name, ps->getName()))
						return true;
					else
						return false;
				});
			if(name != save_detectors.end())
			save_detectors.erase(name);
		}
	}
}

bool game_sv_freemp::BinnarLoadPlayer(game_PlayerState* ps, string_path& filepath)
{
	if (FS.exist(filepath))
	{
		IReader* reader = FS.r_open(filepath);

		if (reader->open_chunk(ACTOR_STATS_CHUNK))
		{
			ps->money_for_round = reader->r_u32();// money
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

bool game_sv_freemp::LoadPlayer(game_PlayerState* ps, CInifile* file)
{
	if (file->section_exist("actor"))
	{
		u32 count = file->r_u32("actor", "items_count");
		ps->money_for_round = file->r_u32("actor", "money");
		if (file->line_exist("actor", "unicue_icon"))
		{
			CActor* pActor = smart_cast<CActor*>(Level().Objects.net_Find(ps->GameID));
			pActor->SetChatIcon(file->r_string("actor", "unicue_icon"));
		}

		Msg("[game_sv_freemp] LoadPlayer [%s] items[%d]", ps->getName(), count);

		//загрузка фонарика
		if (file->line_exist("player_device", "torch_section"))
		{
			LPCSTR torch_name = file->r_string("player_device", "torch_section");
			float torch_cond = 0;
			if(file->line_exist("player_device", "torch_charge"))
			torch_cond = file->r_float("player_device", "torch_charge");

			CSE_Abstract* E = spawn_begin(torch_name);
			E->ID_Parent = ps->GameID;
			CSE_ALifeInventoryItem* item = smart_cast<CSE_ALifeInventoryItem*>(E);

			item->m_fCondition = torch_cond;
			spawn_end(E, m_server->GetServerClient()->ID);
		}

		//загрузка детектора
		if (file->line_exist("player_device", "detector_section"))
		{
			LPCSTR name = file->r_string("player_device", "detector_section");

			float det_cond = 0;

			if (file->line_exist("player_device", "detector_charge"))
				float det_cond = file->r_float("player_device", "detector_charge");

			CSE_Abstract* E = spawn_begin(name);
			E->ID_Parent = ps->GameID;
			CSE_ALifeInventoryItem* item = smart_cast<CSE_ALifeInventoryItem*>(E);
			item->m_fCondition = det_cond;
			spawn_end(E, m_server->GetServerClient()->ID);
		}

		//загрузка детектора аномалий
		if (file->line_exist("player_device", "anomaly_detector_section"))
		{
			LPCSTR pAnomDetector = file->r_string("player_device", "anomaly_detector_section");
			float pAnomDetector_cond = 0;
			if (file->line_exist("player_device", "anomaly_detector_charge"));
			pAnomDetector_cond = file->r_float("player_device", "anomaly_detector_charge");

			CSE_Abstract* E = spawn_begin(pAnomDetector);
			E->ID_Parent = ps->GameID;
			CSE_ALifeInventoryItem* item = smart_cast<CSE_ALifeInventoryItem*>(E);

			item->m_fCondition = pAnomDetector_cond;
			spawn_end(E, m_server->GetServerClient()->ID);
		}

		//загрузка пда
		if (file->line_exist("player_device", "pda_section"))
		{
			LPCSTR pPda = file->r_string("player_device", "pda_section");
			CSE_Abstract* E = spawn_begin(pPda);
			E->ID_Parent = ps->GameID;
			spawn_end(E, m_server->GetServerClient()->ID);
		}

		for (u32 id = 1; id != count + 1; id++)
		{
			string32 itemID;
			xr_sprintf(itemID, "item_%d", id);


			if (file->section_exist(itemID))
			{
				LPCSTR name = file->r_string(itemID, "section");

				CSE_Abstract* E = spawn_begin(name);

				E->ID_Parent = ps->GameID;

				CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
				CSE_ALifeItemWeapon* wpn = smart_cast<CSE_ALifeItemWeapon*>(item);
				CSE_ALifeItemAmmo* ammo = smart_cast<CSE_ALifeItemAmmo*>(item);

				if (file->line_exist(itemID, "condition"))
				{
					float cond = file->r_float(itemID, "condition");
					item->m_fCondition = cond;
				}

				if (wpn)
				{
					u16 ammo_count = file->r_u16(itemID, "ammo_count");
					u8 ammo_type = file->r_u8(itemID, "ammo_type");
					u8 addon_state = file->r_u8(itemID, "addon_State");
					u8 cur_scope = file->r_u8(itemID, "cur_scope");
					wpn->a_elapsed = ammo_count;
					wpn->ammo_type = ammo_type;
					wpn->m_addon_flags.flags = addon_state;
					wpn->m_cur_scope = cur_scope;
				}

				if (ammo)
				{
					u16 ammo_current = file->r_u16(itemID, "ammo_count");
					ammo->a_elapsed = ammo_current;
				}

				if (file->line_exist(itemID, "slot"))
				{
					u16 slot = file->r_u16(itemID, "slot");
					item->slot = slot;
				}

				if (file->line_exist(itemID, "upgrades"))
				{
					LPCSTR upgrades = file->r_string(itemID, "upgrades");
					u32 count = _GetItemCount(upgrades, ',');

					for (u32 id = 0; id != count; id++)
					{
						string64 upgrade;
						_GetItem(upgrades, id, upgrade, ',');
						item->add_upgrade(upgrade);
					}
				}


				spawn_end(E, m_server->GetServerClient()->ID);
			}
		}
		return true;
	}
	else
		return false;
}

void game_sv_freemp::LoadPlayerOtfits(game_PlayerState* ps, CInifile* outfsFile)
{
	auto PN = std::find_if(save_outfits.begin(), save_outfits.end(), [&](outfits data)
		{
			if (strstr(data.player_name, ps->getName()))
				return true;
			else
				return false;
		});

	if (PN == save_outfits.end())
		return;
	

	LPCSTR section = (*PN).outfit_name;
	float cond = (*PN).outfit_cond;

	Msg("%s Load Outfit Sect: %s, Cond: %f ",ps->getName(), section, cond);

		if (ps->testFlag(GAME_PLAYER_MP_SAVE_LOADED))
		{
			cond /= Random.randF(1.1, 2);
		}

		CSE_Abstract* E = spawn_begin(section);
		CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
		if (item)
		{
			item->m_fCondition = cond;
			item->ID_Parent = ps->GameID;
			spawn_end(item, m_server->GetServerClient()->ID);
		}
}

void game_sv_freemp::LoadPlayerDetectors(game_PlayerState* ps, CInifile* detsFile)
{
	auto PD = std::find_if(save_detectors.begin(), save_detectors.end(), [&](detectors data)
		{
			if (strstr(data.player_name, ps->getName()))
				return true;
			else
				return false;
		});

	if (PD == save_detectors.end())
		return;

	LPCSTR section = (*PD).detector_name;
	float cond = (*PD).detector_cond;
	if (cond > 1)
		cond = 1;

	Msg("%s Load Detector: %s",ps->getName(), section);

		CSE_Abstract* E = spawn_begin(section);
		CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
		if (item)
		{
			item->ID_Parent = ps->GameID;
			item->m_fCondition = cond;
			spawn_end(item, m_server->GetServerClient()->ID);
		}
}

bool game_sv_freemp::HasSaveFile(game_PlayerState* ps)
{
	if (ps->GameID == get_id(server().GetServerClient()->ID)->GameID)
		return false;

	if (binar_save)
	{
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
			reader->close();
		}
		return exist;
	}
	else
	{
		string_path path;
		string32 filename;
		xr_strcpy(filename, ps->getName());
		xr_strcat(filename, ".ltx");
		FS.update_path(path, "$mp_saves_players$", filename);
		bool exist = false;
		FS.rescan_path(path, true);

		if (FS.exist(path))
		{
			CInifile* file = xr_new<CInifile>(path, true);
			if (file->section_exist("actor"))
				exist = true;
			if (file && file->section_exist("actor") && file->line_exist("actor", "team"))
				ps->team = file->r_u8("actor", "team");
			xr_delete(file);
		}

		return exist;
	}
}

void game_sv_freemp::SaveInvBox(CSE_ALifeInventoryBox* box, CInifile* file)
{
	u32 ids = 0;
	for (auto id : box->children)
	{
		ids += 1;
		string32 itemID;
		xr_sprintf(itemID, "item_%d", ids);

		CInventoryItem* item = smart_cast<CInventoryItem*>( Level().Objects.net_Find(id));

		if (!item)
			return;

		file->w_string(itemID, "section", item->m_section_id.c_str());
 
		if (item->GetCondition() < 1)
			file->w_float(itemID, "condition", item->GetCondition());

		if (item->cast_weapon_ammo())
		{
			CWeaponAmmo* ammo = smart_cast<CWeaponAmmo*>(item);
			file->w_u16(itemID, "ammo_count", ammo->m_boxCurr);
		}

		if (item->cast_weapon())
		{
			CWeapon* wpn = smart_cast<CWeapon*>(item);
			file->w_u16(itemID, "ammo_count", u16(wpn->GetAmmoElapsed()));
			file->w_u8(itemID, "ammo_type", wpn->m_ammoType);
			file->w_u8(itemID, "addon_State", wpn->GetAddonsState());
			file->w_u8(itemID, "cur_scope", wpn->m_cur_scope);
		}


		if (item->has_any_upgrades())
		{
			string2048 upgrades;
			item->get_upgrades(upgrades);
			file->w_string(itemID, "upgrades", upgrades);
		}

		
	}

	file->w_u32("box", "items_count", ids);

}

void game_sv_freemp::LoadInvBox(CSE_ALifeInventoryBox* box, CInifile* file)
{
	if (!file->section_exist("box"))
		return;

	if (!box)
		return;

	u32 count = file->r_u32("box", "items_count");

	// Msg("[game_sv_freemp] LoadInvBox [%d] items[%d]", box->ID, count);
    
	for (u32 id = 1; id != count + 1; id++)
	{
		string32 itemID;
		xr_sprintf(itemID, "item_%d", id);
 
		if (file->section_exist(itemID))
		{
			LPCSTR name = file->r_string(itemID, "section");				
			CSE_Abstract* E = spawn_begin(name);

			E->ID_Parent = box->ID;

			CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
			CSE_ALifeItemWeapon* wpn = smart_cast<CSE_ALifeItemWeapon*>(item);
			CSE_ALifeItemAmmo* ammo = smart_cast<CSE_ALifeItemAmmo*>(item);

			if (file->line_exist(itemID, "condition"))
			{
				float cond = file->r_float(itemID, "condition");
				item->m_fCondition = cond;
			}

			if (wpn)
			{
				u16 ammo_count = file->r_u16(itemID, "ammo_count");
				u8 ammo_type = file->r_u8(itemID, "ammo_type");
				u8 addon_state = file->r_u8(itemID, "addon_State");
				u8 cur_scope = file->r_u8(itemID, "cur_scope");
				wpn->a_elapsed = ammo_count;
				wpn->ammo_type = ammo_type;
				wpn->m_addon_flags.flags = addon_state;
				wpn->m_cur_scope = cur_scope;
			}

			if (ammo)
			{
				u16 ammo_current = file->r_u16(itemID, "ammo_count");
				ammo->a_elapsed = ammo_current;
			}


			if (file->line_exist(itemID, "upgrades"))
			{
				LPCSTR upgrades = file->r_string(itemID, "upgrades");
				u32 count = _GetItemCount(upgrades, ',');

				for (u32 id = 0; id != count; id++)
				{
					string64 upgrade;
					_GetItem(upgrades, id, upgrade, ',');
					item->m_upgrades.push_back(upgrade);
				}
			}

			spawn_end(E, m_server->GetServerClient()->ID);
		}
	}
}

void game_sv_freemp::assign_RP(CSE_Abstract* E, game_PlayerState* ps_who)
{

		Fvector pos, angle;
		if (!ps_who->testFlag(GAME_PLAYER_MP_SAVE_LOADED) && load_position_RP(ps_who, pos, angle))
		{
			E->o_Position.set(pos);
			E->o_Angle.set(angle);
		}
		else
			inherited::assign_RP(E, ps_who);
}

bool game_sv_freemp::load_position_RP(game_PlayerState* ps, Fvector& pos, Fvector& angle)
{
	if (binar_save)
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
					reader->close();
					return true;
				}
				reader->close();
				return false;
			}
			reader->close();
			return false;
		}
		return false;
	}
	else
	{
		string_path p;
		string32 filename;
		xr_strcpy(filename, ps->getName());
		xr_strcat(filename, ".ltx");

		FS.update_path(p, "$mp_saves_players$", filename);

		CInifile* f = xr_new<CInifile>(p);
		if (f && f->section_exist("actor") && f->line_exist("actor_position", "pos"))
		{
			pos = f->r_fvector3("actor_position", "pos");
			angle = f->r_fvector3("actor_position", "angle");

			//E->o_Position.set(pos);
			//E->o_Angle.set(0, 0, 0);
			xr_delete(f);
			return true;
		}

		return false;
	}
}
