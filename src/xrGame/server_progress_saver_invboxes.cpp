#include "stdafx.h"
#include "server_progress_saver.h"
#include "Weapon.h"

void CProgressSaver::FillInvBoxBuffer(CSE_ALifeInventoryBox* box)
{
	InvBox* OutBox = xr_new<InvBox>();

	string_path path_name;
	string64 invbox_name;
	xr_strcpy(invbox_name, box->name_replace());
	xr_strcat(invbox_name, ".binsave");
	FS.update_path(path_name, "$mp_saves_invbox_bin$", invbox_name);
	xr_strcpy(OutBox->box_path, path_name);

	for (const auto id : box->children)
	{
		SItem Sitem;

		CInventoryItem* item = smart_cast<CInventoryItem*>(Level().Objects.net_Find(id));

		xr_strcpy(Sitem.ItemSect, item->m_section_id.c_str());
		Sitem.ItemCond = item->GetCondition();
		if (item->cast_weapon_ammo())
		{
			CWeaponAmmo* ammo = smart_cast<CWeaponAmmo*>(item);
			Sitem.IsWeaponAmmo = true;
			Sitem.AmmoBoxCurr = ammo->m_boxCurr;
		}
		else
			Sitem.IsWeaponAmmo = false;

		if (item->cast_weapon())
		{
			Sitem.IsWeapon = true;
			CWeapon* wpn = smart_cast<CWeapon*>(item);
			Sitem.AmmoElapsed = u16(wpn->GetAmmoElapsed());
			Sitem.AmmoType = wpn->m_ammoType;
			Sitem.AddonState = wpn->GetAddonsState();
			Sitem.CurrScope = wpn->m_cur_scope;
		}
		else
			Sitem.IsWeapon = false;

		if (item->has_any_upgrades())
		{
			Sitem.HasUpgr = true;
			item->get_upgrades(Sitem.Uphrades);
		}
		else
			Sitem.HasUpgr = false;

		OutBox->Items.push_back(Sitem);
	}

	csSaving.Enter();
	ThreadTasks.push_back({ OutBox, nullptr, nullptr, nullptr });
	csSaving.Leave();
}

void CProgressSaver::BinnarLoadInvBox(CSE_ALifeInventoryBox* box)
{

	string_path filepath;
	string64 invbox_name;
	if (!box)
		return;

	xr_strcpy(invbox_name, box->name_replace());
	xr_strcat(invbox_name, ".binsave");
	FS.update_path(filepath, "$mp_saves_invbox_bin$", invbox_name);

	if (!FS.exist(filepath))
		return;

	if (af_debug_loggining)
		Msg("%s", filepath);

	IReader* reader = FS.r_open(filepath);
	if (reader->open_chunk(INVBOX_ITEMS_CHUNK))
	{
		u16 item_count = reader->r_u16();

		for (u16 id = 0; id != item_count; id++)
		{
			shared_str name;
			reader->r_stringZ(name);
			CSE_Abstract* E = Level().Server->game->spawn_begin(name.c_str());

			E->ID_Parent = box->ID;
			CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
			item->m_fCondition = reader->r_float();

			bool isWeaponAmmo = reader->r_u8();

			if (isWeaponAmmo)
			{
				CSE_ALifeItemAmmo* ammo = smart_cast<CSE_ALifeItemAmmo*>(item);
				ammo->a_elapsed = reader->r_u16();
			}

			bool isWeapon = reader->r_u8();

			if (isWeapon)
			{
				CSE_ALifeItemWeapon* wpn = smart_cast<CSE_ALifeItemWeapon*>(item);
				wpn->a_elapsed = reader->r_u16();
				wpn->ammo_type = reader->r_u8();
				wpn->m_addon_flags.flags = reader->r_u8();
				wpn->m_cur_scope = reader->r_u8();
			}

			bool HasUpgrades = reader->r_u8();

			if (HasUpgrades)
			{
				shared_str upgrades;
				reader->r_stringZ(upgrades);

				u32 count = _GetItemCount(upgrades.c_str(), ',');

				for (u32 id = 0; id != count; id++)
				{
					string64 upgrade;
					_GetItem(upgrades.c_str(), id, upgrade, ',');
					item->m_upgrades.push_back(upgrade);
				}
			}
			Level().Server->game->spawn_end(E, Level().Server->GetServerClient()->ID);
		}
	}
	FS.r_close(reader);

}