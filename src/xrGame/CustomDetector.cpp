﻿#include "stdafx.h"
#include "customdetector.h"
#include "ui/ArtefactDetectorUI.h"
#include "hudmanager.h"
#include "inventory.h"
#include "level.h"
#include "map_manager.h"
#include "ActorEffector.h"
#include "actor.h"
#include "ui/UIWindow.h"
#include "player_hud.h"
#include "weapon.h"
#include "Battery.h"
#include "ui/UIActorMenu.h"
#include "UIGameCustom.h"

ITEM_INFO::ITEM_INFO()
{
	pParticle	= NULL;
	curr_ref	= NULL;
}

ITEM_INFO::~ITEM_INFO()
{
	if(pParticle)
		CParticlesObject::Destroy(pParticle);
}

bool CCustomDetector::CheckCompatibilityInt(CHudItem* itm, u16* slot_to_activate)
{
	if(itm==NULL)
		return true;

	CInventoryItem& iitm			= itm->item();
	u32 slot						= iitm.BaseSlot();
	bool bres = (slot==INV_SLOT_2 || slot==KNIFE_SLOT || slot==BOLT_SLOT);
	if(!bres && slot_to_activate)
	{
		*slot_to_activate	= NO_ACTIVE_SLOT;
		if(m_pInventory->ItemFromSlot(BOLT_SLOT))
			*slot_to_activate = BOLT_SLOT;

		if(m_pInventory->ItemFromSlot(KNIFE_SLOT))
			*slot_to_activate = KNIFE_SLOT;

		if(m_pInventory->ItemFromSlot(INV_SLOT_3) && m_pInventory->ItemFromSlot(INV_SLOT_3)->BaseSlot()!=INV_SLOT_3)
			*slot_to_activate = INV_SLOT_3;

		if(m_pInventory->ItemFromSlot(INV_SLOT_2) && m_pInventory->ItemFromSlot(INV_SLOT_2)->BaseSlot()!=INV_SLOT_3)
			*slot_to_activate = INV_SLOT_2;

		if(*slot_to_activate != NO_ACTIVE_SLOT)
			bres = true;
	}

	if(itm->GetState()!=CHUDState::eShowing)
		bres = bres && !itm->IsPending();

	if(bres)
	{
		CWeapon* W = smart_cast<CWeapon*>(itm);
		if(W)
			bres =	bres								&& 
					(W->GetState()!=CHUDState::eBore)	&& 
					(W->GetState()!=CWeapon::eReload) && 
					(W->GetState()!=CWeapon::eSwitch) && 
					!W->IsZoomed();
	}
	return bres;
}

bool  CCustomDetector::CheckCompatibility(CHudItem* itm)
{
	if(!inherited::CheckCompatibility(itm) )	
		return false;

	if(!CheckCompatibilityInt(itm, NULL))
	{
		HideDetector	(true);
		return			false;
	}
	return true;
}

void CCustomDetector::HideAndSetCallback(detector_fn_t fn)
{
	m_bNeedActivation = false;
	m_bFastAnimMode = true;
	SwitchState(eHiding);

	hide_callback = fn;
}

void CCustomDetector::HideDetector(bool bFastMode)
{
	if(GetState()==eIdle)
		ToggleDetector(bFastMode);
}

void CCustomDetector::ShowDetector(bool bFastMode)
{
	if (CurrentGameUI()->ActorMenu().IsShown())
		return;
	if(GetState()==eHidden)
		ToggleDetector(bFastMode);
}

