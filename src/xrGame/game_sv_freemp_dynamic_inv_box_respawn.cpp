#include "StdAfx.h"
#include "game_sv_freemp.h"
#include "alife_simulator.h"
#include "Level.h"

void game_sv_freemp::DynamicBoxUpdate()
{
	if (Level().game && Device.dwFrame % box_respawn_time == 0)
	{
		for (const auto &entity : inventory_boxes_cse)
		{
			CSE_Abstract* abs = entity.second.entity;
			CSE_ALifeInventoryBox* box = smart_cast<CSE_ALifeInventoryBox*>(abs);;

			if (box->children.empty())
			{
				SpawnInvBoxesItems(box);
			}
		}
	}
}

void game_sv_freemp::DynamicBoxFileCreate()
{
	string_path spawn_trash_path, spawn_boosters_path, spawn_weapons_devices_path,
		spawn_ammo_path, spawn_explosive_path, spawn_weapons_path;

	FS.update_path(spawn_trash_path, "$game_config$", "alife\\spawn_trash.ltx");
	FS.update_path(spawn_boosters_path, "$game_config$", "alife\\spawn_boosters.ltx");
	FS.update_path(spawn_weapons_devices_path, "$game_config$", "alife\\spawn_weapons_devices.ltx");
	FS.update_path(spawn_ammo_path, "$game_config$", "alife\\spawn_ammo.ltx");
	FS.update_path(spawn_explosive_path, "$game_config$", "alife\\spawn_explosive.ltx");
	FS.update_path(spawn_weapons_path, "$game_config$", "alife\\spawn_weapons.ltx");

	spawn_trash = xr_new<CInifile>(spawn_trash_path, true, true);
	spawn_boosters = xr_new<CInifile>(spawn_boosters_path, true, true);
	spawn_weapons_devices = xr_new<CInifile>(spawn_weapons_devices_path, true, true);
	spawn_ammo = xr_new<CInifile>(spawn_ammo_path, true, true);
	spawn_explosive = xr_new<CInifile>(spawn_explosive_path, true, true);
	spawn_weapons = xr_new<CInifile>(spawn_weapons_path, true, true);
}


void game_sv_freemp::SpawnInvBoxesItems(CSE_ALifeInventoryBox* box)
{
	LPCSTR boxfile = box->m_ini_string.c_str();
	CInifile					ini(
		&IReader(
			(void*)(boxfile),
			xr_strlen(boxfile)
		),
		FS.get_path("$game_config$")->m_Path
	);

	CInifile* tmp = NULL;

	if (ini.section_exist("spawn_trash"))
	{
		tmp = spawn_trash;
	}
	else if (ini.section_exist("spawn_boosters"))
	{
		tmp = spawn_boosters;
	}
	else if (ini.section_exist("spawn_weapons_devices"))
	{
		tmp = spawn_weapons_devices;
	}
	else if (ini.section_exist("spawn_ammo"))
	{
		tmp = spawn_ammo;
	}
	else if (ini.section_exist("spawn_explosive"))
	{
		tmp = spawn_explosive;
	}
	else if (ini.section_exist("spawn_weapons"))
	{
		tmp = spawn_weapons;
	}

	if (tmp != nullptr)
	{
		LPCSTR					N, V;
		float					p;

		if (tmp->section_exist("spawn"))
		{
			for (u32 k = 0, j; tmp->r_line("spawn", k, &N, &V); k++)
			{
				VERIFY(xr_strlen(N));

				float f_cond = 1.0f;
				bool bScope = false;
				bool bSilencer = false;
				bool bLauncher = false;


				int cur_scope = 0;

				j = 1;
				p = 1.f;

				if (V && xr_strlen(V)) {
					string64			buf;
					j = atoi(_GetItem(V, 0, buf));
					if (!j)		j = 1;

					bScope = (NULL != strstr(V, "scope"));
					bSilencer = (NULL != strstr(V, "silencer"));
					bLauncher = (NULL != strstr(V, "launcher"));
					//probability
					if (NULL != strstr(V, "prob="))
						p = (float)atof(strstr(V, "prob=") + 5);
					if (fis_zero(p)) p = 1.0f;
					if (NULL != strstr(V, "cond="))
						f_cond = (float)atof(strstr(V, "cond=") + 5);
					if (nullptr != strstr(V, "scope="))
						cur_scope = atoi(strstr(V, "scope=") + 6);
				}
				for (u32 i = 0; i < j; ++i) {
					if (Random.randF(1.f) < p) {
						//CSE_Abstract* E = alife().spawn_item(N, box->o_Position, box->m_tNodeID, box->m_tGraphID, box->ID);
						CSE_Abstract* E = spawn_begin(N);
						E->ID_Parent = box->ID;

						//подсоединить аддоны к оружию, если включены соответствующие флажки
						CSE_ALifeItemWeapon* W = smart_cast<CSE_ALifeItemWeapon*>(E);
						if (W) {
							if (W->m_scope_status == ALife::eAddonAttachable)
							{
								W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonScope, bScope);
								W->m_cur_scope = cur_scope;
							}
							if (W->m_silencer_status == ALife::eAddonAttachable)
								W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSilencer, bSilencer);
							if (W->m_grenade_launcher_status == ALife::eAddonAttachable)
								W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher, bLauncher);
						}
						CSE_ALifeInventoryItem* IItem = smart_cast<CSE_ALifeInventoryItem*>(E);
						if (IItem)
						{
							f_cond = Random.randF(0.0f, 0.6f);
							IItem->m_fCondition = f_cond;
						}
						spawn_end(E, m_server->GetServerClient()->ID);
					}
				}
			}
		}
	}
}

