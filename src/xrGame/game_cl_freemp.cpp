#include "stdafx.h"
#include "game_cl_freemp.h"
#include "clsid_game.h"
#include "xr_level_controller.h"
#include "UIGameFMP.h"
#include "actor_mp_client.h"
#include "VoiceChat.h"
#include "ui/UIMainIngameWnd.h"
#include "game_news.h"
#include "Inventory.h"
#include "ui/UITalkWnd.h"
#include "ui/UIMessagesWindow.h"
#include "string_table.h"
#include "ui/UIPdaWnd.h"
#include "UIPdaChat.h"

game_cl_freemp::game_cl_freemp()
{
	if (!g_dedicated_server)
		m_pVoiceChat = xr_new<CVoiceChat>();
	else
		m_pVoiceChat = NULL;
}

game_cl_freemp::~game_cl_freemp()
{
	xr_delete(m_pVoiceChat);
}


CUIGameCustom* game_cl_freemp::createGameUI()
{
	if (g_dedicated_server)
		return NULL;

	CLASS_ID clsid = CLSID_GAME_UI_FREEMP;
	m_game_ui = smart_cast<CUIGameFMP*> (NEW_INSTANCE(clsid));
	R_ASSERT(m_game_ui);
	m_game_ui->Load();
	m_game_ui->SetClGame(this);
	return					m_game_ui;
}

void game_cl_freemp::SetGameUI(CUIGameCustom* uigame)
{
	inherited::SetGameUI(uigame);
	m_game_ui = smart_cast<CUIGameFMP*>(uigame);
	R_ASSERT(m_game_ui);

	if (m_pVoiceChat)
	{
		m_game_ui->UIMainIngameWnd->SetVoiceDistance(m_pVoiceChat->GetDistance());
	}
}


void game_cl_freemp::net_import_state(NET_Packet & P)
{
	inherited::net_import_state(P);
}

void game_cl_freemp::net_import_update(NET_Packet & P)
{
	inherited::net_import_update(P);
}

void game_cl_freemp::shedule_Update(u32 dt)
{
	game_cl_GameState::shedule_Update(dt);

	if (!local_player)
		return;

	if (!g_dedicated_server && m_pVoiceChat)
	{
		const bool started = m_pVoiceChat->IsStarted();
		const bool is_dead = !local_player || local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD);
		const bool has_shown_dialogs = CurrentGameUI()->HasShownDialogs();
		if (started && (is_dead || has_shown_dialogs))
		{
			m_pVoiceChat->Stop();
			CurrentGameUI()->UIMainIngameWnd->SetActiveVoiceIcon(false);
		}
		m_pVoiceChat->Update();
	}

	// синхронизация имени и денег игроков для InventoryOwner
	for (auto cl : players)
	{
		game_PlayerState* ps = cl.second;
		if (!ps || ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)) continue;

		CActor* pActor = smart_cast<CActor*>(Level().Objects.net_Find(ps->GameID));
		if (!pActor || !pActor->g_Alive()) continue;

		pActor->SetName(ps->getName());
		pActor->cName_set(ps->getName());

		if (ps->team != pActor->Community())
		{
			CHARACTER_COMMUNITY	community;
			community.set(ps->team);
			pActor->SetCommunity(community.index());
			pActor->ChangeTeam(community.team(), 0, 0);
		}

		if (local_player->GameID == ps->GameID)
		{
			pActor->set_money((u32)ps->money_for_round, false);
		}
	}
}

extern BOOL	g_player_names;

void game_cl_freemp::OnRender()
{
	inherited::OnRender();

	if (g_player_names)
	{
		if (local_player->testFlag(GAME_PLAYER_HAS_ADMIN_RIGHTS))
			if (local_player->testFlag(GAME_PLAYER_SUPER_ADMIN))
			{
				if (local_player)
				{
					PLAYERS_MAP_IT it = players.begin();
					for (;it != players.end();++it)
					{
						game_PlayerState* ps = it->second;
						u16 id = ps->GameID;
						if (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)) continue;
						CObject* pObject = Level().Objects.net_Find(id);
						if (!pObject) continue;
						if (!pObject || !smart_cast<CActor*>(pObject)) continue;
						if (ps == local_player) continue;
						CActor* pActor = smart_cast<CActor*>(pObject);
						string128 text;
						sprintf(text, "%s | %d", ps->getName(), ps->team);
						float pDub = 0.0f;
						Fvector Pos = pActor->Position();
						Pos.y -= pActor->Position().y;
						pActor->RenderText(text, Pos, &pDub, 0xff40ff40);
					}
				};
			}
	}

	if (m_pVoiceChat)
		m_pVoiceChat->OnRender();
}


