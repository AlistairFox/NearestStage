#include "StdAfx.h"
#include "RadioItem.h"
#include "Inventory.h"
#include "Actor.h"
#include "UIRadioItem.h"
#include "HUDManager.h"
#include "UIGameCustom.h"
#include "player_hud.h"
#include "Weapon.h"
#include <game_cl_freemp.h>
#include "VoiceChat.h"

CRadioItem::CRadioItem(void)
{
	m_bRadioEnabled = false;
	m_bRadioInHand = false;
	CurrentHZ = 0;
	MinHZ = 0;
	MaxHZ = 100;
	pUIRadioItem = nullptr;
	m_bNeedActivation = false;
	m_sFactor = 0.f;
}

CRadioItem::~CRadioItem(void)
{
	if (OnClient())
		xr_delete(pUIRadioItem);
}

BOOL CRadioItem::net_Spawn(CSE_Abstract* DC)
{
	CurrentActor = 0;
	CurrentInvOwner = 0;
	return (inherited::net_Spawn(DC));
}

void CRadioItem::Load(LPCSTR section)
{
	inherited::Load(section);

	MinHZ = READ_IF_EXISTS(pSettings, r_u16, section, "minimal_hz", 0.f);
	MaxHZ = READ_IF_EXISTS(pSettings, r_u16, section, "maximum_hz", 100.f);
	MaxDistance = READ_IF_EXISTS(pSettings, r_float, section, "maximal_distance", 5000);


	m_sounds.LoadSound(section, "snd_draw", "sndShow");
	m_sounds.LoadSound(section, "snd_holster", "sndHide");
	m_sounds.LoadSound(section, "snd_activate", "sndAct");
	m_sounds.LoadSound(section, "snd_deactivate", "sndDeAct");
	m_sounds.LoadSound(section, "snd_freq_set", "sndFreq");
	m_sounds.LoadSound(section, "snd_enable", "sndEn");
	m_sounds.LoadSound(section, "snd_disable", "sndDis");


	toggle_offsets[0] = READ_IF_EXISTS(pSettings, r_fvector3, section, "toggle_offset_pos", Fvector().set(0.f, 0.f, 0.f));
	toggle_offsets[1] = READ_IF_EXISTS(pSettings, r_fvector3, section, "toggle_offset_rot", Fvector().set(0.f, 0.f, 0.f));
	if (pSettings->line_exist(section, "idle_sound"))
	{
		ActiveSnd = true;
		m_sounds.LoadSound(section, "idle_sound", "sndIdleF");
		idle_snd = pSettings->r_string(section, "idle_sound");
		IdleSound.create(idle_snd, st_Effect, sg_SourceType);
	}

	if (OnClient())
	{
		pUIRadioItem = xr_new<UIRadioItem>(this);
		pUIRadioItem->Init();
		pUIRadioItem->Show(false);
	}
}

void CRadioItem::OnEvent(NET_Packet& P, u16 type)
{
	switch (type)
	{
	default:
		break;
	}
}

void CRadioItem::net_Export(NET_Packet& P)
{
	inherited::net_Export(P);
}

void CRadioItem::net_Import(NET_Packet& P)
{
	inherited::net_Import(P);
}

void CRadioItem::OnH_A_Chield()
{
	CurrentActor = smart_cast<CActor*>(H_Parent());
	CurrentInvOwner = smart_cast<CInventoryOwner*>(H_Parent());
	inherited::OnH_A_Chield();
}

void CRadioItem::SetRadioHZ(u16 hz)
{
	clamp(hz, MinHZ, MaxHZ);

	m_sounds.PlaySound("sndFreq", Fvector().set(0, 0, 0), this, true, false);
	CurrentHZ = hz;

	NET_Packet P;
	Game().u_EventGen(P, GE_PLAYER_SET_RADIO_HZ, 0);
	P.w_u16(CurrentHZ);
	Game().u_EventSend(P);
}

