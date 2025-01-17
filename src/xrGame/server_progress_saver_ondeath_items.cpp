#include "StdAfx.h"
#include "server_progress_saver.h"



void CProgressSaver::LoadPlayersOnDeath(game_PlayerState* ps)
{
#ifdef	PLAYERONDEATH_SAVING
	if (MPlayersOnDeath.find(ps->GetStaticID()) != MPlayersOnDeath.end())
	{
		for (auto& TItem : MPlayersOnDeath[ps->GetStaticID()].OnDeathItems)
		{
			LPCSTR sect = TItem.ItemSect;
			float cond = TItem.ItemCond;

			Msg("AFPROGRESSAVER: %s Load OnDeath Outfit: %s, cond: %f", ps->getName(), sect, cond);
			cond /= Random.randF(1.1, 2);

			CSE_Abstract* E = Level().Server->game->spawn_begin(sect);
			CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
			if (item)
			{
				item->m_fCondition = cond;
				item->ID_Parent = ps->GameID;
				item->slot = TItem.ItemSlot;
				if (TItem.ItemType == SItem::ItemTypes::Weapon)
				{
					u8 AddonState = TItem.AddonState;
					u8 CurrScope = TItem.CurrScope;
					CSE_ALifeItemWeapon* wpn = smart_cast<CSE_ALifeItemWeapon*>(item);
					wpn->m_addon_flags.flags = AddonState;
					wpn->m_cur_scope = CurrScope;
				}


				if (TItem.HasUpgr)
				{

					const u32 itemscount = _GetItemCount(TItem.Uphrades);

					for (u32 id = 0; id != itemscount; id++)
					{
						string64 upgrade;
						_GetItem(TItem.Uphrades, id, upgrade, ',');
						item->add_upgrade(upgrade);
					}
				}
			}
			Level().Server->game->spawn_end(item, Level().Server->game->m_server->GetServerClient()->ID);
		}
	}
#endif
}

void CProgressSaver::ClearPlayersOnDeathBuffer(u16 StaticID)
{
#ifdef	PLAYERONDEATH_SAVING
	if (MPlayersOnDeath.find(StaticID) != MPlayersOnDeath.end())
		MPlayersOnDeath.erase(StaticID);
#endif
}

void CProgressSaver::FillPlayerOnDisconnect(u16 StaticID, string_path path, LPSTR Name)
{
#ifdef	PLAYERONDEATH_SAVING
	if (MPlayersOnDeath.find(StaticID) == MPlayersOnDeath.end())
		return;

	OnDeathDisconnect* dis = xr_new<OnDeathDisconnect>();

	xr_strcpy(dis->PlayerPath, path);
	xr_strcpy(dis->PlayerName, Name);
	dis->Items = MPlayersOnDeath[StaticID];

	csSaving.Enter();
	ThreadTasks.push_back(SThreadTask(dis));
	csSaving.Leave();
#endif
}