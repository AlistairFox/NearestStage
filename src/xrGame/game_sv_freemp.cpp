#include "stdafx.h"
#include "game_sv_freemp.h"
#include "Level.h"
#include "alife_simulator.h"
#include "alife_spawn_registry.h"
#include "actor_mp_client.h"
#include "alife_time_manager.h"
#include "CustomOutfit.h"
#include "Actor.h"
#include <ui/UIInventoryUtilities.h>

game_sv_freemp::game_sv_freemp()
	:pure_relcase(&game_sv_freemp::net_Relcase)
{
	m_type = eGameIDFreeMp;

	FS.update_path(spawn_config, "$game_config$", "alife\\start_stuf.ltx");
	spawn_file = xr_new<CInifile>(spawn_config, true, true);

	DynamicBoxFileCreate();
	DynamicMusicFileCreate();

	Saver = xr_new<CProgressSaver>(this);
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
	xr_delete(spawn_file);
	xr_delete(Saver);
}


void game_sv_freemp::Create(shared_str & options)
{
	inherited::Create(options);
	//R_ASSERT2(rpoints[0].size(), "rpoints for players not found");

	//	if (strstr(*options, "/alife"))
//		m_alife_simulator = xr_new<CALifeSimulator>(&server(), &options);

	switch_Phase(GAME_PHASE_PENDING);

	::Random.seed(GetTickCount());
	m_CorpseList.clear();

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
		phy_objects_cse[E->ID] = E;
	}
}

// player connect #1
void game_sv_freemp::OnPlayerConnect(ClientID id_who)
{
	inherited::OnPlayerConnect(id_who);

	xrClientData* xrCData = m_server->ID_to_client(id_who);
	game_PlayerState*	ps_who = get_id(id_who);

	if (af_debug_loggining)
		Msg("Player: Name: %s, StaticID: %d", ps_who->getName(), ps_who->GetStaticID());

	if (!xrCData->flags.bReconnect)
	{
		ps_who->clear();
		ps_who->team = 8;
		ps_who->skin = -1;
	};
	ps_who->setFlag(GAME_PLAYER_FLAG_SPECTATOR);

	ps_who->resetFlag(GAME_PLAYER_FLAG_SKIP);


	std::string SName = ps_who->getName();
	string_path JsonFilePath;
	FS.update_path(JsonFilePath, "$mp_saves_logins$", "logins.json");
	Object JsonMain;
	Array AccountsArray;
	std::ifstream ifile(JsonFilePath);
	std::string str((std::istreambuf_iterator<char>(ifile)), std::istreambuf_iterator<char>());
	JsonMain.parse(str);
	ifile.close();
	u8 AdminLevel = 0;

	if (JsonMain.has<Array>("ACCOUNTS:"))
	{
		AccountsArray = JsonMain.get<Array>("ACCOUNTS:");
		for (int i = 0; i != AccountsArray.size(); i++)
		{
			Object NowCheckAccount = AccountsArray.get<Object>(i);
			if (NowCheckAccount.has<String>("login"))
			{
				std::string AccName = NowCheckAccount.get<String>("login");
				if (AccName == SName)
				{
					if (NowCheckAccount.has<Number>("Admin"))
					{
						AdminLevel = NowCheckAccount.get<Number>("Admin");
					}
				}
			}
		}
	}

	if (AdminLevel == 1)
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
	else if (AdminLevel == 2)
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
		
			if (Saver && Saver->HasBinnarSaveFile(xrCData->ps))
			{
				xrCData->ps->resetFlag(GAME_PLAYER_MP_SAVE_LOADED);
			}
			else
			{
				xrCData->ps->setFlag(GAME_PLAYER_MP_SAVE_LOADED);
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

	Msg("Player connected: name: %s, static id: %d", ps->getName(), ps->GetStaticID());

	string_path JsonFilePath;
	FS.update_path(JsonFilePath, "$mp_saves_logins$", "logins.json");

	Object JsonMain;
	Array AccountsArray;
	Array ReloadAccountsArray;

	std::ifstream ifile(JsonFilePath);

	std::string str((std::istreambuf_iterator<char>(ifile)), std::istreambuf_iterator<char>());
	JsonMain.parse(str);
	ifile.close();
	u8 kit_numb = 1;

	std::string SName = ps->getName();
	bool need_load_stuf = true;
	if (JsonMain.has<Array>("ACCOUNTS:"))
	{
		Object NowCheckAccounts;
		AccountsArray = JsonMain.get<Array>("ACCOUNTS:");
		for (int i = 0; i != AccountsArray.size(); i++)
		{
			NowCheckAccounts = AccountsArray.get<Object>(i);
			if (NowCheckAccounts.has<String>("login"))
			{
				std::string AccLogin = NowCheckAccounts.get<String>("login");
				if (SName == AccLogin)
				{
					if (NowCheckAccounts.has<String>("stafloaded"))
						need_load_stuf = false;
					else
						NowCheckAccounts << "stafloaded" << "true";

					kit_numb = NowCheckAccounts.get<Number>("kit");
				}
			}
			ReloadAccountsArray << NowCheckAccounts;
		}
	}

	if (need_load_stuf)
	{
		JsonMain << "ACCOUNTS:" << ReloadAccountsArray;
		IWriter* writer = FS.w_open(JsonFilePath);
		if (writer)
		{
			writer->w_string(JsonMain.json().c_str());
			FS.w_close(writer);
		}

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

			if (spawn_file->section_exist(spawn_section))
			{
				for (u32 k = 0; spawn_file->r_line(spawn_section, k, &N, &V); k++)
				{
					SpawnItemToActor(ps->GameID, N);
				}
			}
			else
				Msg("!! Can't Find spawn_kit section: [%s]", spawn_section);
	}

	if (ps)
	{
		ps->resetFlag(GAME_PLAYER_MP_SAFE_MODE);
		ps->resetFlag(GAME_PLAYER_MP_WOUND_MODE);
		ps->resetFlag(GAME_PLAYER_MP_LOOT_MODE);
	}

	if (Game().Type() == eGameIDFreeMp)
		if(Saver)
			Saver->OnPlayerRespawn(ps);
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
void game_sv_freemp::OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID, u16 StaticID)
{
	NET_Packet					P;
	GenerateGameMessage(P);
	P.w_u32(GAME_EVENT_PLAYER_DISCONNECTED);
	P.w_stringZ(Name);
	u_EventSend(P);
	//---------------------------------------------------
	if (Saver)
	{
		Saver->OnPlayerDisconnect(Name, StaticID);
	}

//	AllowDeadBodyRemove			(id_who, GameID);
	CObject* pObject = Level().Objects.net_Find(GameID);

	inherited::OnPlayerDisconnect(id_who, Name, GameID, StaticID);

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

void game_sv_freemp::assign_RP(CSE_Abstract* E, game_PlayerState* ps_who)
{
	Fvector pos, angle;
	float health;
	if (Saver && !ps_who->testFlag(GAME_PLAYER_MP_SAVE_LOADED) && Saver->load_position_RP_Binnar(ps_who, pos, angle))
	{
		E->position().set(pos);
		E->angle().set(angle);
	}
	else
		inherited::assign_RP(E, ps_who);
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

void game_sv_freemp::Update()
{
	inherited::Update();

	if (Phase() != GAME_PHASE_INPROGRESS)
	{
		OnRoundStart();
	}

	if (!g_pGameLevel)
		return;

	DynamicWeatherUpdate();
	DynamicMusicUpdate();
	DynamicBoxUpdate();

	if(Saver)
		Saver->SaveManagerUpdate();
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