void CRadioItem::UpdateHudAdditional(Fmatrix& trans)
{

	Fvector						curr_offs, curr_rot;
	curr_offs = toggle_offsets[0];//pos,aim
	curr_rot = toggle_offsets[1];//rot,aim
	curr_offs.mul(m_sFactor);
	curr_rot.mul(m_sFactor);

	if (SayNow)
		m_sFactor += Device.fTimeDelta / 0.3;
	else
		m_sFactor -= Device.fTimeDelta / 0.3;

	Fmatrix						hud_rotation;
	hud_rotation.identity();
	hud_rotation.rotateX(curr_rot.x);

	Fmatrix						hud_rotation_y;
	hud_rotation_y.identity();
	hud_rotation_y.rotateY(curr_rot.y);
	hud_rotation.mulA_43(hud_rotation_y);

	hud_rotation_y.identity();
	hud_rotation_y.rotateZ(curr_rot.z);
	hud_rotation.mulA_43(hud_rotation_y);

	hud_rotation.translate_over(curr_offs);
	trans.mulB_43(hud_rotation);

	clamp(m_sFactor, 0.f, 1.f);
}

void CRadioItem::OnH_B_Independet(bool just_before_destroy)
{
	inherited::OnH_B_Independent(just_before_destroy);

	CurrentActor = 0;
	CurrentInvOwner = 0;
}

void CRadioItem::OnHiddenItem()
{
}

void CRadioItem::OnActiveItem()
{
	return;
}

void CRadioItem::UpdateXForm()
{
	CInventoryItem::UpdateXForm();
}

void CRadioItem::ShowUI(bool show)
{
	if (show)
	{
		if (OnClient())
		{
			CurrentGameUI()->HideActorMenu();
			pUIRadioItem->ShowDialog(true);
		}
	}
	else
	{
		if (OnClient())
			if (pUIRadioItem->IsShown())
				pUIRadioItem->HideDialog();
	}
}



void CRadioItem::OnStateSwitch(u32 S)
{
	inherited::OnStateSwitch(S);
	switch (S)
	{
	case eShowing:
	{
		g_player_hud->attach_item(this);
		PlayHUDMotion("anm_show", FALSE, this, GetState());
		m_sounds.PlaySound("sndShow", Fvector().set(0, 0, 0), this, true, false);
		SetPending(TRUE);
	}break;
	case eHiding:
	{
		m_sounds.PlaySound("sndHide", Fvector().set(0, 0, 0), this, true, false);
		PlayHUDMotion("anm_hide", FALSE, this, GetState());
		SetPending(TRUE);
	}break;
	case eIdle:
	{
		PlayAnimIdle();
		SetPending(FALSE);
	}break;
	default:
		break;
	}
}

void CRadioItem::OnAnimationEnd(u32 state)
{
	inherited::OnAnimationEnd(state);
	switch (state)
	{
	case eShowing:
	{
		SwitchState(eIdle);
	}break;
	case eHiding:
	{
		SwitchState(eHidden);
		//TurnOff();
		g_player_hud->detach_item(this);
	}
	default:
		break;
	}
}

void CRadioItem::SwitchState(u32 S)
{
	if (OnServer())
	{
		inherited::SwitchState(S);
		return;
	}

	if (OnClient())
	{
		SetNextState(S);
		OnStateSwitch(S);

		switch (S)
		{
		case eHidden:
		{

		}break;
		case eShowing:
		case eIdle:
		{

		}break;
		default:
			break;
		}
	}
}

