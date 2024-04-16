#include "StdAfx.h"
#include "game_sv_freemp.h"
#include "Level.h"
#include "CustomDetector.h"
#include "CustomOutfit.h"
#include "Actor.h"
#include "Inventory.h"
#include "ActorHelmet.h"
#include "Weapon.h"


#ifdef OLD_BOX_SAVING
void game_sv_freemp::SavePlayersOnDeath(game_PlayerState* ps)
{
	CObject* obj = Level().Objects.net_Find(ps->GameID);
	CActor* actor = smart_cast<CActor*>(obj);
	if (!actor)
		return;

	if (!actor->g_Alive())
		return;

	if (smart_cast<CInventoryOwner*>(obj))
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
	}
}
#endif // OLD_BOX_SAVING



void game_sv_freemp::LoadPlayersOnDeath(game_PlayerState* ps)
{
	if (MPlayersOnDeath.find(ps->GetStaticID()) != MPlayersOnDeath.end())
	{
		if (MPlayersOnDeath[ps->GetStaticID()].Outfit)
		{
			LPCSTR sect = MPlayersOnDeath[ps->GetStaticID()].OutfitName;
			float cond = MPlayersOnDeath[ps->GetStaticID()].OutfitCond;

			Msg("%s Load OnDeath Outfit: %s, cond: %f", ps->getName(), sect, cond);
			cond /= Random.randF(1.1, 2);

			CSE_Abstract* E = spawn_begin(sect);
			CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
			if (item)
			{
				item->m_fCondition = cond;
				item->ID_Parent = ps->GameID;
				item->slot = MPlayersOnDeath[ps->GetStaticID()].OutfitSlot;

				if (MPlayersOnDeath[ps->GetStaticID()].OutfUpg)
				{

					const u32 itemscount = _GetItemCount(MPlayersOnDeath[ps->GetStaticID()].OutfitUpgrades);

					for (u32 id = 0; id != itemscount; id++)
					{
						string64 upgrade;
						_GetItem(MPlayersOnDeath[ps->GetStaticID()].OutfitUpgrades, id, upgrade, ',');
						item->add_upgrade(upgrade);
					}
				}
			}
			spawn_end(item, m_server->GetServerClient()->ID);
		}

		if (MPlayersOnDeath[ps->GetStaticID()].helm)
		{
			LPCSTR sect = MPlayersOnDeath[ps->GetStaticID()].HelmetName;
			float cond = MPlayersOnDeath[ps->GetStaticID()].HelmetCond;

			Msg("%s Load OnDeath Helmet: %s, cond: %f", ps->getName(), sect, cond);
			cond /= Random.randF(1.1, 2);

			CSE_Abstract* E = spawn_begin(sect);
			CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
			if (item)
			{
				item->m_fCondition = cond;
				item->ID_Parent = ps->GameID;
				item->slot = MPlayersOnDeath[ps->GetStaticID()].HelmSlot;

				if (MPlayersOnDeath[ps->GetStaticID()].HelmUpg)
				{
					const u32 itemscount = _GetItemCount(MPlayersOnDeath[ps->GetStaticID()].HelmetUpgrades);

					for (u32 id = 0; id != itemscount; id++)
					{
						string64 upgrade;
						_GetItem(MPlayersOnDeath[ps->GetStaticID()].HelmetUpgrades, id, upgrade, ',');
						item->add_upgrade(upgrade);
					}
				}
			}
			spawn_end(item, m_server->GetServerClient()->ID);
		}

		if (MPlayersOnDeath[ps->GetStaticID()].detector)
		{
			LPCSTR sect = MPlayersOnDeath[ps->GetStaticID()].DetectorName;
			float cond = MPlayersOnDeath[ps->GetStaticID()].DetectorCond;
			Msg("%s Load OnDeath Detector: %s", ps->getName(), sect);

			cond /= Random.randF(1.1, 2);

			CSE_Abstract* E = spawn_begin(sect);
			CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
			item->slot = MPlayersOnDeath[ps->GetStaticID()].DetectorSlot;
			if (item)
			{
				item->ID_Parent = ps->GameID;
				item->m_fCondition = cond;
			}
			spawn_end(item, m_server->GetServerClient()->ID);
		}

		if (MPlayersOnDeath[ps->GetStaticID()].weapon1)
		{
			LPCSTR sect = MPlayersOnDeath[ps->GetStaticID()].Weapon1Sect;
			float cond = MPlayersOnDeath[ps->GetStaticID()].Weapon1Cond;
			u8 AddonState = MPlayersOnDeath[ps->GetStaticID()].Weapon1AddonState;
			u8 CurrScope = MPlayersOnDeath[ps->GetStaticID()].Weapon1CurScope;
			Msg("%s Load OnDeath Weapon: %s, cond: %f", ps->getName(), sect, cond);


			cond /= Random.randF(1.1, 2);
			CSE_Abstract* E = spawn_begin(sect);
			E->ID_Parent = ps->GameID;
			CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
			item->m_fCondition = cond;
			item->slot = MPlayersOnDeath[ps->GetStaticID()].Weapon1Slot;
			CSE_ALifeItemWeapon* wpn = smart_cast<CSE_ALifeItemWeapon*>(item);
			wpn->m_addon_flags.flags = AddonState;
			wpn->m_cur_scope = CurrScope;


			if (MPlayersOnDeath[ps->GetStaticID()].weapon1Upgr)
			{
				const u32 upgrCount = _GetItemCount(MPlayersOnDeath[ps->GetStaticID()].Weapon1Upgrades, ',');

				for (u32 id = 0; id != upgrCount; id++)
				{
					string64 upgrade;
					_GetItem(MPlayersOnDeath[ps->GetStaticID()].Weapon1Upgrades, id, upgrade, ',');
					item->add_upgrade(upgrade);
				}
			}
			spawn_end(E, m_server->GetServerClient()->ID);
		}

		if (MPlayersOnDeath[ps->GetStaticID()].weapon2)
		{
			LPCSTR sect = MPlayersOnDeath[ps->GetStaticID()].Weapon2Sect;
			float cond = MPlayersOnDeath[ps->GetStaticID()].Weapon2Cond;
			u8 AddonState = MPlayersOnDeath[ps->GetStaticID()].Weapon2AddonState;
			u8 CurrScope = MPlayersOnDeath[ps->GetStaticID()].Weapon2CurScope;
			Msg("%s Load OnDeath Weapon: %s, cond: %f", ps->getName(), sect, cond);


			cond /= Random.randF(1.1, 2);
			CSE_Abstract* E = spawn_begin(sect);
			E->ID_Parent = ps->GameID;
			CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
			item->m_fCondition = cond;
			item->slot = MPlayersOnDeath[ps->GetStaticID()].Weapon2Slot;
			CSE_ALifeItemWeapon* wpn = smart_cast<CSE_ALifeItemWeapon*>(item);
			wpn->m_addon_flags.flags = AddonState;
			wpn->m_cur_scope = CurrScope;

			if (MPlayersOnDeath[ps->GetStaticID()].weapon2Upgr)
			{
				const u32 upgrCount = _GetItemCount(MPlayersOnDeath[ps->GetStaticID()].Weapon2Upgrades, ',');

				for (u32 id = 0; id != upgrCount; id++)
				{
					string64 upgrade;
					_GetItem(MPlayersOnDeath[ps->GetStaticID()].Weapon2Upgrades, id, upgrade, ',');
					item->add_upgrade(upgrade);
				}
			}
			spawn_end(E, m_server->GetServerClient()->ID);
		}
	}
}

