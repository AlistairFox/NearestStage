#include "stdafx.h"
#include "UIGameFMP.h"
#include "game_cl_freemp.h"
#include "../xrEngine/xr_input.h"

#include "Actor.h"
#include "level.h"
#include "xr_level_controller.h"
#include "GameTaskManager.h"
#include "GameTask.h"

#include "ui/UIStatic.h"
#include "ui/UIXmlInit.h"
#include "ui/UIActorMenu.h"
#include "ui/UIPdaWnd.h"
#include "ui/UITalkWnd.h"
#include "ui/UIMessageBox.h"
#include "UI_AnimMode.h"
#include "Inventory.h"
#include "static_cast_checked.hpp"
#include "CameraEffector.h"
#include "ActorEffector.h"
#include "player_hud.h"
#include "Weapon.h"

BOOL g_cl_draw_mp_statistic = FALSE;

CUIGameFMP::CUIGameFMP()
{
	m_game = NULL;
	m_anims = NULL;
}

CUIGameFMP::~CUIGameFMP()
{
	xr_delete(m_anims);
}

void CUIGameFMP::Init(int stage)
{
	CUIXml uiXml;
	uiXml.Load(CONFIG_PATH, UI_PATH, "ui_game_fmp.xml");

	if (stage == 0)
	{
		//shared
		m_stats = xr_new<CUITextWnd>();
		m_stats->SetAutoDelete(true);

		inherited::Init(stage);

		CUIXmlInit::InitWindow(uiXml, "global", 0, m_window);
		CUIXmlInit::InitTextWnd(uiXml, "stats", 0, m_stats);
	}
	else if (stage == 1)
	{
		//unique

	}
	else if (stage == 2)
	{
		//after
		inherited::Init(stage);
		m_window->AttachChild(m_stats);
	}

	m_anims = xr_new<CUIAMode>();
 	m_anims->Init();
	m_window->AttachChild(m_anims);
}

void CUIGameFMP::SetClGame(game_cl_GameState * g)
{
	inherited::SetClGame(g);
	m_game = smart_cast<game_cl_freemp*>(g);
	R_ASSERT(m_game);
}

void CUIGameFMP::HideShownDialogs()
{
	inherited::HideShownDialogs();
}

extern bool g_bDisableAllInput;

void _BCL CUIGameFMP::OnFrame()
{

	inherited::OnFrame();

	if (need_activate_inventory && old_timer <= Device.dwTimeGlobal)
	{
		CInventoryOwner* pInvOwner = smart_cast<CInventoryOwner*>(Level().CurrentEntity());
		CActor* pActor = smart_cast<CActor*>(pInvOwner);
		if (pInvOwner && pActor && pActor->g_Alive())
		{
			g_player_hud->script_anim_play(2, "item_ea_backpack_open_hud", "anm_ea_idle", false, 1.0f);
			if (!pActor->inventory_disabled())
				ShowActorMenu();
		}

		old_timer = 0;
		need_activate_inventory = false;
	}



	if (g_cl_draw_mp_statistic && Level().game->local_player)
	{
		IClientStatistic& stats = Level().GetStatistic();

		string1024 outstr;
		if (UseDirectPlay())
		{
			xr_sprintf(
				outstr,
				"ping: %u/%u\\n"
				"in/out: %.1f/%.2f KB/s\\n"
				"packets dropped: %u\\n"
				"packets retried: %u\\n",
				Level().game->local_player->ping,
				stats.getPing(),
				stats.getReceivedPerSec() / 1000.0f,
				stats.getSendedPerSec() / 1000.0f,
				stats.getDroppedCount(),
				stats.getRetriedCount()
			);
		}
		else
		{
			xr_sprintf(
				outstr,
				"ping: %u/%u\\n"
				"in/out: %.1f/%.2f KB/s\\n"
				"packets in/out: %.0f/%.0f\\n"
				//"queue time: %u\\n"
				//"send rate: %u bps\\n"
				//"pending reliable: %u\\n"
				//"pending unreliable: %u\\n"
				//"sent unacked reliable: %u\\n"
				"quality local: %.2f\\n"
				"quality remote: %.2f\\n",

				Level().game->local_player->ping,
				stats.getPing(),
				stats.getReceivedPerSec() / 1000.0f,
				stats.getSendedPerSec() / 1000.0f,
				stats.getPacketsInPerSec(),
				stats.getPacketsOutPerSec(),
				//stats.getQueueTime(),
				//stats.getSendRateBytesPerSecond(),
				//stats.getPendingReliable(),
				//stats.getPendingUnreliable(),
				//stats.getSentUnackedReliable(),
				stats.getQualityLocal(),
				stats.getQualityRemote()
			);
		}

		m_stats->SetTextST(outstr);
		m_stats->Enable(true);
	}
	else if (m_stats->IsEnabled())
	{
		m_stats->SetTextST("");
		m_stats->Enable(false);
	}
}