void CRadioItem::TakeOn()
{
	m_bNeedActivation = false;

	if (GetState() == eHidden)
	{
		if (m_pInventory->ActiveItem())
		{
			if (OnServer())
			{
				// ѕытаемс€ достать допустимый предмет: нож, оружие или тп
				// при этом будет спр€тано текущее оружие
				m_pInventory->Activate(NO_ACTIVE_SLOT);
			}
			else
			{
				if (H_Parent() && H_Parent() == Level().CurrentViewEntity())
				{
					Msg("ActivateSlot");
					NET_Packet						P;
					CGameObject::u_EventGen(P, GEG_PLAYER_ACTIVATE_SLOT, H_Parent()->ID());
					P.w_u16(NO_ACTIVE_SLOT);
					CGameObject::u_EventSend(P);
				}
			}
			m_bNeedActivation = true;
		}
		else
		{
			SwitchState(eShowing);
			m_bRadioInHand = true;
		}
	}

	SoundTimer = 0;
}

void CRadioItem::TakeOff()
{
	if (m_bRadioInHand)
		SwitchState(eHiding);

	SayNow = false;
	m_bRadioInHand = false;
	ShowUI(false);

}

void CRadioItem::ActivateVoice(bool status)
{
	game_cl_freemp* fmp = smart_cast<game_cl_freemp*>(Level().game);
	if (fmp)
	{
		if (status)
		{
			if (!m_bRadioEnabled)
				return;

			SayNow = true;
			m_sounds.PlaySound("sndAct", Fvector().set(0, 0, 0), this, true, false);
			fmp->m_pVoiceChat->Start();
		}
		else
		{
			SayNow = false;
			m_sounds.PlaySound("sndDeAct", Fvector().set(0, 0, 0), this, true, false);
			if (fmp->m_pVoiceChat->IsStarted())
				fmp->m_pVoiceChat->Stop();
		}
	}
}

void CRadioItem::EnableRadio(bool status)
{
	if (status)
	{
		m_sounds.PlaySound("sndEn", Fvector().set(0, 0, 0), this, true, false);
		m_bRadioEnabled = true;
	}
	else
	{
		m_sounds.PlaySound("sndDis", Fvector().set(0, 0, 0), this, true, false);
		m_bRadioEnabled = false;
	}
}

void CRadioItem::OnMoveToSlot(const SInvItemPlace& prev)
{
	inherited::OnMoveToSlot(prev);
}

void CRadioItem::OnMoveToRuck(const SInvItemPlace& prev)
{
	inherited::OnMoveToRuck(prev);

	m_bRadioEnabled = false;
	TakeOff();
	if (OnClient())
	{
		ShowUI(false);
	}
}

void CRadioItem::shedule_Update(u32 dt)
{
	inherited::shedule_Update(dt);
	if (!IsInHand())
		return;
	if (!H_Parent())
		return;
}

void CRadioItem::UpdateCL()
{
	inherited::UpdateCL();
	if (!H_Parent())
		return;

	if (!Actor())
		return;

	if (!CurrentActor)
		return;

	if (CurrentActor != smart_cast<CActor*>(Level().CurrentControlEntity()))
		return;

	if (m_bNeedActivation && !m_pInventory->ActiveItem())
	{
		TakeOn();
	}

	if (IsInHand())
	{
		if (OnClient())
		{
			u16 curSlot = m_pInventory->GetActiveSlot();
			if (curSlot != NO_ACTIVE_SLOT)
			{
				PIItem pItm = m_pInventory->ItemFromSlot(m_pInventory->GetNextActiveSlot());
				if (pItm)
				{
					if (GetState() == eIdle || GetState() == eShowing)
					{
						m_bNeedActivation = false;
						SwitchState(eHiding);
						return;
					}
				}
			}
		}
	}

	if (!CurrentActor->g_Alive())
	{
		TakeOff();
		m_bRadioEnabled = false;
	}

	if (m_bRadioEnabled)
	{
		if (ActiveSnd && SoundTimer <= Device.dwTimeGlobal)
		{
			SoundTimer = Device.dwTimeGlobal + (IdleSound.get_length_sec() * 1000);

			Fvector pos = { 0,0,0 };
			float volume = 0.1f;
			IdleSound.play_no_feedback(this, sm_2D, 0.f, &pos, &volume);
		}
	}
}