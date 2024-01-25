#include "stdafx.h"
#include "game_sv_freemp.h"
#include "Level.h"
#include "alife_simulator.h"
#include "alife_spawn_registry.h"
#include "actor_mp_client.h"
#include "alife_time_manager.h"
#include "CustomOutfit.h"

BOOL g_SV_IsVipeMode = FALSE;
int g_sv_server_goodwill = 0;

game_sv_freemp::game_sv_freemp()
	:pure_relcase(&game_sv_freemp::net_Relcase)
{
	m_type = eGameIDFreeMp;

	if (g_SV_IsVipeMode) 
	{
		
	}

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

	string_path music_path;
	FS.update_path(music_path, "$game_config$", "alife\\music.ltx");
	Music = xr_new<CInifile>(music_path, true, true);
	MusicCount = Music->r_u8("music", "sound_count");
}

game_sv_freemp::~game_sv_freemp()
{
	
	xr_delete(spawn_trash);
	xr_delete(spawn_boosters);
	xr_delete(spawn_weapons_devices);
	xr_delete(spawn_ammo);
	xr_delete(spawn_explosive);
	xr_delete(spawn_weapons);
	xr_delete(Music);
	
}

void game_sv_freemp::Create(shared_str & options)
{
	inherited::Create(options);
	R_ASSERT2(rpoints[0].size(), "rpoints for players not found");

	//	if (strstr(*options, "/alife"))
//		m_alife_simulator = xr_new<CALifeSimulator>(&server(), &options);

	switch_Phase(GAME_PHASE_PENDING);

	::Random.seed(GetTickCount());
	m_CorpseList.clear();

	oldTime = 0;
}

void game_sv_freemp::OnAlifeCreate(CSE_Abstract* E)
{
	CSE_ALifeInventoryBox* box = smart_cast<CSE_ALifeInventoryBox*>(E);
	if (box)
	{
		OnStartSpawnInvBoxesItems(box);
		inventory_boxes data;
		data.entity = E;
		inventory_boxes_cse[E->ID] = data;
	}
}

void game_sv_freemp::OnObjectsCreate(CSE_Abstract* E)
{
	CSE_ALifeObjectPhysic* phy = smart_cast<CSE_ALifeObjectPhysic*>(E);
	if (phy)
	{
		first_play = false;
		need_stop_music = true;
		lenght = 0;
		Physics_objects data;
		data.entity = E;
		phy_objects_cse[E->ID] = data;
	}
}

// player connect #1
void game_sv_freemp::OnPlayerConnect(ClientID id_who)
{
	inherited::OnPlayerConnect(id_who);

	xrClientData* xrCData = m_server->ID_to_client(id_who);
	game_PlayerState*	ps_who = get_id(id_who);

	if (!xrCData->flags.bReconnect)
	{
		ps_who->clear();
		ps_who->team = 8;
		ps_who->skin = -1;
	};
	ps_who->setFlag(GAME_PLAYER_FLAG_SPECTATOR);

	ps_who->resetFlag(GAME_PLAYER_FLAG_SKIP);


	{
		string_path path_xray; // logins
		FS.update_path(path_xray, "$mp_saves_logins$", "logins.ltx"); // logins
		CInifile* file = xr_new<CInifile>(path_xray, true); // logins
		u8 level;

		if (file->section_exist(ps_who->getName()))
		{
			if (file->line_exist(ps_who->getName(), "Admin"))
			{
				level = file->r_u8(ps_who->getName(), "Admin");

				if (level == 1)
				{
					Msg("-- %s является администратором", ps_who->getName());
					xrCData->m_admin_rights.m_has_admin_rights = TRUE;
					xrCData->m_admin_rights.m_has_super_admin_rights = FALSE;
					xrCData->m_admin_rights.m_dwLoginTime = Device.dwTimeGlobal;
					if (xrCData->ps)
					{
						xrCData->ps->setFlag(GAME_PLAYER_HAS_ADMIN_RIGHTS);
						xrCData->ps->resetFlag(GAME_PLAYER_SUPER_ADMIN);
						m_server->game->signal_Syncronize();
					}
					NET_Packet			P_answ;
					P_answ.w_begin(M_REMOTE_CONTROL_AUTH);
					P_answ.w_stringZ("admin rights acces");
					m_server->SendTo(xrCData->ID, P_answ, net_flags(TRUE, TRUE));
				}
				else if (level == 2)
				{
					Msg("-- %s является супер администратором", ps_who->getName());
					xrCData->m_admin_rights.m_has_admin_rights = TRUE;
					xrCData->m_admin_rights.m_has_super_admin_rights = TRUE;
					xrCData->m_admin_rights.m_dwLoginTime = Device.dwTimeGlobal;
					if (xrCData->ps)
					{
						xrCData->ps->setFlag(GAME_PLAYER_HAS_ADMIN_RIGHTS);
						xrCData->ps->setFlag(GAME_PLAYER_SUPER_ADMIN);
						m_server->game->signal_Syncronize();
					}
					NET_Packet			P_answ;
					P_answ.w_begin(M_REMOTE_CONTROL_AUTH);
					P_answ.w_stringZ("admin rights acces");
					m_server->SendTo(xrCData->ID, P_answ, net_flags(TRUE, TRUE));
				}

			}
		}
		xr_delete(file);
	}

	if (g_dedicated_server && (xrCData == m_server->GetServerClient()))
	{
		ps_who->setFlag(GAME_PLAYER_FLAG_SKIP);
		return;
	}
}