void CCustomDetector::ToggleDetector(bool bFastMode)
{
	if (CurrentGameUI()->ActorMenu().IsShown())
		return;
	Msg("ToggleDetector");
	m_bNeedActivation		= false;
	m_bFastAnimMode			= bFastMode;

	if(GetState()==eHidden)
	{
		PIItem iitem = m_pInventory->ActiveItem();
		CHudItem* itm = (iitem)?iitem->cast_hud_item():NULL;
		u16 slot_to_activate = NO_ACTIVE_SLOT;

		if(CheckCompatibilityInt(itm, &slot_to_activate))
		{
			if(slot_to_activate!=NO_ACTIVE_SLOT)
			{
				if (OnServer())
				{
					// Пытаемся достать допустимый предмет: нож, оружие или тп
					// при этом будет спрятано текущее оружие
					m_pInventory->Activate(slot_to_activate);
				}
				else
				{
					if (H_Parent() && H_Parent() == Level().CurrentViewEntity())
					{
						NET_Packet						P;
						CGameObject::u_EventGen(P, GEG_PLAYER_ACTIVATE_SLOT, H_Parent()->ID());
						P.w_u16(slot_to_activate);
						CGameObject::u_EventSend(P);
					}
				}
				// указываем, что нужно будет достать детектор
				m_bNeedActivation = true;
			}
			else
			{
				SwitchState				(eShowing);
				TurnDetectorInternal	(true);
			}
		}
	}else
	if(GetState()==eIdle)
		SwitchState					(eHiding);

}
void CCustomDetector::SwitchState(u32 S)
{
	if (IsGameTypeSingle() || OnServer())
	{
		inherited::SwitchState(S);
		return;
	}

	if (!IsGameTypeSingle() && OnClient())
	{
		SetNextState(S);
		OnStateSwitch(u32(S));

		switch (S)
		{
		case eHidden:
			if (hide_callback)
			{
				hide_callback();
			}
			m_bDetectorActive = false;
			ClearCallback();
			break;
		case eShowing:
		case eIdle:
			m_bDetectorActive = true;
			ClearCallback();
			break;
		default:
			break;
		}
	}
}


void CCustomDetector::OnStateSwitch(u32 S)
{
	inherited::OnStateSwitch(S);

	switch(S)
	{
	case eShowing:
		{
		m_bDetectorActive = true;
			g_player_hud->attach_item	(this);
			m_sounds.PlaySound			(m_bFastAnimMode?"sndShowFast":"sndShow", Fvector().set(0,0,0), this, true, false);
			PlayHUDMotion				(m_bFastAnimMode?"anm_show_fast":"anm_show", FALSE/*TRUE*/, this, GetState());
			SetPending					(TRUE);
		}break;
	case eHiding:
		{
		m_bDetectorActive = false;
			m_sounds.PlaySound			(m_bFastAnimMode?"sndHideFast":"sndHide", Fvector().set(0,0,0), this, true, false);
			PlayHUDMotion				(m_bFastAnimMode?"anm_hide_fast":"anm_hide", FALSE/*TRUE*/, this, GetState());
			SetPending					(TRUE);
		}break;
	case eIdle:
		{
		m_bDetectorActive = true;
			PlayAnimIdle				();
			SetPending					(FALSE);
		}break;
}
}

void CCustomDetector::OnAnimationEnd(u32 state)
{
	inherited::OnAnimationEnd	(state);
	switch(state)
	{
	case eShowing:
		{
			SwitchState					(eIdle);
		} break;
	case eHiding:
		{
			SwitchState					(eHidden);
			TurnDetectorInternal		(false);
			g_player_hud->detach_item	(this);
		} break;
	}
}

void CCustomDetector::UpdateXForm()
{
	CInventoryItem::UpdateXForm();
}

void CCustomDetector::OnActiveItem()
{
	return;
}

void CCustomDetector::OnHiddenItem()
{
}

CCustomDetector::CCustomDetector() 
{
	m_ui				= NULL;
	m_bFastAnimMode		= false;
	m_bNeedActivation	= false;

	m_fMaxChargeLevel = 0.0f;
	m_fCurrentChargeLevel = 1.0f;
	m_fUnchargeSpeed = 0.0f;
	m_SuitableBattery = nullptr;
}

CCustomDetector::~CCustomDetector() 
{
	m_artefacts.destroy		();
	TurnDetectorInternal	(false);
	xr_delete				(m_ui);
}

BOOL CCustomDetector::net_Spawn(CSE_Abstract* DC) 
{
	TurnDetectorInternal(false);
	return		(inherited::net_Spawn(DC));
}

