#include "stdafx.h"
#include "game_sv_freemp.h"
#include "Level.h"
#include "Actor.h"
#include "Inventory.h"
#include "Weapon.h"
#include "xrServer_Objects_ALife.h"

void game_sv_freemp::SaveInvBox(CSE_ALifeInventoryBox* box, CInifile* file)
{
	u32 ids = 0;
	for (auto id : box->children)
	{
		ids += 1;
		string32 itemID;
		xr_sprintf(itemID, "item_%d", ids);

		CInventoryItem* item = smart_cast<CInventoryItem*>(Level().Objects.net_Find(id));

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