// player connect #2
void game_sv_freemp::OnPlayerConnectFinished(ClientID id_who)
{
	xrClientData* xrCData = m_server->ID_to_client(id_who);
	SpawnPlayer(id_who, "spectator");

	if (xrCData)
	{
		R_ASSERT2(xrCData->ps, "Player state not created yet");
		NET_Packet					P;
		GenerateGameMessage(P);
		P.w_u32(GAME_EVENT_PLAYER_CONNECTED);
		P.w_clientID(id_who);
		xrCData->ps->team = 8;
		xrCData->ps->setFlag(GAME_PLAYER_FLAG_SPECTATOR);
		xrCData->ps->setFlag(GAME_PLAYER_FLAG_READY);
		
		if (!g_SV_IsVipeMode) 
		{
			if (HasSaveFile(xrCData->ps))
			{
				xrCData->ps->resetFlag(GAME_PLAYER_MP_SAVE_LOADED);
			}
			else
			{
				xrCData->ps->setFlag(GAME_PLAYER_MP_SAVE_LOADED);
			}
		}
		else
		{
			if (HasSaveFile(xrCData->ps))
			{
				xrCData->ps->team = 11;
				xrCData->ps->resetFlag(GAME_PLAYER_MP_SAVE_LOADED);
			}
			else
			{
				xrCData->ps->team = 11;
				xrCData->ps->setFlag(GAME_PLAYER_MP_SAVE_LOADED);
			}
		}

 		xrCData->ps->net_Export(P, TRUE);
		u_EventSend(P);
		xrCData->net_Ready = TRUE;
	};
}

void game_sv_freemp::ChangeGameTime(u32 day, u32 hour, u32 minute)
{
	if (ai().get_alife())
	{
		u32 value = day * 86400 + hour * 3600 + minute * 60;
		float fValue = static_cast<float> (value);
		value *= 1000;//msec
		g_pGamePersistent->Environment().ChangeGameTime(fValue);
		alife().time_manager().change_game_time(value);
	}
}

void game_sv_freemp::AddMoneyToPlayer(game_PlayerState * ps, s32 amount)
{
	if (!ps) return;

	Msg("- Add money to player: [%u]%s, %d amount", ps->GameID, ps->getName(), amount);

	s64 total_money = ps->money_for_round;
	total_money += amount;

	if (total_money < 0)
		total_money = 0;

	if (total_money > std::numeric_limits<s32>().max())
	{
		Msg("! The limit of the maximum amount of money has been exceeded.");
		total_money = std::numeric_limits<s32>().max() - 1;
	}

	ps->money_for_round = s32(total_money);
	signal_Syncronize();
}