void CCustomDetector::Load(LPCSTR section) 
{
	inherited::Load			(section);

	m_fAfDetectRadius		= pSettings->r_float(section,"af_radius");
	m_fAfVisRadius			= pSettings->r_float(section,"af_vis_radius");
	m_artefacts.load		(section, "af");

	m_sounds.LoadSound( section, "snd_draw", "sndShow");
	m_sounds.LoadSound( section, "snd_holster", "sndHide");
	m_sounds.LoadSound(section, "snd_draw_fast", "sndShowFast");
	m_sounds.LoadSound(section, "snd_holster_fast", "sndHideFast");

	m_fMaxChargeLevel = READ_IF_EXISTS(pSettings, r_float, section, "max_charge_level", 1.0f);
	m_fUnchargeSpeed = READ_IF_EXISTS(pSettings, r_float, section, "uncharge_speed", 0.0f);
	m_SuitableBattery = READ_IF_EXISTS(pSettings, r_string, section, "suitable_battery", "torch_battery");

	float rnd_charge = ::Random.randF(0.0f, m_fMaxChargeLevel);
	m_fCurrentChargeLevel = rnd_charge;

}

void CCustomDetector::OnEvent(NET_Packet& P, u16 type)
{
	switch (type)
	{
	case GEG_PLAYER_CHARGE_DETECTORS:
	{
		float charge_level;
		P.r_float(charge_level);
		Recharge(charge_level);

	}break;
	default:
		inherited::OnEvent(P, type);
		break;
	}
}

void CCustomDetector::shedule_Update(u32 dt) 
{
	inherited::shedule_Update(dt);
	
	if( !IsWorking() )			return;

	Position().set(H_Parent()->Position());

	Fvector						P; 
	P.set						(H_Parent()->Position());
	m_artefacts.feel_touch_update(P,m_fAfDetectRadius);
}


bool CCustomDetector::IsWorking()
{
	return m_bWorking && H_Parent() && H_Parent()==Level().CurrentViewEntity();
}

void CCustomDetector::UpfateWork()
{
	if (m_fCurrentChargeLevel > 0)
	UpdateAf				();
	m_ui->update			();
}

void CCustomDetector::net_Export(NET_Packet& P)
{
	inherited::net_Export(P);
	P.w_float(GetCondition());
	P.w_float(m_fCurrentChargeLevel);
}

void CCustomDetector::net_Import(NET_Packet& P)
{
	inherited::net_Import(P);

		P.r_float(m_fCondition);
		P.r_float(m_fCurrentChargeLevel);
}

void CCustomDetector::UpdateVisibility()
{
	// Pavel: Предотвращение краша при дисконнекте сервера
	if (!Actor())
		return;

	// Pavel: ХАК. Прячем детектор, если в руках оказывается оружие (кроме пистолета, ножа, болта)
	if (!IsGameTypeSingle() && OnClient())
	{
		u16 curSlot = m_pInventory->GetActiveSlot();
		if (curSlot != NO_ACTIVE_SLOT)
		{
			PIItem pWpn = m_pInventory->ItemFromSlot(m_pInventory->GetNextActiveSlot());
			if (pWpn)
			{
				u32 slot = pWpn->BaseSlot();
				bool bres = (slot == INV_SLOT_2 || slot == KNIFE_SLOT || slot == BOLT_SLOT || curSlot == NO_ACTIVE_SLOT);
				if (GetState() == eIdle || GetState() == eShowing) {
					if (!bres)
					{
						m_bNeedActivation = false;
						m_bFastAnimMode = true;
						SwitchState(eHiding);
						// HideDetector(true);
						return;
					}
				}
			}
		}
	}

	//check visibility
	attachable_hud_item* i0		= g_player_hud->attached_item(0);
	if(i0 && HudItemData())
	{
		bool bClimb			= ( (Actor()->MovingState()&mcClimb) != 0 );
		if(bClimb)
		{
			HideDetector		(true);
			m_bNeedActivation	= true;
		}else
		{
			CWeapon* wpn			= smart_cast<CWeapon*>(i0->m_parent_hud_item);
			if(wpn)
			{
				u32 state			= wpn->GetState();
				if(wpn->IsZoomed() || state==CWeapon::eReload || state==CWeapon::eSwitch)
				{
					HideDetector		(true);
					m_bNeedActivation	= true;
				}
			}
		}
	}else
	if(m_bNeedActivation)
	{
		attachable_hud_item* i0		= g_player_hud->attached_item(0);
		bool bClimb					= ( (Actor()->MovingState()&mcClimb) != 0 );
		if(!bClimb)
		{
			CHudItem* huditem		= (i0)?i0->m_parent_hud_item : NULL;
			bool bChecked			= !huditem || CheckCompatibilityInt(huditem, 0);
			
			if(	bChecked )
				ShowDetector		(true);
		}
	}
}

