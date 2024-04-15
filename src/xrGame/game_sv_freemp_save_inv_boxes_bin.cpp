#include "stdafx.h"
#include "game_sv_freemp.h"
#include "Level.h"
#include "Actor.h"
#include "Inventory.h"
#include "Weapon.h"
#include "xrServer_Objects_ALife.h"

void game_sv_freemp::FillInvBoxBuffer(CSE_ALifeInventoryBox* box)
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
		InvBoxItem Sitem;

		CInventoryItem* item = smart_cast<CInventoryItem*>(Level().Objects.net_Find(id));

		xr_strcpy(Sitem.item_sect, item->m_section_id.c_str());
		Sitem.item_cond = item->GetCondition();
		if (item->cast_weapon_ammo())
		{
			CWeaponAmmo* ammo = smart_cast<CWeaponAmmo*>(item);
			Sitem.weapon_ammo = true;
			Sitem.m_boxCurr = ammo->m_boxCurr;
		}
		else
			Sitem.weapon_ammo = false;

		if (item->cast_weapon())
		{
			Sitem.weapon = true;
			CWeapon* wpn = smart_cast<CWeapon*>(item);
			Sitem.ammoElapse = u16(wpn->GetAmmoElapsed());
			Sitem.ammoType = wpn->m_ammoType;
			Sitem.WeaponAddonState = wpn->GetAddonsState();
			Sitem.WeaponCurScope = wpn->m_cur_scope;
		}
		else
			Sitem.weapon = false;

		if (item->has_any_upgrades())
		{
			Sitem.has_upg = true;
			item->get_upgrades(Sitem.upgrades);
		}
		else
			Sitem.has_upg = false;

		OutBox->Items.push_back(Sitem);
	}

	csSaving.Enter();
	ThreadTasks.push_back({ OutBox, nullptr, nullptr });
	csSaving.Leave();
}

#ifdef OLD_BOX_SAVING
void game_sv_freemp::BinnarSaveInvBox(CSE_ALifeInventoryBox* box, string_path& filepath)
{
	IWriter* writer = FS.w_open(filepath);

	writer->open_chunk(INVBOX_ITEMS_CHUNK);

	writer->w_u16(box->children.size());

	for (auto id : box->children)
	{
		CInventoryItem* item = smart_cast<CInventoryItem*>(Level().Objects.net_Find(id));
		writer->w_stringZ(item->m_section_id.c_str());
		writer->w_float(item->GetCondition());
		if (item->cast_weapon_ammo())
		{
			CWeaponAmmo* ammo = smart_cast<CWeaponAmmo*>(item);
			writer->w_u8(1);
			writer->w_u16(ammo->m_boxCurr);
		}
		else
			writer->w_u8(0);

		if (item->cast_weapon())
		{
			writer->w_u8(1);
			CWeapon* wpn = smart_cast<CWeapon*>(item);
			writer->w_u16(u16(wpn->GetAmmoElapsed()));
			writer->w_u8(wpn->m_ammoType);
			writer->w_u8(wpn->GetAddonsState());
			writer->w_u8(wpn->m_cur_scope);
		}
		else
			writer->w_u8(0);

		if (item->has_any_upgrades())
		{
			writer->w_u8(1);
			string2048 upgrades;
			item->get_upgrades(upgrades);
			writer->w_stringZ(upgrades);
		}
		else
			writer->w_u8(0);

	}
	writer->close_chunk();
	FS.w_close(writer);
}
#endif // OLD_BOX_SAVING

void game_sv_freemp::BinnarLoadInvBox(CSE_ALifeInventoryBox* box)
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

	Msg("%s", filepath);

	IReader* reader = FS.r_open(filepath);
	if (reader->open_chunk(INVBOX_ITEMS_CHUNK))
	{
		u16 item_count = reader->r_u16();

		for (u16 id = 0; id != item_count; id++)
		{
			shared_str name;
			reader->r_stringZ(name);
			CSE_Abstract* E = spawn_begin(name.c_str());

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

			spawn_end(E, m_server->GetServerClient()->ID);
		}
	}
	FS.r_close(reader);

}