void game_sv_freemp::SpawnItemToActor(u16 actorId, LPCSTR name)
{
	if (!name) return;

	CSE_Abstract *E = spawn_begin(name);
	E->ID_Parent = actorId;
	E->s_flags.assign(M_SPAWN_OBJECT_LOCAL);	// flags

	CSE_ALifeItemWeapon		*pWeapon = smart_cast<CSE_ALifeItemWeapon*>(E);
	if (pWeapon)
	{
		u16 ammo_magsize = pWeapon->get_ammo_magsize();
		pWeapon->a_elapsed = ammo_magsize;
	}

	CSE_ALifeItemPDA *pPda = smart_cast<CSE_ALifeItemPDA*>(E);
	if (pPda)
	{
		pPda->m_original_owner = actorId;
	}

	spawn_end(E, m_server->GetServerClient()->ID);
}

void game_sv_freemp::OnTransferMoney(NET_Packet & P, ClientID const & clientID)
{
	ClientID to;
	s32 money;

	P.r_clientID(to);
	P.r_s32(money);

	Msg("* Try to transfer money from %u to %u. Amount: %d", clientID.value(), to.value(), money);

	game_PlayerState* ps_from = get_id(clientID);

	if (!ps_from)
	{
		Msg("! Can't find player state with id=%u", clientID.value());
		return;
	}

	game_PlayerState* ps_to = get_id(to);
	if (!ps_to)
	{
		Msg("! Can't find player state with id=%u", to.value());
		return;
	}

	if (money <= 0 || ps_from->money_for_round < money)
		return;

	AddMoneyToPlayer(ps_from, -money);
	AddMoneyToPlayer(ps_to, money);
}

void game_sv_freemp::RespawnPlayer(ClientID id_who, bool NoSpectator)
{
	inherited::RespawnPlayer(id_who, NoSpectator);
	xrClientData* xrCData = (xrClientData*)m_server->ID_to_client(id_who);

 	game_PlayerState* ps = get_id(id_who);


	if (ps->testFlag(GAME_PLAYER_MP_SAVE_LOADED))
	{
		LoadPlayerOtfits(ps, nullptr);
		LoadPlayerDetectors(ps, nullptr);
	}

	string_path filepath;
	string_path login_path;


	FS.update_path(login_path, "$mp_saves_logins$", "logins.ltx"); // file with player logins

	FS.update_path(filepath, "$mp_saves_logins$", "checkstuf.ltx"); // file for check on first connect player or not


	CInifile* checkstuf_file = xr_new<CInifile>(filepath, false, true); // inifile for check on first connect player

	LPCSTR loginname = ps->getName();

	if (checkstuf_file && !checkstuf_file->line_exist(loginname, "staf_load"))
	{
		CInifile* login_file = xr_new<CInifile>(login_path, true, true);

		if (login_file->section_exist(loginname))
		{
			u8 kit_numb = login_file->r_u8(loginname, "kit_number");

			string_path spawn_config;
			FS.update_path(spawn_config, "$game_config$", "alife\\start_stuf.ltx");
			CInifile* spawn_file = xr_new<CInifile>(spawn_config, true, true);
			LPCSTR N, V;
			s32 money;

			LPCSTR spawn_section;
			if (kit_numb == 1)
				spawn_section = "spawn_kit_1";
			else if (kit_numb == 2)
				spawn_section = "spawn_kit_2";
			else if (kit_numb == 3)
				spawn_section = "spawn_kit_3";
			else if (kit_numb == 4)
				spawn_section = "spawn_kit_4";

			money = spawn_file->r_s32("start_money", "money");
			xrCData->ps->money_for_round = money;

			for (u32 k = 0; spawn_file->r_line(spawn_section, k, &N, &V); k++)
			{
				SpawnItemToActor(ps->GameID, N);
			}
			xr_delete(spawn_file);

			checkstuf_file->w_bool(loginname, "staf_load", true);
			checkstuf_file->save_as(filepath);
		}
		xr_delete(login_file);
	}
	xr_delete(checkstuf_file);

	if (ps)
	{
		ps->resetFlag(GAME_PLAYER_MP_SAFE_MODE);
		ps->resetFlag(GAME_PLAYER_MP_WOUND_MODE);
		ps->resetFlag(GAME_PLAYER_MP_LOOT_MODE);
	}

	if (Game().Type() == eGameIDFreeMp)

		if (ps->testFlag(GAME_PLAYER_MP_SAVE_LOADED))
		{
			xrCData->ps->money_for_round /= Random.randF(1.f, 1.2f);
		}

	if (ps && !ps->testFlag(GAME_PLAYER_MP_SAVE_LOADED))
	{
		SpawnItemToActor(ps->GameID, "wpn_binoc");
		if (binar_save)
		{
			string_path file_name_path;
			string32 file_name;
			xr_strcpy(file_name, ps->getName());
			xr_strcat(file_name, ".binsave");
			FS.update_path(file_name_path, "$mp_saves_players_bin$", file_name);
			if (FS.exist(file_name_path))
			{
				Msg("read file path = %s", file_name);
				BinnarLoadPlayer(ps, file_name_path);
			}
		}
		else
		{
			string_path file_name;
			string32 filename;
			xr_strcpy(filename, ps->getName());
			xr_strcat(filename, ".ltx");

			FS.update_path(file_name, "$mp_saves_players$", filename);

			Msg("read file path = %s", file_name);

			CInifile* file = xr_new<CInifile>(file_name, true);

			LoadPlayer(ps, file);
			xr_delete(file);
		}
				
 		ps->setFlag(GAME_PLAYER_MP_SAVE_LOADED);
 	}
}