void CCustomDetector::UpdateCL() 
{
	inherited::UpdateCL();

	UpdateChargeLevel();

	if(H_Parent()!=Level().CurrentEntity() )			return;

	UpdateVisibility		();
	if( !IsWorking() )		return;
	UpfateWork				();
}

void CCustomDetector::OnH_A_Chield() 
{
	inherited::OnH_A_Chield		();
}

void CCustomDetector::OnH_B_Independent(bool just_before_destroy) 
{
	inherited::OnH_B_Independent(just_before_destroy);
	
	m_artefacts.clear			();
}


void CCustomDetector::OnMoveToRuck(const SInvItemPlace& prev)
{
	inherited::OnMoveToRuck	(prev);
	if(prev.type==eItemPlaceSlot)
	{
		SwitchState					(eHidden);
		g_player_hud->detach_item	(this);

		// Pavel: фикс для мп, не нужно доставать детектор,
		// если он был убран / заменён на другой во время перезарядки
		m_bNeedActivation = false;
	}
	TurnDetectorInternal			(false);
	StopCurrentAnimWithoutCallback	();
}

void CCustomDetector::OnMoveToSlot(const SInvItemPlace& prev)
{
	inherited::OnMoveToSlot	(prev);
}

void CCustomDetector::TurnDetectorInternal(bool b)
{
	m_bWorking				= b;
	if(b && m_ui==NULL)
	{
		CreateUI			();
	}else
	{
//.		xr_delete			(m_ui);
	}

	UpdateNightVisionMode	(b);
}



#include "game_base_space.h"
void CCustomDetector::UpdateNightVisionMode(bool b_on)
{
}

void CCustomDetector::save(NET_Packet& output_packet)
{
	inherited::save(output_packet);
	save_data(m_fCurrentChargeLevel, output_packet);

}

void CCustomDetector::load(IReader& input_packet)
{
	inherited::load(input_packet);
	load_data(m_fCurrentChargeLevel, input_packet);
}

void CCustomDetector::UpdateChargeLevel(void)
{
	if (IsWorking())
	{
		float uncharge_coef = (m_fUnchargeSpeed / 16) * Device.fTimeDelta;

		m_fCurrentChargeLevel -= uncharge_coef;

		float condition = 1.f * m_fCurrentChargeLevel;
		SetCondition(condition);

		clamp(m_fCurrentChargeLevel, 0.f, m_fMaxChargeLevel);
		SetCondition(m_fCurrentChargeLevel);
	}
}

float CCustomDetector::GetUnchargeSpeed() const
{
	return m_fUnchargeSpeed;
}

float CCustomDetector::GetCurrentChargeLevel() const
{
	return m_fCurrentChargeLevel;
}

void CCustomDetector::SetCurrentChargeLevel(float val)
{
	m_fCurrentChargeLevel = val;
	clamp(m_fCurrentChargeLevel, 0.f, m_fMaxChargeLevel);
	float condition = 1.f * m_fCurrentChargeLevel / m_fUnchargeSpeed;
	SetCondition(condition);
}

void CCustomDetector::Recharge(float val)
{
	m_fCurrentChargeLevel += val;
	clamp(m_fCurrentChargeLevel, 0.f, m_fMaxChargeLevel);

	SetCondition(m_fCurrentChargeLevel);
}

BOOL CAfList::feel_touch_contact	(CObject* O)
{
	TypesMapIt it				= m_TypesMap.find(O->cNameSect());

	bool res					 = (it!=m_TypesMap.end());
	if(res)
	{
		CArtefact*	pAf				= smart_cast<CArtefact*>(O);
		
		if(pAf->GetAfRank()>m_af_rank)
			res = false;
	}
	return						res;
}