void game_sv_freemp::OnStartSpawnInvBoxesItems(CSE_ALifeInventoryBox* box)
{
	LPCSTR boxfile = box->m_ini_string.c_str();
	CInifile					ini(
		&IReader(
			(void*)(boxfile),
			xr_strlen(boxfile)
		),
		FS.get_path("$game_config$")->m_Path
	);

	CInifile* tmp = NULL;

	if (ini.section_exist("spawn_trash"))
	{
		tmp = spawn_trash;
	}
	else if (ini.section_exist("spawn_boosters"))
	{
		tmp = spawn_boosters;
	}
	else if (ini.section_exist("spawn_weapons_devices"))
	{
		tmp = spawn_weapons_devices;
	}
	else if (ini.section_exist("spawn_ammo"))
	{
		tmp = spawn_ammo;
	}
	else if (ini.section_exist("spawn_explosive"))
	{
		tmp = spawn_explosive;
	}
	else if (ini.section_exist("spawn_weapons"))
	{
		tmp = spawn_weapons;
	}

	if (tmp != nullptr)
	{
		LPCSTR					N, V;
		float					p;

		if (tmp->section_exist("spawn"))
		{
			for (u32 k = 0, j; tmp->r_line("spawn", k, &N, &V); k++)
			{
				VERIFY(xr_strlen(N));

				float f_cond = 1.0f;
				bool bScope = false;
				bool bSilencer = false;
				bool bLauncher = false;


				int cur_scope = 0;

				j = 1;
				p = 1.f;

				if (V && xr_strlen(V)) {
					string64			buf;
					j = atoi(_GetItem(V, 0, buf));
					if (!j)		j = 1;

					bScope = (NULL != strstr(V, "scope"));
					bSilencer = (NULL != strstr(V, "silencer"));
					bLauncher = (NULL != strstr(V, "launcher"));
					//probability
					if (NULL != strstr(V, "prob="))
						p = (float)atof(strstr(V, "prob=") + 5);
					if (fis_zero(p)) p = 1.0f;
					if (NULL != strstr(V, "cond="))
						f_cond = (float)atof(strstr(V, "cond=") + 5);
					if (nullptr != strstr(V, "scope="))
						cur_scope = atoi(strstr(V, "scope=") + 6);
				}
				for (u32 i = 0; i < j; ++i) {
					if (Random.randF(1.f) < p) {
						CSE_Abstract* E = alife().spawn_item(N, box->o_Position, box->m_tNodeID, box->m_tGraphID, box->ID);

						//подсоединить аддоны к оружию, если включены соответствующие флажки
						CSE_ALifeItemWeapon* W = smart_cast<CSE_ALifeItemWeapon*>(E);
						if (W) {
							if (W->m_scope_status == ALife::eAddonAttachable)
							{
								W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonScope, bScope);
								W->m_cur_scope = cur_scope;
							}
							if (W->m_silencer_status == ALife::eAddonAttachable)
								W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSilencer, bSilencer);
							if (W->m_grenade_launcher_status == ALife::eAddonAttachable)
								W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher, bLauncher);
						}
						CSE_ALifeInventoryItem* IItem = smart_cast<CSE_ALifeInventoryItem*>(E);
						if (IItem)
						{
							f_cond = Random.randF(0.0f, 0.6f);
							IItem->m_fCondition = f_cond;
						}
					}
				}
			}
		}
	}
}