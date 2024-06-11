#include "StdAfx.h"
#include "server_progress_saver.h"
#include "CustomDetector.h"
#include "CustomOutfit.h"
#include "ActorHelmet.h"
#include "Weapon.h"



void CProgressSaver::LoadPlayersOnDeath(game_PlayerState* ps)
{
	if (MPlayersOnDeath.find(ps->GetStaticID()) != MPlayersOnDeath.end())
	{
		if (MPlayersOnDeath[ps->GetStaticID()].Outfit)
		{
			LPCSTR sect = MPlayersOnDeath[ps->GetStaticID()].OutfitName;
			float cond = MPlayersOnDeath[ps->GetStaticID()].OutfitCond;

			Msg("%s Load OnDeath Outfit: %s, cond: %f", ps->getName(), sect, cond);
			cond /= Random.randF(1.1, 2);

			CSE_Abstract* E = Level().Server->game->spawn_begin(sect);
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
			Level().Server->game->spawn_end(item, Level().Server->game->m_server->GetServerClient()->ID);
		}

		if (MPlayersOnDeath[ps->GetStaticID()].helm)
		{
			LPCSTR sect = MPlayersOnDeath[ps->GetStaticID()].HelmetName;
			float cond = MPlayersOnDeath[ps->GetStaticID()].HelmetCond;

			Msg("%s Load OnDeath Helmet: %s, cond: %f", ps->getName(), sect, cond);
			cond /= Random.randF(1.1, 2);

			CSE_Abstract* E = Level().Server->game->spawn_begin(sect);
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
			Level().Server->game->spawn_end(item, Level().Server->game->m_server->GetServerClient()->ID);
		}

		if (MPlayersOnDeath[ps->GetStaticID()].detector)
		{
			LPCSTR sect = MPlayersOnDeath[ps->GetStaticID()].DetectorName;
			float cond = MPlayersOnDeath[ps->GetStaticID()].DetectorCond;
			Msg("%s Load OnDeath Detector: %s", ps->getName(), sect);

			cond /= Random.randF(1.1, 2);

			CSE_Abstract* E = Level().Server->game->spawn_begin(sect);
			CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
			item->slot = MPlayersOnDeath[ps->GetStaticID()].DetectorSlot;
			if (item)
			{
				item->ID_Parent = ps->GameID;
				item->m_fCondition = cond;
			}
			Level().Server->game->spawn_end(item, Level().Server->game->m_server->GetServerClient()->ID);
		}

		if (MPlayersOnDeath[ps->GetStaticID()].weapon1)
		{
			LPCSTR sect = MPlayersOnDeath[ps->GetStaticID()].Weapon1Sect;
			float cond = MPlayersOnDeath[ps->GetStaticID()].Weapon1Cond;
			u8 AddonState = MPlayersOnDeath[ps->GetStaticID()].Weapon1AddonState;
			u8 CurrScope = MPlayersOnDeath[ps->GetStaticID()].Weapon1CurScope;
			Msg("%s Load OnDeath Weapon: %s, cond: %f", ps->getName(), sect, cond);


			cond /= Random.randF(1.1, 2);
			CSE_Abstract* E = Level().Server->game->spawn_begin(sect);
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
			Level().Server->game->spawn_end(E, Level().Server->game->m_server->GetServerClient()->ID);
		}

		if (MPlayersOnDeath[ps->GetStaticID()].weapon2)
		{
			LPCSTR sect = MPlayersOnDeath[ps->GetStaticID()].Weapon2Sect;
			float cond = MPlayersOnDeath[ps->GetStaticID()].Weapon2Cond;
			u8 AddonState = MPlayersOnDeath[ps->GetStaticID()].Weapon2AddonState;
			u8 CurrScope = MPlayersOnDeath[ps->GetStaticID()].Weapon2CurScope;
			Msg("%s Load OnDeath Weapon: %s, cond: %f", ps->getName(), sect, cond);


			cond /= Random.randF(1.1, 2);
			CSE_Abstract* E = Level().Server->game->spawn_begin(sect);
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
			Level().Server->game->spawn_end(E, fmp->m_server->GetServerClient()->ID);
		}
	}
}

void CProgressSaver::ClearPlayersOnDeathBuffer(u16 StaticID)
{
	if (MPlayersOnDeath.find(StaticID) != MPlayersOnDeath.end())
		MPlayersOnDeath.erase(StaticID);
}

void CProgressSaver::FillPlayerOnDisconnect(u16 StaticID, string_path path)
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