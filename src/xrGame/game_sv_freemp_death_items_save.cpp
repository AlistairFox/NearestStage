#include "StdAfx.h"
#include "game_sv_freemp.h"
#include "Level.h"
#include "CustomDetector.h"
#include "CustomOutfit.h"
#include "Actor.h"
#include "Inventory.h"
#include "ActorHelmet.h"
#include "Weapon.h"

void game_sv_freemp::SavePlayersOnDeath(game_PlayerState* ps)
{
	CObject* obj = Level().Objects.net_Find(ps->GameID);
	CActor* actor = smart_cast<CActor*>(obj);
	if (!actor)
		return;

	if (smart_cast<CInventoryOwner*>(obj))
	{
		if (MPlayersOnDeath.find(ps->getName()) != MPlayersOnDeath.end())
			MPlayersOnDeath.erase(ps->getName());

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

		MPlayersOnDeath[ps->getName()] = buff;
	}
}

void game_sv_freemp::LoadPlayersOnDeath(game_PlayerState* ps)
{
	if (MPlayersOnDeath.find(ps->getName()) != MPlayersOnDeath.end())
	{
		if (MPlayersOnDeath[ps->getName()].Outfit)
		{
			LPCSTR sect = MPlayersOnDeath[ps->getName()].OutfitName;
			float cond = MPlayersOnDeath[ps->getName()].OutfitCond;

			Msg("%s Load OnDeath Outfit: %s, cond: %f", ps->getName(), sect, cond);
			cond /= Random.randF(1.1, 2);

			CSE_Abstract* E = spawn_begin(sect);
			CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
			if (item)
			{
				item->m_fCondition = cond;
				item->ID_Parent = ps->GameID;
				item->slot = MPlayersOnDeath[ps->getName()].OutfitSlot;

				if (MPlayersOnDeath[ps->getName()].OutfUpg)
				{

					const u32 itemscount = _GetItemCount(MPlayersOnDeath[ps->getName()].OutfitUpgrades);

					for (u32 id = 0; id != itemscount; id++)
					{
						string64 upgrade;
						_GetItem(MPlayersOnDeath[ps->getName()].OutfitUpgrades, id, upgrade, ',');
						item->add_upgrade(upgrade);
					}
				}
			}
			spawn_end(item, m_server->GetServerClient()->ID);
		}

		if (MPlayersOnDeath[ps->getName()].helm)
		{
			LPCSTR sect = MPlayersOnDeath[ps->getName()].HelmetName;
			float cond = MPlayersOnDeath[ps->getName()].HelmetCond;

			Msg("%s Load OnDeath Helmet: %s, cond: %f", ps->getName(), sect, cond);
			cond /= Random.randF(1.1, 2);

			CSE_Abstract* E = spawn_begin(sect);
			CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
			if (item)
			{
				item->m_fCondition = cond;
				item->ID_Parent = ps->GameID;
				item->slot = MPlayersOnDeath[ps->getName()].HelmSlot;

				if (MPlayersOnDeath[ps->getName()].HelmUpg)
				{
					const u32 itemscount = _GetItemCount(MPlayersOnDeath[ps->getName()].HelmetUpgrades);

					for (u32 id = 0; id != itemscount; id++)
					{
						string64 upgrade;
						_GetItem(MPlayersOnDeath[ps->getName()].HelmetUpgrades, id, upgrade, ',');
						item->add_upgrade(upgrade);
					}
				}
			}
			spawn_end(item, m_server->GetServerClient()->ID);
		}

		if (MPlayersOnDeath[ps->getName()].detector)
		{
			LPCSTR sect = MPlayersOnDeath[ps->getName()].DetectorName;
			float cond = MPlayersOnDeath[ps->getName()].DetectorCond;
			Msg("%s Load OnDeath Detector: %s", ps->getName(), sect);

			cond /= Random.randF(1.1, 2);

			CSE_Abstract* E = spawn_begin(sect);
			CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
			if (item)
			{
				item->ID_Parent = ps->GameID;
				item->m_fCondition = cond;
			}
			spawn_end(item, m_server->GetServerClient()->ID);
		}

		if (MPlayersOnDeath[ps->getName()].weapon1)
		{
			LPCSTR sect = MPlayersOnDeath[ps->getName()].Weapon1Sect;
			float cond = MPlayersOnDeath[ps->getName()].Weapon1Cond;
			u8 AddonState = MPlayersOnDeath[ps->getName()].Weapon1AddonState;
			u8 CurrScope = MPlayersOnDeath[ps->getName()].Weapon1CurScope;
			Msg("%s Load OnDeath Weapon: %s, cond: %f", ps->getName(), sect, cond);


			cond /= Random.randF(1.1, 2);
			CSE_Abstract* E = spawn_begin(sect);
			E->ID_Parent = ps->GameID;
			CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
			item->m_fCondition = cond;
			item->slot = MPlayersOnDeath[ps->getName()].Weapon1Slot;
			CSE_ALifeItemWeapon* wpn = smart_cast<CSE_ALifeItemWeapon*>(item);
			wpn->m_addon_flags.flags = AddonState;
			wpn->m_cur_scope = CurrScope;


			if (MPlayersOnDeath[ps->getName()].weapon1Upgr)
			{
				const u32 upgrCount = _GetItemCount(MPlayersOnDeath[ps->getName()].Weapon1Upgrades, ',');

				for (u32 id = 0; id != upgrCount; id++)
				{
					string64 upgrade;
					_GetItem(MPlayersOnDeath[ps->getName()].Weapon1Upgrades, id, upgrade, ',');
					item->add_upgrade(upgrade);
				}
			}
			spawn_end(E, m_server->GetServerClient()->ID);
		}

		if (MPlayersOnDeath[ps->getName()].weapon2)
		{
			LPCSTR sect = MPlayersOnDeath[ps->getName()].Weapon2Sect;
			float cond = MPlayersOnDeath[ps->getName()].Weapon2Cond;
			u8 AddonState = MPlayersOnDeath[ps->getName()].Weapon2AddonState;
			u8 CurrScope = MPlayersOnDeath[ps->getName()].Weapon2CurScope;
			Msg("%s Load OnDeath Weapon: %s, cond: %f", ps->getName(), sect, cond);


			cond /= Random.randF(1.1, 2);
			CSE_Abstract* E = spawn_begin(sect);
			E->ID_Parent = ps->GameID;
			CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
			item->m_fCondition = cond;
			item->slot = MPlayersOnDeath[ps->getName()].Weapon2Slot;
			CSE_ALifeItemWeapon* wpn = smart_cast<CSE_ALifeItemWeapon*>(item);
			wpn->m_addon_flags.flags = AddonState;
			wpn->m_cur_scope = CurrScope;

			if (MPlayersOnDeath[ps->getName()].weapon2Upgr)
			{
				const u32 upgrCount = _GetItemCount(MPlayersOnDeath[ps->getName()].Weapon2Upgrades, ',');

				for (u32 id = 0; id != upgrCount; id++)
				{
					string64 upgrade;
					_GetItem(MPlayersOnDeath[ps->getName()].Weapon2Upgrades, id, upgrade, ',');
					item->add_upgrade(upgrade);
				}
			}
			spawn_end(E, m_server->GetServerClient()->ID);
		}
	}
}

