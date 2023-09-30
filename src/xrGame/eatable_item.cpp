////////////////////////////////////////////////////////////////////////////
//	Module 		: eatable_item.cpp
//	Created 	: 24.03.2003
//  Modified 	: 29.01.2004
//	Author		: Yuri Dobronravin
//	Description : Eatable item
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "eatable_item.h"
#include "xrmessages.h"
#include "physic_item.h"
#include "Level.h"
#include "entity_alive.h"
#include "EntityCondition.h"
#include "InventoryOwner.h"
#include "Actor.h"
#include "Inventory.h"
#include "Level.h"
#include "game_object_space.h"
#include "ai_object_location.h"
#include "Weapon.h"
#include "actorEffector.h"
#include "HudManager.h"
#include "UIGameCustom.h"
#include "player_hud.h"
#include "../xrPhysics/ElevatorState.h"
#include <static_cast_checked.hpp>
#include <game_cl_freemp.h>
#include "UIGameFMP.h"
#include "ui/UIActorMenu.h"

CEatableItem::CEatableItem()
{
	m_iPortionsNum = -1;
	use_cam_effector = nullptr;
	anim_sect = nullptr;
	m_physic_item = 0;
	m_iAnimHandsCnt = 1;
	m_iAnimLength = 0;
	m_bActivated = false;
	m_bItmStartAnim = false;
}

CEatableItem::~CEatableItem()
{
}

DLL_Pure* CEatableItem::_construct()
{
	m_physic_item = smart_cast<CPhysicItem*>(this);
	return			(inherited::_construct());
}

void CEatableItem::Load(LPCSTR section)
{
	inherited::Load(section);

	m_iPortionsNum = pSettings->r_s32(section, "eat_portions_num");
	anim_sect = READ_IF_EXISTS(pSettings, r_string, section, "anm", nullptr);
	m_iCamEffector = READ_IF_EXISTS(pSettings, r_string, section, "cam", nullptr);
	//m_iHudAnm = pSettings->r_string(section, "hud_anm");
	VERIFY(m_iPortionsNum < 10000);
}

BOOL CEatableItem::net_Spawn(CSE_Abstract* DC)
{
	if (!inherited::net_Spawn(DC)) return FALSE;

	return TRUE;
};

bool CEatableItem::Useful() const
{
	if (!inherited::Useful()) return false;

	//проверить не все ли еще съедено
	if (m_iPortionsNum == 0) return false;

	return true;
}

void CEatableItem::OnH_A_Independent()
{
	inherited::OnH_A_Independent();
	if (!Useful()) {
		if (object().Local() && OnServer())	object().DestroyObject();
	}
}

void CEatableItem::OnH_B_Independent(bool just_before_destroy)
{
	if (!Useful())
	{
		object().setVisible(FALSE);
		object().setEnabled(FALSE);
		if (m_physic_item)
			m_physic_item->m_ready_to_destroy = true;
	}
	inherited::OnH_B_Independent(just_before_destroy);
}

void CEatableItem::UpdateCL()
{
	inherited::UpdateCL();

	if (need_hide_timer && HideTimer <= Device.dwTimeGlobal)
	{
		CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
		if (pActor)
			pActor->TimeBlockAction(anim_sect);

		if(pActor && m_iCamEffector != nullptr)
			pActor->add_cam_effector(m_iCamEffector, 8555, false, "");

		m_bItmStartAnim = true;

		need_hide_timer = false;
		HideTimer = 0;
	}

	if (m_bItmStartAnim && m_pInventory->GetActiveSlot() == NO_ACTIVE_SLOT)
		StartAnimation();

	if (m_bActivated && OnClient())
	{
		if (m_iAnimLength <= Device.dwTimeGlobal)
		{
			m_iAnimLength = Device.dwTimeGlobal;
			m_bActivated = false;


			NET_Packet P;
			Game().u_EventGen(P, GEG_PLAYER_NEED_EAT_ITEM, this->object_id());
			Level().Send(P, net_flags(TRUE, TRUE, FALSE, TRUE));

			m_pInventory->ClientEat(this);
		}
	}
}

