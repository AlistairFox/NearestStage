#include "stdafx.h"
#include "game_sv_freemp.h"
#include "Level.h"
#include "alife_simulator.h"
#include "alife_spawn_registry.h"
#include "actor_mp_client.h"
#include "alife_time_manager.h"
#include "CustomOutfit.h"

BOOL g_SV_IsVipeMode = FALSE;

game_sv_freemp::game_sv_freemp()
	:pure_relcase(&game_sv_freemp::net_Relcase)
{
	m_type = eGameIDFreeMp;

	if (g_SV_IsVipeMode) 
	{
		
	}
}

game_sv_freemp::~game_sv_freemp()
{
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
	if (smart_cast<CSE_ALifeInventoryBox*>(E))
	{
		inventory_boxes data;
		data.entity = E;
		inventory_boxes_cse[E->ID] = data;
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

	/*string_path file_out;
	FS.update_path(file_out, "$mp_saves_players_outfits$", "players_outfits.ltx");
	CInifile* filoutf = xr_new<CInifile>(file_out, true);
	if (filoutf)
	LoadPlayerOtfits(ps, nullptr);
	xr_delete(filoutf);
	
	string_path file_det;
	FS.update_path(file_det, "$mp_saves_players_detectors$", "players_detectors.ltx");
	CInifile* filedet = xr_new<CInifile>(file_det, true);
	if (filedet)
	LoadPlayerDetectors(ps, nullptr);
	xr_delete(filedet);
	*/

	if (ps->testFlag(GAME_PLAYER_MP_SAVE_LOADED))
	{
		LoadPlayerOtfits(ps, nullptr);
		LoadPlayerDetectors(ps, nullptr);
	}

	if (ps)
	{
		ps->resetFlag(GAME_PLAYER_MP_SAFE_MODE);
		ps->resetFlag(GAME_PLAYER_MP_ANIMATION_MODE);
	}

	if (Game().Type() == eGameIDFreeMp)

		if (ps->testFlag(GAME_PLAYER_MP_SAVE_LOADED))
		{
			xrCData->ps->money_for_round /= Random.randF(1.f, 1.2f);
		}

	if (ps && !ps->testFlag(GAME_PLAYER_MP_SAVE_LOADED))
	{
		string_path file_name;
		string32 filename;
		xr_strcpy(filename, ps->getName());
		xr_strcat(filename, ".ltx");

		FS.update_path(file_name, "$mp_saves_players$", filename);

		Msg("read file path = %s", file_name);

		CInifile* file = xr_new<CInifile>(file_name, true);

		SpawnItemToActor(ps->GameID, "wpn_binoc");
		LoadPlayer(ps, file);
		xr_delete  (file);
				
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

	std::remove_if(save_outfits.begin(), save_outfits.end(), [&](outfits data)
		{
			if (strstr(data.player_name, Name))
				return true;
			else
				return false;
		});

	std::remove_if(save_detectors.begin(), save_detectors.end(), [&](detectors data)
		{
			if (strstr(data.player_name, Name))
				return true;
			else
				return false;
		});
	

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

#include "Actor.h"
extern int save_time;
extern int save_time2;
void game_sv_freemp::Update()
{
	inherited::Update();

	if (Phase() != GAME_PHASE_INPROGRESS)
	{
		OnRoundStart();
	}

	if (!g_pGameLevel)
		return;


	//CTimer timers;
	//timers.Start();

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

				string_path file_name;
				string32 filename;
				xr_strcpy(filename, player.second->getName());
				xr_strcat(filename, ".ltx");

				FS.update_path(file_name, "$mp_saves_players$", filename);

				CInifile* file = xr_new<CInifile>(file_name, false, false);
				SavePlayer(player.second, file);
				file->save_as(file_name);
				SavePlayerOutfits(player.second, nullptr);
				SavePlayerDetectors(player.second, nullptr);
				xr_delete(file);
			}

			//CObject* obj = Level().Objects.net_Find(player.second->GameID);
			//CActor* actor = smart_cast<CActor*>(obj);
/*
			if (!actor)
				return;
			if (!actor->g_Alive())
				return;
			string_path file_outf_name;
			FS.update_path(file_outf_name, "$mp_saves_players_outfits$", "players_outfits.ltx");

			CInifile* outfsFile = xr_new<CInifile>(file_outf_name, false, false);
			SavePlayerOutfits(player.second, outfsFile);
			if(actor->g_Alive())
			outfsFile->save_as(file_outf_name);
			xr_delete(outfsFile);


			string_path file_det_name;
			FS.update_path(file_det_name, "$mp_saves_players_detectors$", "players_detectors.ltx");

			CInifile* detsFile = xr_new<CInifile>(file_det_name, false, false);
			SavePlayerDetectors(player.second, detsFile);
			if (actor->g_Alive())
				detsFile->save_as(file_det_name);
			xr_delete(detsFile);
			*/
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
					xr_delete(curr_box_file);
				}

				
			}
		}
		oldTime = Device.dwTimeGlobal;
	}

	//if (timers.GetElapsed_ms() > 5)
	//{
	//	Msg("save %d", timers.GetElapsed_ms());
	//}
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