bool game_cl_freemp::OnKeyboardPress(int key)
{
	switch (key)
	{
	case kVOICE_CHAT:
	{
		if (local_player && !local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
		{
			if (!m_pVoiceChat->IsStarted())
			{
				m_pVoiceChat->Start();
				CurrentGameUI()->UIMainIngameWnd->SetActiveVoiceIcon(true);
			}
		}
		return true;
	}break;

	case kVOICE_DISTANCE:
	{
		if (local_player && !local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
		{
			u8 distance = m_pVoiceChat->SwitchDistance();
			CurrentGameUI()->UIMainIngameWnd->SetVoiceDistance(distance);
		}
		return true;
	}break;

	case kJUMP:
	{
		bool b_need_to_send_ready = false;

		CObject* curr = Level().CurrentControlEntity();
		if (!curr) return(false);

		bool is_actor = !!smart_cast<CActor*>(curr);
		bool is_spectator = !!smart_cast<CSpectator*>(curr);

		game_PlayerState* ps = local_player;
				
		if (is_spectator || (is_actor && ps && ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)))
		{
			b_need_to_send_ready = true;
		}

		if (b_need_to_send_ready)
		{
			CGameObject* GO = smart_cast<CGameObject*>(curr);
			NET_Packet			P;
			GO->u_EventGen(P, GE_GAME_EVENT, GO->ID());
			P.w_u16(GAME_EVENT_PLAYER_READY);
			GO->u_EventSend(P);
			//			SendHelloMsg();
			return				true;
		}
		else
		{
			return false;
		}
	}break;

	default:
		break;
}

	return inherited::OnKeyboardPress(key);
}

LPCSTR game_cl_freemp::GetGameScore(string32&	score_dest)
{
	s32 frags = local_player ? local_player->frags() : 0;
	xr_sprintf(score_dest, "[%d]", frags);
	return score_dest;
}

bool game_cl_freemp::OnKeyboardRelease(int key)
{
	switch (key)
	{
	case kVOICE_CHAT:
	{
		m_pVoiceChat->Stop();
		CurrentGameUI()->UIMainIngameWnd->SetActiveVoiceIcon(false);
		return true;
	}break;

	default:
		break;
	}

	return inherited::OnKeyboardRelease(key);
}

void game_cl_freemp::OnConnected()
{
	inherited::OnConnected();
	if (m_game_ui)
	{
		R_ASSERT(!g_dedicated_server);
		m_game_ui = smart_cast<CUIGameFMP*>	(CurrentGameUI());
		m_game_ui->SetClGame(this);
	}

	luabind::functor<void>	funct;
	R_ASSERT(ai().script_engine().functor("mp_game_cl.on_connected", funct));
	funct();
}

void game_cl_freemp::OnScreenResolutionChanged()
{
	if (m_game_ui && m_pVoiceChat)
	{
		m_game_ui->UIMainIngameWnd->SetVoiceDistance(m_pVoiceChat->GetDistance());
	}
}

void game_cl_freemp::OnVoiceMessage(NET_Packet* P)
{
	m_pVoiceChat->ReceiveMessage(P);
}

void game_cl_freemp::OnChatMessage(NET_Packet* P)
{
	CInventoryOwner* pInvOwner = smart_cast<CInventoryOwner*>(Level().CurrentEntity());
	if (!pInvOwner)
		return;

	CActor* pActor = smart_cast<CActor*>(pInvOwner);
	if (!pActor)
		return;

	shared_str PlayerName;
	shared_str ChatMsg;
	s16 team;
	shared_str PrivatedName;

	P->r_s16();
	P->r_stringZ(PlayerName);
	P->r_stringZ(ChatMsg);
	P->r_s16(team);
	P->r_stringZ(PrivatedName);

	LPCSTR Nameofplayer = PlayerName.c_str();
	LPCSTR Message = ChatMsg.c_str();
	LPCSTR Anonim = "Единая Сталкерская Сеть";
	LPCSTR lastName = local_player->getName();
	GAME_NEWS_DATA				news_data;

	if (team != 0 && team !=160)
	{
		news_data.m_type = (GAME_NEWS_DATA::eNewsType)0;
		news_data.news_caption = Nameofplayer;
		news_data.news_text = Message;
		news_data.show_time = 3000;// override default

//		VERIFY(xr_strlen("ui_inGame2_Hero") > 0);

		shared_str PlayerTex = "ui_inGame2_Hero";

		if (xr_strcmp(Nameofplayer, "АлистарФокс") == 0)
			PlayerTex = "ui_inGame2_player_alistarfox";

		else if (xr_strcmp(Nameofplayer, "lintelli") == 0)
			PlayerTex = "ui_inGame2_player_lintelli";

		else if (team == 2)
			PlayerTex = "ui_inGame2_bandit_1";

		else if (team == 3)
			PlayerTex = "ui_inGame2_Dolg_1";

		else if (team == 4)
			PlayerTex = "ui_inGame2_ecolog_military";

		else if (team == 5)
			PlayerTex = "ui_inGame2_Freedom_1";

		else if (team == 6)
			PlayerTex = "ui_inGame2_merc_2";

		else if (team == 7)
			PlayerTex = "ui_inGame2_Soldier_3";

		else if (team == 8)
			PlayerTex = "ui_inGame2_monolit_2";

		else if (team == 9)
			PlayerTex = "ui_inGame2_neutral_2_mask";

		else if (team == 12)
			PlayerTex = "ui_inGame2_obre4en";

		else if (team == 50)
			PlayerTex = "ui_inGame2_V_poiskah_Soroki";


		news_data.texture_name = PlayerTex;

			if (pActor->inventory().m_slots[PDA_SLOT].m_pIItem && !g_dedicated_server)
			{
				Actor()->AddGameNews(news_data);
			}
	}
	else if (team == 160 && lastName == PrivatedName)
	{
		string256 str;
		sprintf(str, "Принято [%s -> %s (private)]", Nameofplayer, lastName);
			news_data.m_type = (GAME_NEWS_DATA::eNewsType)0;
			news_data.news_caption = str;
			news_data.news_text = Message;
			news_data.show_time = 3000;// override default

			news_data.texture_name = "ui_inGame2_PD_Torgovets_informatsiey";

			if (pActor->inventory().m_slots[PDA_SLOT].m_pIItem && !g_dedicated_server)
			{
				Actor()->AddGameNews(news_data);
			}
	}
	else if(team == 0)
	{
		news_data.m_type = (GAME_NEWS_DATA::eNewsType)0;
		news_data.news_caption = Anonim;
		news_data.news_text = Message;
		news_data.show_time = 3000;// override default

		VERIFY(xr_strlen("ui_inGame2_Hero") > 0);

		news_data.texture_name = "ui_inGame2_Radiopomehi";

		if (pActor->inventory().m_slots[PDA_SLOT].m_pIItem)
		Actor()->AddGameNews(news_data);
	}
}

void game_cl_freemp::TranslateGameMessage(u32 msg, NET_Packet& P) 
{
	switch (msg) {

	case (GAME_EVENT_NEWS_MESSAGE): 
	{
		shared_str name, icon, name_item;
		u16 count;

		P.r_stringZ(name_item);
		P.r_stringZ(name);
		P.r_u16(count);
		P.r_stringZ(icon);
		LPCSTR transl_name = {};
		shared_str item_name = {};

		if (pSettings->section_exist(name_item))
		{
			item_name = pSettings->r_string(name_item, "inv_name");
			transl_name = CStringTable().translate(item_name).c_str();
		}

		string1024 release_text = {};
		sprintf(release_text, "%s - %u",transl_name, count);
		Msg("release_text: %s", release_text);

		GAME_NEWS_DATA data;
		data.m_type = data.eNews;
		data.news_caption = name.c_str();
		data.news_text = release_text;
		data.texture_name = icon;
		data.receive_time = Level().GetGameTime();

		if (CurrentGameUI())
		{
			bool talk = CurrentGameUI()->TalkMenu && CurrentGameUI()->TalkMenu->IsShown();

			if (CurrentGameUI()->UIMainIngameWnd && !talk)
				CurrentGameUI()->m_pMessagesWnd->AddIconedPdaMessage(&data);
			else
				if (talk)
					CurrentGameUI()->TalkMenu->AddIconedMessage(name.c_str(), release_text, icon.c_str(), "iconed_answer_item");
		}
	}break;
	case GAME_EVENT_NEWS_MONEY_MESSAGE:
	{
		shared_str name, text, icon;
		P.r_stringZ(name);
		P.r_stringZ(text);
		P.r_stringZ(icon);
		GAME_NEWS_DATA data;
		data.m_type = data.eNews;
		data.news_caption = name;
		data.news_text = text;
		data.texture_name = icon;
		data.receive_time = Level().GetGameTime();

			if (CurrentGameUI())
			{
				bool talk = CurrentGameUI()->TalkMenu && CurrentGameUI()->TalkMenu->IsShown();

				if (CurrentGameUI()->UIMainIngameWnd && !talk)
					CurrentGameUI()->m_pMessagesWnd->AddIconedPdaMessage(&data);
				else
					if (talk)
						CurrentGameUI()->TalkMenu->AddIconedMessage(name.c_str(), text.c_str(), icon.c_str(), "iconed_answer_item");
			}

	}break;
	case GE_PDA_CHAT:
	{
		if (!g_dedicated_server)
			m_game_ui->PdaMenu().pUIChatWnd->RecivePacket(P);
	}break;

	default:
		inherited::TranslateGameMessage(msg, P);
	}
}

/*
void game_cl_freemp::SendHelloMsg()
{
	LPCSTR Message = "Единая Сталкерская Сеть.";
	for (auto cl : players)
	{
		game_PlayerState* ps = cl.second;
		if (!ps || ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)) continue;
		CActor* pActor = smart_cast<CActor*>(Level().Objects.net_Find(ps->GameID));
		if (!pActor || !pActor->g_Alive()) continue;
		Message = "%p, Единая Сталкерская Сеть.", pActor->Name();
	}
	LPCSTR Nameofplayer = "Единая Сталкерская Сеть";
	GAME_NEWS_DATA				news_data;
	shared_str PlayerTex = "ui_inGame2_Radiopomehi";
	news_data.m_type = (GAME_NEWS_DATA::eNewsType)0;
	news_data.news_caption = Nameofplayer;
	news_data.news_text = Message;
	news_data.show_time = 3000;// override default
	news_data.texture_name = PlayerTex;
	pActor()->AddGameNews(news_data);
}
*/