void game_sv_freemp::OnPlayerReady(ClientID id_who)
{
	switch (Phase())
	{
		case GAME_PHASE_INPROGRESS:
		{
			xrClientData*	xrCData = (xrClientData*)m_server->ID_to_client(id_who);
			game_PlayerState*	ps = get_id(id_who);

			if (ps->IsSkip())
				break;

			if (!(ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)))
				break;

			RespawnPlayer(id_who, true);
		} break;

		default:
			break;
	};
}

// player disconnect
void game_sv_freemp::OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID)
{
	NET_Packet					P;
	GenerateGameMessage(P);
	P.w_u32(GAME_EVENT_PLAYER_DISCONNECTED);
	P.w_stringZ(Name);
	u_EventSend(P);
	//---------------------------------------------------

	auto it =
		std::find_if(save_outfits.begin(), save_outfits.end(), [&](outfits data)
			{
				if (strstr(data.player_name, Name))
					return true;
				else
					return false;
			});
	if (it != save_outfits.end())
		save_outfits.erase(it);

	auto name =
		std::find_if(save_detectors.begin(), save_detectors.end(), [&](detectors data)
			{

				if (strstr(data.player_name, Name))
					return true;
				else
					return false;
			});
	if (name != save_detectors.end())
		save_detectors.erase(name);
	

//	AllowDeadBodyRemove			(id_who, GameID);
	CObject* pObject = Level().Objects.net_Find(GameID);

	inherited::OnPlayerDisconnect(id_who, Name, GameID);

	CActorMP* pActor = smart_cast <CActorMP*>(pObject);
	if (pActor)
		pActor->DestroyObject();


}

void game_sv_freemp::OnPlayerKillPlayer(game_PlayerState * ps_killer, game_PlayerState * ps_killed, KILL_TYPE KillType, SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract * pWeaponA)
{

	if (ps_killed)
	{
		ps_killed->setFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD);
		ps_killed->DeathTime = Device.dwTimeGlobal;
	}
	signal_Syncronize();
}