void game_sv_freemp::ClearPlayersOnDeathBuffer(u16 StaticID)
{
	if (MPlayersOnDeath.find(StaticID) != MPlayersOnDeath.end())
		MPlayersOnDeath.erase(StaticID);
}

void game_sv_freemp::FillPlayerOnDisconnect(u16 StaticID, string_path path)
{
	if (MPlayersOnDeath.find(StaticID) == MPlayersOnDeath.end())
		return;

	OnDeathDisconnect* dis = xr_new<OnDeathDisconnect>();

	xr_strcpy(dis->PlayerPath, path);
	dis->Items = MPlayersOnDeath[StaticID];

	csSaving.Enter();
	ThreadTasks.push_back({ nullptr, nullptr, dis, nullptr });
	csSaving.Leave();
}

#ifdef OLD_BOX_SAVING
void game_sv_freemp::SavePlayerOnDisconnect(u16 StaticID, string_path path)
{
	Msg("-- Create save file by player: %d ", StaticID);
	if (MPlayersOnDeath.find(StaticID) == MPlayersOnDeath.end())
		return;

	IWriter* writer = FS.w_open(path);

	{
		writer->open_chunk(ACTOR_MONEY);
		writer->w_u32(MPlayersOnDeath[StaticID].PlayerMoney);
		writer->close_chunk();
	}

	{
		writer->open_chunk(ACTOR_TEAM);
		writer->w_u8(MPlayersOnDeath[StaticID].Team);
		writer->close_chunk();
	}

	{
		writer->open_chunk(ACTOR_POS);
		writer->w_u8(0);
		writer->close_chunk();
	}

	{
		writer->open_chunk(ACTOR_DEVICES_CHUNK);
		writer->w_u8(0);
		if (MPlayersOnDeath[StaticID].detector)
		{
			writer->w_u8(1);
			writer->w_stringZ(MPlayersOnDeath[StaticID].DetectorName);
			writer->w_float(MPlayersOnDeath[StaticID].DetectorCond);
		}
		else
			writer->w_u8(0);

		writer->w_u8(0);
		writer->w_u8(0);
		writer->close_chunk();
	}

	{
		writer->open_chunk(ACTOR_INV_ITEMS_CHUNK);
		u32 i = 0;
		if (MPlayersOnDeath[StaticID].Outfit)
			i++;
		if (MPlayersOnDeath[StaticID].helm)
			i++;
		if (MPlayersOnDeath[StaticID].weapon1)
			i++;
		if (MPlayersOnDeath[StaticID].weapon2)
			i++;
		if (MPlayersOnDeath[StaticID].detector)
			i++;

		writer->w_u32(i);
		if (MPlayersOnDeath[StaticID].Outfit)
		{
			writer->w_stringZ(MPlayersOnDeath[StaticID].OutfitName);
			writer->w_u16(MPlayersOnDeath[StaticID].OutfitSlot);
			writer->w_float(MPlayersOnDeath[StaticID].OutfitCond);
			writer->w_u8(0);
			writer->w_u8(0);
			if (MPlayersOnDeath[StaticID].OutfUpg)
			{
				writer->w_u8(1);
				writer->w_stringZ(MPlayersOnDeath[StaticID].OutfitUpgrades);
			}
			else
				writer->w_u8(0);
		}
		if (MPlayersOnDeath[StaticID].helm)
		{
			writer->w_stringZ(MPlayersOnDeath[StaticID].HelmetName);
			writer->w_u16(MPlayersOnDeath[StaticID].HelmSlot);
			writer->w_float(MPlayersOnDeath[StaticID].HelmetCond);
			writer->w_u8(0);
			writer->w_u8(0);
			if (MPlayersOnDeath[StaticID].HelmUpg)
			{
				writer->w_u8(1);
				writer->w_stringZ(MPlayersOnDeath[StaticID].HelmetUpgrades);
			}
			else
				writer->w_u8(0);
		}

		if (MPlayersOnDeath[StaticID].weapon1)
		{
			writer->w_stringZ(MPlayersOnDeath[StaticID].Weapon1Sect);
			writer->w_u16(MPlayersOnDeath[StaticID].Weapon1Slot);
			writer->w_float(MPlayersOnDeath[StaticID].Weapon1Cond);
			writer->w_u8(0);
			writer->w_u8(1);
			writer->w_u16(0);
			writer->w_u8(0);
			writer->w_u8(MPlayersOnDeath[StaticID].Weapon1AddonState);
			writer->w_u8(MPlayersOnDeath[StaticID].Weapon1CurScope);
			if (MPlayersOnDeath[StaticID].weapon1Upgr)
			{
				writer->w_u8(1);
				writer->w_stringZ(MPlayersOnDeath[StaticID].Weapon1Upgrades);
			}
			else
				writer->w_u8(0);
		}

		if (MPlayersOnDeath[StaticID].weapon2)
		{
			writer->w_stringZ(MPlayersOnDeath[StaticID].Weapon2Sect);
			writer->w_u16(MPlayersOnDeath[StaticID].Weapon2Slot);
			writer->w_float(MPlayersOnDeath[StaticID].Weapon2Cond);
			writer->w_u8(0);
			writer->w_u8(1);
			writer->w_u16(0);
			writer->w_u8(0);
			writer->w_u8(MPlayersOnDeath[StaticID].Weapon2AddonState);
			writer->w_u8(MPlayersOnDeath[StaticID].Weapon2CurScope);
			if (MPlayersOnDeath[StaticID].weapon2Upgr)
			{
				writer->w_u8(1);
				writer->w_stringZ(MPlayersOnDeath[StaticID].Weapon2Upgrades);
			}
			else
				writer->w_u8(0);
		}

		writer->close_chunk();
		FS.w_close(writer);
	}
}
#endif