void CEatableItem::HideWeapon()
{

		if (OnClient())
		{
			CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
			game_cl_freemp* huita = smart_cast<game_cl_freemp*>(&Game());
			if (huita && huita->m_game_ui->ActorMenu().IsShown())
			{
				huita->m_game_ui->HideActorMenu();
				g_player_hud->script_anim_play(2, "item_ea_backpack_close_hud", "anm_ea_show", false, 1.0f);
				pActor->PlayAnmSound("interface\\item_usage\\backpack_close");
				pActor->add_cam_effector("itemuse_anm_effects\\backpack_open.anm", 8555, false, "");
				HideTimer = Device.dwTimeGlobal + 1500;
				need_hide_timer = true;
			}
			else
			{
				if (pActor)
					pActor->TimeBlockAction(anim_sect);
				pActor->add_cam_effector(m_iCamEffector, 8555, false, "");
				m_bItmStartAnim = true;
			}
		}

		if (OnServer())
		{
			NET_Packet				tmp_packet;
			CGameObject::u_EventGen(tmp_packet, GEG_PLAYER_START_ANIMATION, object().ID());
			Level().Send(tmp_packet);
		}
}

void CEatableItem::OnEvent(NET_Packet& P, u16 type)
{
	switch (type)
	{
	case GEG_PLAYER_START_ANIMATION:
	{
		m_bItmStartAnim = true;
		return;
	}
	case GEG_PLAYER_NEED_EAT_ITEM:
	{
		if (OnServer)
		{
			Msg("server eat item");
			m_pInventory->Eat(this);
		}
	}
	default:
		inherited::OnEvent(P, type);
		break;
	}
}

void CEatableItem::StartAnimation()
{
	m_bActivated = true;


	if (pSettings->line_exist(anim_sect, "single_handed_anim"))
		m_iAnimHandsCnt = pSettings->r_u32(anim_sect, "single_handed_anim");

	m_bItmStartAnim = false;

	if (pSettings->line_exist(anim_sect, "anm_ea_show"))
	{
		g_player_hud->script_anim_play(m_iAnimHandsCnt, anim_sect, "anm_ea_show", false, 1.0f);
		m_iAnimLength = Device.dwTimeGlobal + g_player_hud->motion_length_script(anim_sect, "anm_ea_show", 1.0f);
	}

	if (pSettings->line_exist(anim_sect, "postprocess"))
	{
		Actor()->PlayPPEffect(pSettings->r_string(anim_sect, "postprocess"));
	}

	if (pSettings->line_exist(anim_sect, "snd"))
	{
		if (m_using_sound._feedback())
			m_using_sound.stop();

		shared_str snd_name = pSettings->r_string(anim_sect, "snd");
		m_using_sound.create(snd_name.c_str(), st_Effect, sg_SourceType);
		m_using_sound.play(NULL, sm_2D);
	}
}

bool CEatableItem::UseBy(CEntityAlive* entity_alive)
{
	SMedicineInfluenceValues	V;
	V.Load(m_physic_item->cNameSect());

	CInventoryOwner* IO = smart_cast<CInventoryOwner*>(entity_alive);
	R_ASSERT(IO);
	R_ASSERT(m_pInventory == IO->m_inventory);
	R_ASSERT(object().H_Parent()->ID() == entity_alive->ID());

	entity_alive->conditions().ApplyInfluence(V, m_physic_item->cNameSect());

	for (u8 i = 0; i < (u8)eBoostMaxCount; i++)
	{
		if (pSettings->line_exist(m_physic_item->cNameSect().c_str(), ef_boosters_section_names[i]))
		{
			SBooster B;
			B.Load(m_physic_item->cNameSect(), (EBoostParams)i);
			entity_alive->conditions().ApplyBooster(B, m_physic_item->cNameSect());
		}
	}

	if (!IsGameTypeSingle() && OnServer())
	{
		NET_Packet				tmp_packet;
		CGameObject::u_EventGen(tmp_packet, GEG_PLAYER_USE_BOOSTER, entity_alive->ID());
		tmp_packet.w_u16(object_id());
		Level().Send(tmp_packet);
	}

	if (m_iPortionsNum > 0)
		--m_iPortionsNum;
	else
		m_iPortionsNum = 0;

	return true;
}