#include "actoreffector.h"
#include <CustomDetector.h>
float CUIGameFMP::add_cam_effector(LPCSTR fn, int id, bool cyclic, LPCSTR cb_func)
{
	CAnimatorCamEffectorScriptCB* e = xr_new<CAnimatorCamEffectorScriptCB>(cb_func);
	e->SetType((ECamEffectorType)id);
	e->SetCyclic(cyclic);
	e->Start(fn);
	Actor()->Cameras().AddCamEffector(e);
	return						e->GetAnimatorLength();
}


bool CUIGameFMP::IR_UIOnKeyboardPress(int dik)
{
	if (inherited::IR_UIOnKeyboardPress(dik)) return true;
	if (Device.Paused()) return false;

	CInventoryOwner* pInvOwner = smart_cast<CInventoryOwner*>(Level().CurrentEntity());
	if (!pInvOwner)			return false;

	CEntityAlive* EA = smart_cast<CEntityAlive*>(Level().CurrentEntity());
	if (!EA || !EA->g_Alive())	return false;

	CActor *pActor = smart_cast<CActor*>(pInvOwner);
	if (!pActor)
		return false;

	if (!pActor->g_Alive())
		return false;

	switch (get_binded_action(dik))
	{
	case kINVENTORY:
	{
		if (!need_activate_inventory)
		{
			Actor()->SetWeaponHideState(INV_STATE_BLOCK_ALL, true);
			CCustomDetector* pDet = smart_cast<CCustomDetector*>(Actor()->inventory().ItemFromSlot(DETECTOR_SLOT));

			if (pDet)
				pDet->HideDetector(true);

			g_player_hud->script_anim_play(2, "item_ea_backpack_open_hud", "anm_ea_show", false, 1.0f);
			Actor()->PlayAnmSound("interface\\item_usage\\backpack_open");
			add_cam_effector("itemuse_anm_effects\\backpack_open.anm", 8555, false, "");
			old_timer = Device.dwTimeGlobal + g_player_hud->motion_length_script("item_ea_backpack_open_hud", "anm_ea_show", 1.0f);
			need_activate_inventory = true;
		}
	} break;

		case kAnimationMode:
		{
			CUIPdaWnd* pda = CurrentGameUI() && &CurrentGameUI()->PdaMenu() ? &CurrentGameUI()->PdaMenu() : nullptr;

			if (!m_anims->IsShown() && !pda->IsShown())
				m_anims->ShowDialog(false);

		}break;

		default:
			break;
//	case kSCORES:
//		if (!pActor->inventory_disabled())
//		{
//			m_game_objective = AddCustomStatic("main_task", true);
//			CGameTask* t1 = Level().GameTaskManager().ActiveTask();
//			m_game_objective->m_static->TextItemControl()->SetTextST((t1) ? t1->m_Title.c_str() : "st_no_active_task");
//
//			if (t1 && t1->m_Description.c_str())
//			{
//				SDrawStaticStruct* sm2 = AddCustomStatic("secondary_task", true);
//				sm2->m_static->TextItemControl()->SetTextST(t1->m_Description.c_str());
//			}
//		}break;
	}
	return false;
}

void  CUIGameFMP::StartUpgrade(CInventoryOwner* pActorInv, CInventoryOwner* pMech)
{
	//.	if( MainInputReceiver() )	return;

	m_ActorMenu->SetActor(pActorInv);
	m_ActorMenu->SetPartner(pMech);

	m_ActorMenu->SetMenuMode(mmUpgrade);
	m_ActorMenu->ShowDialog(true);
}