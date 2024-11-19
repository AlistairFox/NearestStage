#include "stdafx.h"
#include "server_progress_saver.h"

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
		CInventoryItem* item = smart_cast<CInventoryItem*>(Level().Objects.net_Find(id));
		SItem Sitem(item);

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

			u32 ItemType = reader->r_u32();

			if (ItemType == SItem::ItemTypes::WeaponAmmo)
			{
				CSE_ALifeItemAmmo* ammo = smart_cast<CSE_ALifeItemAmmo*>(item);
				ammo->a_elapsed = reader->r_u16();
			}

			if (ItemType == SItem::ItemTypes::Weapon)
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