void game_sv_freemp::ClearPlayersOnDeathBuffer(LPSTR name)
{
	if (MPlayersOnDeath.find(name) != MPlayersOnDeath.end())
		MPlayersOnDeath.erase(name);
}

void game_sv_freemp::SavePlayerOnDisconnect(LPCSTR Name, string_path path)
{
	Msg("-- Create save file by player: %s ", Name);

	IWriter* writer = FS.w_open(path);

	{
		writer->open_chunk(ACTOR_MONEY);
		writer->w_u32(MPlayersOnDeath[Name].PlayerMoney);
		writer->close_chunk();
	}

	{
		writer->open_chunk(ACTOR_TEAM);
		writer->w_u8(MPlayersOnDeath[Name].Team);
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
		if (MPlayersOnDeath[Name].detector)
		{
			writer->w_u8(1);
			writer->w_stringZ(MPlayersOnDeath[Name].DetectorName);
			writer->w_float(MPlayersOnDeath[Name].DetectorCond);
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
		if (MPlayersOnDeath[Name].Outfit)
			i++;
		if (MPlayersOnDeath[Name].helm)
			i++;
		if (MPlayersOnDeath[Name].weapon1)
			i++;
		if (MPlayersOnDeath[Name].weapon2)
			i++;
		writer->w_u32(i);
		if (MPlayersOnDeath[Name].Outfit)
		{
			writer->w_stringZ(MPlayersOnDeath[Name].OutfitName);
			writer->w_u16(MPlayersOnDeath[Name].OutfitSlot);
			writer->w_float(MPlayersOnDeath[Name].OutfitCond);
			writer->w_u8(0);
			writer->w_u8(0);
			if (MPlayersOnDeath[Name].OutfUpg)
			{
				writer->w_u8(1);
				writer->w_stringZ(MPlayersOnDeath[Name].OutfitUpgrades);
			}
			else
				writer->w_u8(0);
		}
		if (MPlayersOnDeath[Name].helm)
		{
			writer->w_stringZ(MPlayersOnDeath[Name].HelmetName);
			writer->w_u16(MPlayersOnDeath[Name].HelmSlot);
			writer->w_float(MPlayersOnDeath[Name].HelmetCond);
			writer->w_u8(0);
			writer->w_u8(0);
			if (MPlayersOnDeath[Name].HelmUpg)
			{
				writer->w_u8(1);
				writer->w_stringZ(MPlayersOnDeath[Name].HelmetUpgrades);
			}
			else
				writer->w_u8(0);
		}

		if (MPlayersOnDeath[Name].weapon1)
		{
			writer->w_stringZ(MPlayersOnDeath[Name].Weapon1Sect);
			writer->w_u16(MPlayersOnDeath[Name].Weapon1Slot);
			writer->w_float(MPlayersOnDeath[Name].Weapon1Cond);
			writer->w_u8(0);
			writer->w_u8(1);
			writer->w_u16(0);
			writer->w_u8(0);
			writer->w_u8(MPlayersOnDeath[Name].Weapon1AddonState);
			writer->w_u8(MPlayersOnDeath[Name].Weapon1CurScope);
			if (MPlayersOnDeath[Name].weapon1Upgr)
			{
				writer->w_u8(1);
				writer->w_stringZ(MPlayersOnDeath[Name].Weapon1Upgrades);
			}
			else
				writer->w_u8(0);
		}

		if (MPlayersOnDeath[Name].weapon2)
		{
			writer->w_stringZ(MPlayersOnDeath[Name].Weapon2Sect);
			writer->w_u16(MPlayersOnDeath[Name].Weapon2Slot);
			writer->w_float(MPlayersOnDeath[Name].Weapon2Cond);
			writer->w_u8(0);
			writer->w_u8(1);
			writer->w_u16(0);
			writer->w_u8(0);
			writer->w_u8(MPlayersOnDeath[Name].Weapon2AddonState);
			writer->w_u8(MPlayersOnDeath[Name].Weapon2CurScope);
			if (MPlayersOnDeath[Name].weapon2Upgr)
			{
				writer->w_u8(1);
				writer->w_stringZ(MPlayersOnDeath[Name].Weapon2Upgrades);
			}
			else
				writer->w_u8(0);
		}

		writer->close_chunk();
		FS.w_close(writer);
	}
}