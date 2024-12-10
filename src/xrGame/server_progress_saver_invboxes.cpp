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

#ifdef PRIVATE_INVBOX_SAVING
	CInventoryBox* PrivateBox = smart_cast<CInventoryBox*>(Level().Objects.net_Find(box->ID));

	if (PrivateBox->IsPrivateBox)
	{
		OutBox->BoxType = InvBox::BoxTypes::INVBOX_PRIVATE;
		for (const auto& BoxItemsList : PrivateBox->private_items)
		{
			CInventoryItem* item = smart_cast<CInventoryItem*>(Level().Objects.net_Find(BoxItemsList.first));
			SItem Sitem(item);
			PrivateBoxItem privateItem(Sitem, BoxItemsList.second);
			OutBox->PrivateItems.push_back(privateItem);
		}
	}
	else
#endif
	{
		OutBox->BoxType = InvBox::BoxTypes::INVBOX_PUBLIC;
		for (const auto id : box->children)
		{
			CInventoryItem* item = smart_cast<CInventoryItem*>(Level().Objects.net_Find(id));
			SItem Sitem(item);

			OutBox->Items.push_back(Sitem);
		}
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
		u8 BoxType = reader->r_u8();
		u16 item_count = reader->r_u32();
		for (u32 id = 0; id != item_count; id++)
		{
			u16 OwnerId = 0;
			if (BoxType == InvBox::BoxTypes::INVBOX_PRIVATE)
				OwnerId = reader->r_u16();

			shared_str name;
			reader->r_stringZ(name);

			if (!pSettings->section_exist(name))
				break;


			reader->r_u16();
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
			CSE_Abstract* FinalItem = fmp->spawn_end(E, fmp->m_server->GetServerClient()->ID);
#ifdef PRIVATE_INVBOX_SAVING
			if (BoxType == InvBox::BoxTypes::INVBOX_PRIVATE)
			{
				CInventoryBox* PrivateBox = smart_cast<CInventoryBox*>(Level().Objects.net_Find(box->ID));
				PrivateBox->private_items[FinalItem->ID] = OwnerId;
			}
#endif // PRIVATE_INVBOX_SAVING
		}
	}
	FS.r_close(reader);

}