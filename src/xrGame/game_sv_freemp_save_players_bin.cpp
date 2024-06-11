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
#include "ActorHelmet.h"

void game_sv_freemp::FillPlayerBuffer(game_PlayerState* ps, string_path& filepath)
{
	CObject* obj = Level().Objects.net_Find(ps->GameID);
	CActor* actor = smart_cast<CActor*>(obj);
	CInventoryOwner* pInvOwner = smart_cast<CInventoryOwner*>(obj);
	Players* pl = xr_new<Players>();

	if (actor && actor->g_Alive())
	{
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


		PlayerStats stat;
		xr_strcpy(pl->PlayerPath, filepath);
		stat.money = ps->money_for_round;
		stat.satiety = Players_condition[ps->GetStaticID()].satiety;
		stat.thirst = Players_condition[ps->GetStaticID()].thirst;
		stat.radiation = Players_condition[ps->GetStaticID()].radiation;
		stat.team = ps->team;

		CSE_ALifeCreatureActor* actor_cse = smart_cast<CSE_ALifeCreatureActor*>(server().ID_to_entity(ps->GameID));

		if (actor_cse)
		{
			stat.SetPossition = true;
			stat.pos = actor_cse->o_Position;
			stat.angle = actor_cse->o_Angle;
			stat.health = actor_cse->get_health();
		}
		else
			stat.SetPossition = false;; // cheking save position
		pl->Stats = stat;

		TIItemContainer items;
		actor->inventory().AddSaveAvailableItems(items);

		for (const auto itm : items)
		{
			SItem pItem;

			xr_strcpy(pItem.ItemSect, itm->m_section_id.c_str());
			pItem.ItemSlot = itm->CurrValue();
			pItem.ItemCond = itm->GetCondition();
			if (itm->cast_weapon_ammo())
			{
				pItem.IsWeaponAmmo = true;
				CWeaponAmmo* ammo = smart_cast<CWeaponAmmo*>(itm);
				pItem.AmmoBoxCurr = ammo->m_boxCurr;
			}
			else
				pItem.IsWeaponAmmo = false;

			if (itm->cast_weapon())
			{
				pItem.IsWeapon = true;
				CWeapon* wpn = smart_cast<CWeapon*>(itm);
				pItem.AmmoElapsed = wpn->GetAmmoElapsed();
				pItem.AmmoType = wpn->m_ammoType;
				pItem.AddonState = wpn->GetAddonsState();
				pItem.CurrScope = wpn->m_cur_scope;
			}
			else
				pItem.IsWeapon = false;

			if (itm->has_any_upgrades())
			{
				pItem.HasUpgr = true;
				itm->get_upgrades(pItem.Uphrades);
			}
			else
				pItem.HasUpgr = false;

			pl->Items.push_back(pItem);

		}

		pl->InfoPortions.swap(Player_portions[ps->GetStaticID()]);

	}
	
	csSaving.Enter();
	ThreadTasks.push_back({ nullptr, pl, nullptr, nullptr });
	csSaving.Leave();

}

#ifdef OLD_BOX_SAVING
void game_sv_freemp::BinnarSavePlayer(game_PlayerState* ps, string_path& filepath)
{
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
		writer->w_float(Players_condition[ps->GetStaticID()].satiety);
		writer->w_float(Players_condition[ps->GetStaticID()].thirst);
		writer->w_float(Players_condition[ps->GetStaticID()].radiation);
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
#endif

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
			u_EventGen(P, GE_GET_SAVE_PORTIONS, ps->GameID);
			save_data(Player_portions[ps->GetStaticID()], P);
			P.w_u8(true);
			u_EventSend(P);
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

	Players_condition[ps->GetStaticID()].satiety = satiety;
	Players_condition[ps->GetStaticID()].thirst = thirst;
	Players_condition[ps->GetStaticID()].radiation = radiation;
}

void game_sv_freemp::LoadPlayerPortions(game_PlayerState* ps)
{
	NET_Packet P;
	u_EventGen(P, GE_GET_SAVE_PORTIONS, ps->GameID);
	save_data(Player_portions[ps->GetStaticID()], P);
	P.w_u8(false);
	u_EventSend(P);
}


void game_sv_freemp::SavePlayerPortions(ClientID sender, shared_str info_id, bool add)
{
	game_PlayerState* ps = get_id(sender);

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
				Msg("Player: %s get portion: %s", ps->getName(), info_id.c_str());
				Player_portions[ps->GetStaticID()].push_back(info_id);
			}
			else
				return;
		}
		else
		{
			if (it != Player_portions[ps->GetStaticID()].end())
			{
				Msg("Player: %s lost portion: %s", ps->getName(), info_id.c_str());
				Player_portions[ps->GetStaticID()].erase(it);
			}
			else
				return;
		}
	}


}

void game_sv_freemp::assign_RP(CSE_Abstract* E, game_PlayerState* ps_who)
{
	Fvector pos, angle;
	float health;
	if (!ps_who->testFlag(GAME_PLAYER_MP_SAVE_LOADED) && load_position_RP_Binnar(ps_who, pos, angle, health))
	{
		//E->o_Position.set(pos);
		//E->o_Angle.set(angle);
		E->position().set(pos);
		E->angle().set(angle);
		//E->cast_actor_mp()->set_health(health);
	}
	else
		inherited::assign_RP(E, ps_who);
}