void game_sv_freemp::OnEvent(NET_Packet &P, u16 type, u32 time, ClientID sender)
{
	switch (type)
	{
	case GAME_EVENT_PLAYER_KILL: // (g_kill)
		{
			u16 ID = P.r_u16();
			xrClientData *l_pC = (xrClientData*)get_client(ID);
			if (!l_pC) break;
			KillPlayer(l_pC->ID, l_pC->ps->GameID);
		}
		break;
	case GAME_EVENT_MP_TRADE:
		{
			OnPlayerTrade(P, sender);
		}
		break;
	case GAME_EVENT_TRANSFER_MONEY:
		{
			OnTransferMoney(P, sender);
		}
		break;
	default:
		inherited::OnEvent(P, type, time, sender);
	};
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

/// Dynamic Music
void game_sv_freemp::MusicPlay(CSE_ALifeObjectPhysic* obj,int pass, int obj_num)
{
	LPCSTR musicfile = obj->m_ini_string.c_str();
	CInifile ini(&IReader((void*)(musicfile), xr_strlen(musicfile)), FS.get_path("$game_config$")->m_Path);
	
	CInifile* tmp = NULL;
	shared_str music_name;
	string128 line_name;
	xr_sprintf(line_name, sizeof(line_name), "sound%d", pass);
	if (ini.section_exist("play_bar_snd"))
	{
		tmp = Music;
	}

	if (tmp != nullptr)
	{
		if (tmp->section_exist("music"))
		{
			music_name = tmp->r_string("music", line_name);

			Fvector pos = obj->Position();
			NET_Packet P;

			Msg("- Server: MusicStart: %s", music_name.c_str());
			Msg("- object pos: %f, %f, %f", pos.x, pos.y, pos.z);
			P.w_begin(M_MUSIC_UPDATE);
			P.w_stringZ(music_name);
			P.w_vec3(pos);
			P.w_u8(obj_num);
			server().SendBroadcast(BroadcastCID, P, net_flags(true, true));

			snd.create(music_name.c_str(), st_Music, sg_SourceType);
			snd.play(NULL, sm_2D);
			lenght = Device.dwTimeGlobal + (snd.get_length_sec() * 1000) + 10000;
			Msg("lenght: %f", snd.get_length_sec());
			snd.destroy();
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

#include "Actor.h"
#include <ui/UIInventoryUtilities.h>
extern int save_time;
extern int save_time2;
extern int save_time3;
extern int box_respawn_time;
extern BOOL		set_next_music;
void game_sv_freemp::Update()
{
	inherited::Update();

	if (Phase() != GAME_PHASE_INPROGRESS)
	{
		OnRoundStart();
	}

	if (!g_pGameLevel)
		return;

	if (Level().game && Device.dwFrame % save_time == 0)
	{
		for (auto player : Level().game->players)
		{
			if (player.second->testFlag(GAME_PLAYER_MP_SAVE_LOADED))
			{
				if (player.first == server().GetServerClient()->ID)
					continue;

				CObject* obj = Level().Objects.net_Find(player.second->GameID);
				CActor* actor = smart_cast<CActor*>(obj);
				if (!actor)
					return;
				if (!actor->g_Alive())
					return;


				if (binar_save)
				{
					string_path file_name_path;
					string32 file_name;
					xr_strcpy(file_name, player.second->getName());
					xr_strcat(file_name, ".binsave");
					FS.update_path(file_name_path, "$mp_saves_players_bin$", file_name);
					BinnarSavePlayer(player.second, file_name_path);
				}
				else
				{
					string_path file_name;
					string32 filename;
					xr_strcpy(filename, player.second->getName());
					xr_strcat(filename, ".ltx");

					FS.update_path(file_name, "$mp_saves_players$", filename);
					CInifile* file = xr_new<CInifile>(file_name, false, false);
					SavePlayer(player.second, file);
					file->save_as(file_name);
					xr_delete(file);
				}
				SavePlayerOutfits(player.second, nullptr);
				SavePlayerDetectors(player.second, nullptr);
			}
		}
	}

	///////////////Server environment saving//////////////////////
	if (Level().game && Device.dwFrame % save_time3 == 0)
	{
		string_path save_game_time;
		FS.update_path(save_game_time, "$global_server_data$", "server_data.ltx");
		CInifile* global_server_data = xr_new<CInifile>(save_game_time, false, false);

		LPCSTR time = InventoryUtilities::GetGameTimeAsString(InventoryUtilities::etpTimeToSeconds).c_str();
		LPCSTR data = InventoryUtilities::GetDateAsString(GetGameTime(), InventoryUtilities::edpDateToNormal).c_str();
		global_server_data->w_string("server_env", "time", time);
		global_server_data->w_string("server_env", "data", data);
		global_server_data->w_string("server_env", "weather", g_pGamePersistent->Environment().CurrentWeatherName.c_str());
		global_server_data->save_as(save_game_time);
		xr_delete(global_server_data);
	}
	///////////////Server environment saving//////////////////////


	/// calculate dynamic weather
	if (Level().game)
	{
		shared_str envtime = InventoryUtilities::GetGameTimeAsString(InventoryUtilities::etpTimeToMinutes);

		if (!need_change_weather)
		{
			if (xr_strcmp(envtime, "01:00") == 0)
			{
				bool	changeweather = false;
				float random = 0.0f;
				need_change_weather = true;
				random += Random.randF(0.f, 1.f);
				shared_str def;
				shared_str curr_weather = g_pGamePersistent->Environment().CurrentWeatherName;

				while (!changeweather)
				{
					if (random <= 0.25f && (xr_strcmp(curr_weather, "[main_cycle_multy]") == 0))
					{
						Msg("! !ERROR: overlap weather [main_cycle_multy]!");
						random = Random.randF(0.f, 1.f);
					}
					if (random <= 0.50f && (xr_strcmp(curr_weather, "[main_cycle_foggy_day]") == 0))
					{
						Msg("! !ERROR: overlap weather [main_cycle_foggy_day]!");
						random = Random.randF(0.f, 1.f);
					}
					if (random <= 0.75f && (xr_strcmp(curr_weather, "[main_cycle_sunny_day]") == 0))
					{
						Msg("! !ERROR: overlap weather [main_cycle_sunny_day]!");
						random = Random.randF(0.f, 1.f);
					}
					if (random > 0.75f && (xr_strcmp(curr_weather, "[main_cycle_thunder_year]") == 0))
					{
						Msg("! !ERROR: overlap weather [main_cycle_thunder_year]!");
						random = Random.randF(0.f, 1.f);
					}
					Msg("Calculate next weather: %s, rand: %f", def.c_str(), random);

						if (random <= 0.25f && (xr_strcmp(curr_weather, "[main_cycle_multy]") != 0) && !changeweather)
						{
							def = "[main_cycle_multy]";
							random = 0.0f;
							changeweather = true;
						}

						if (random <= 0.50f && (xr_strcmp(curr_weather, "[main_cycle_foggy_day]") != 0) && !changeweather)
						{
							def = "[main_cycle_foggy_day]";
							random = 0.0f;
							changeweather = true;
						}

						if (random <= 0.75f && (xr_strcmp(curr_weather, "[main_cycle_sunny_day]") != 0) && !changeweather)
						{
							random = 0.0f;
							def = "[main_cycle_sunny_day]";
							changeweather = true;
						}

						if (random > 0.75f && (xr_strcmp(curr_weather, "[main_cycle_thunder_year]") != 0) && !changeweather)
						{
							random = 0.0f;
							def = "[main_cycle_thunder_year]";
							changeweather = true;
						}
				}

				if (changeweather)
				{
					Msg("- Calculate end");
					g_pGamePersistent->Environment().SetWeather(def, true);
					changeweather = false;
				}

			}
		}
	}

	if (need_change_weather)
	{
		if (Level().game && Device.dwFrame % 4000 == 0)
		{
			need_change_weather = false;
		}
	}
	/// calculate dynamic weather


	/// Calculate Sync Radio Music
	if (set_next_music)
	{
		lenght = 0;
		set_next_music = FALSE;
		need_stop_music = true;
	}

	if (Level().game && !first_play && need_stop_music && lenght <= Device.dwTimeGlobal)
	{
		need_stop_music = false;
		need_next_snd = true;
		stop_timer = Device.dwTimeGlobal + 5000;
		//stop_timer = 0;
		NET_Packet P;
		P.w_begin(M_MUSIC_STOP);
		server().SendBroadcast(BroadcastCID, P, net_flags(true, true));
	}

	if (Level().game && need_next_snd && stop_timer <= Device.dwTimeGlobal)
	{
		i = Random.randI(1, MusicCount);

		need_next_snd = false;

		for (auto entity : phy_objects_cse)
		{
			CSE_Abstract* abs = entity.second.entity;
			CSE_ALifeObjectPhysic* phy = smart_cast<CSE_ALifeObjectPhysic*>(abs);
			if (phy)
			{
				numb += 1;

				if (numb == obj_count + 1)
					numb = 1;
				need_next_snd = false;
				lenght = 0;
				Msg("%d", numb);
				MusicPlay(phy, i, numb);
			}
		}

		need_stop_music = true;
		first_play = false;
	}
	/// Calculate Sync Radio Music




		if (Level().game && Device.dwFrame % box_respawn_time == 0)
		{
			for (auto entity : inventory_boxes_cse)
			{
				CSE_Abstract* abs = entity.second.entity;
				CSE_ALifeInventoryBox* box = smart_cast<CSE_ALifeInventoryBox*>(abs);;

					if (box->children.empty())
					{
						SpawnInvBoxesItems(box);
					}
			}
		}

		if (Level().game && Device.dwFrame % save_time2 == 0)
		{
		//for (int i = 0; i != server().GetEntitiesNum(); i++)
			for(auto entity:inventory_boxes_cse)
			{
			CSE_Abstract* abs = entity.second.entity;
			CSE_ALifeInventoryBox* box = smart_cast<CSE_ALifeInventoryBox*>(abs);
			if (box)
			{
				string_path path_name;
				string64 invbox_name;
				xr_strcpy(invbox_name, box->name_replace());
				xr_strcat(invbox_name, ".ltx");
				FS.update_path(path_name, "$mp_saves_invbox$", invbox_name);


				//check saving box or not
				string_path curr_invbox_name;
				FS.update_path(curr_invbox_name, "$mp_check_saves_invbox$", "save_box_list.ltx");
				CInifile* curr_box_file = xr_new<CInifile>(curr_invbox_name, true);
				LPCSTR box_name = box->name_replace();
				//

				if (!entity.second.loaded)
				{
					inventory_boxes_cse[entity.first].loaded = true;
					CInifile* boxFile = xr_new<CInifile>(path_name, true);
					LoadInvBox(box, boxFile);
					xr_delete (boxFile);
				}
				else if(curr_box_file->line_exist("saving_boxes", box_name))
				{
					CInifile* boxFile = xr_new<CInifile>(path_name, false, false);
					bool can_write = FS.can_modify_file(path_name);
					if (!can_write)                         FS.file_delete(path_name);
					SaveInvBox(box, boxFile);
					boxFile->save_as(path_name);
					xr_delete(boxFile);
				}
				xr_delete(curr_box_file);
				
			}
		}
		oldTime = Device.dwTimeGlobal;
	}

}

BOOL game_sv_freemp::OnTouch(u16 eid_who, u16 eid_what, BOOL bForced)
{
	CSE_ActorMP *e_who = smart_cast<CSE_ActorMP*>(m_server->ID_to_entity(eid_who));
	if (!e_who)
		return TRUE;

	CSE_Abstract *e_entity = m_server->ID_to_entity(eid_what);
	if (!e_entity)
		return FALSE;

	return TRUE;
}

void game_sv_freemp::on_death(CSE_Abstract* e_dest, CSE_Abstract* e_src)
{
	inherited::on_death(e_dest, e_src);

	if (!ai().get_alife())
		return;

	alife().on_death(e_dest, e_src);
}

ALife::_TIME_ID game_sv_freemp::GetStartGameTime()
{
	if (ai().get_alife() && ai().alife().initialized())
		return(ai().alife().time_manager().start_game_time());
	else
		return(inherited::GetStartGameTime());
}

ALife::_TIME_ID game_sv_freemp::GetGameTime()
{
	if (ai().get_alife() && ai().alife().initialized())
		return(ai().alife().time_manager().game_time());
	else
		return(inherited::GetGameTime());
}

ALife::_TIME_ID game_sv_freemp::GetEnvironmentGameTime()
{
	if (ai().get_alife() && ai().alife().initialized())
		return(alife().time_manager().game_time());
	else
		return(inherited::GetGameTime());
}