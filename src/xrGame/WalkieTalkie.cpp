#include "StdAfx.h"
#include "WalkieTalkie.h"
#include "Inventory.h"
#include "Actor.h"
#include "UIWalkieTalkie.h"
#include "HUDManager.h"
#include "UIGameCustom.h"
#include "player_hud.h"
#include "Weapon.h"
#include <game_cl_freemp.h>
#include "VoiceChat.h"
#include "GameObject.h"

CWalkieTalkie::CWalkieTalkie(void)
{
	m_bRadioEnabled = false;
	m_bRadioInHand = false;
	CurrentHZ = 0;
	MinHZ = 0;
	MaxHZ = 100;
	MaxDistance = 0.f;
	pUIWalkieTalkie = nullptr;
	m_bNeedActivation = false;
	m_sFactor = 0.f;
}

CWalkieTalkie::~CWalkieTalkie(void)
{
	if (OnClient())
		xr_delete(pUIWalkieTalkie);
}

BOOL CWalkieTalkie::net_Spawn(CSE_Abstract* DC)
{
	inherited::net_Spawn(DC);
	CurrentActor = 0;
	CurrentInvOwner = 0;

	CSE_ALifeItemWalkieTalkie* pSE_WT = smart_cast<CSE_ALifeItemWalkieTalkie*>(DC);
	if (pSE_WT)
	{
		MinHZ = pSE_WT->MinFreq;
		MaxHZ = pSE_WT->MaxFreq;
		MaxDistance = pSE_WT->MaxDistance;
		SetState(pSE_WT->State);
		SetNextState(pSE_WT->State);
	}

	return TRUE;
}

void CWalkieTalkie::Load(LPCSTR section)
{
	inherited::Load(section);

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
		pUIWalkieTalkie = xr_new<UIWalkieTalkie>(this);
		pUIWalkieTalkie->Init();
		pUIWalkieTalkie->Show(false);
	}
}

void CWalkieTalkie::OnEvent(NET_Packet& P, u16 type)
{
	switch (type)
	{
	case GE_PLAYER_SET_RADIO_HZ:
	{
		P.r_u16(CurrentHZ);
	}break;
	default:
		inherited::OnEvent(P, type);
		break;
	}
}

void CWalkieTalkie::net_Export(NET_Packet& P)
{
	inherited::net_Export(P);
	P.w_u16(CurrentHZ);
	P.w_u8(GetState());
}

void CWalkieTalkie::net_Import(NET_Packet& P)
{
	inherited::net_Import(P);
	CurrentHZ = P.r_u16();
	P.r_u8();
}

void CWalkieTalkie::OnH_A_Chield()
{
	CurrentActor = smart_cast<CActor*>(H_Parent());
	CurrentInvOwner = smart_cast<CInventoryOwner*>(H_Parent());
	inherited::OnH_A_Chield();
}

void CWalkieTalkie::SetRadioHZ(u16 hz)
{
	clamp(hz, MinHZ, MaxHZ);

	m_sounds.PlaySound("sndFreq", Fvector().set(0, 0, 0), this, true, false);
	CurrentHZ = hz;

	NET_Packet P;
	u_EventGen(P, GE_PLAYER_SET_RADIO_HZ, ID());
	P.w_u16(CurrentHZ);
	u_EventSend(P);
}

void CWalkieTalkie::UpdateHudAdditional(Fmatrix& trans)
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

void CWalkieTalkie::OnH_B_Independent(bool just_before_destroy)
{
	inherited::OnH_B_Independent(just_before_destroy);

	SetPending(FALSE);
	SwitchState(eHidden);
	TakeOff();
	CurrentActor = 0;
	CurrentInvOwner = 0;
	g_player_hud->detach_item(this);
}

void CWalkieTalkie::OnHiddenItem()
{
	SwitchState(eHidden);
}

void CWalkieTalkie::OnActiveItem()
{
	SwitchState(eShowing);
}

BOOL CWalkieTalkie::net_Relevant()
{
	return TRUE;
}

void CWalkieTalkie::ShowUI(bool show)
{
	if (show)
	{
		if (OnClient())
		{
			CurrentGameUI()->HideActorMenu();
			pUIWalkieTalkie->ShowDialog(true);
		}
	}
	else
	{
		if (OnClient())
			if (pUIWalkieTalkie->IsShown())
				pUIWalkieTalkie->HideDialog();
	}
}



void CWalkieTalkie::OnStateSwitch(u32 S)
{
	inherited::OnStateSwitch(S);

	if (OnServer() || (H_Parent() == Level().CurrentControlEntity()))
	{
		switch (S)
		{
		case eShowing:
		{
			g_player_hud->attach_item(this);
			PlayHUDMotion("anm_show", true, this, GetState());
			m_sounds.PlaySound("sndShow", Fvector().set(0, 0, 0), this, true, false);
			SetPending(TRUE);
			m_bRadioInHand = true;
			SoundTimer = 0;
		}break;
		case eHiding:
		{
			m_sounds.PlaySound("sndHide", Fvector().set(0, 0, 0), this, true, false);
			PlayHUDMotion("anm_hide", true, this, GetState());
			SetPending(TRUE);
			TakeOff();
		}break;
		case eIdle:
		{
			PlayAnimIdle();
			SetPending(FALSE);
		}break;
		case eHidden:
		{
			g_player_hud->detach_item(this);
			SetPending(FALSE);
		}break;
		}
	}
}

void CWalkieTalkie::OnAnimationEnd(u32 state)
{
	switch (state)
	{
	case eShowing:
	{
		SwitchState(eIdle);
	}break;
	case eHiding:
	{
		SwitchState(eHidden);
	}break;
	default:
		break;
	}
}

void CWalkieTalkie::SwitchState(u32 S)
{
	if (OnServer())
	{
		inherited::SwitchState(S);
	}

	if (OnClient()  && H_Parent() == Level().CurrentControlEntity())
	{
		SetNextState(S);
		OnStateSwitch(u32(S));
	}
}

void CWalkieTalkie::TakeOn()
{
	SwitchState(eShowing);
}

void CWalkieTalkie::TakeOff()
{
	SayNow = false;
	m_bRadioInHand = false;
	ShowUI(false);
}

void CWalkieTalkie::ActivateVoice(bool status)
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

void CWalkieTalkie::EnableRadio(bool status)
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

void CWalkieTalkie::OnMoveToSlot(const SInvItemPlace& prev)
{
	inherited::OnMoveToSlot(prev);
}

void CWalkieTalkie::OnMoveToRuck(const SInvItemPlace& prev)
{
	inherited::OnMoveToRuck(prev);

	m_bRadioEnabled = false;
	TakeOff();
	if (OnClient())
	{
		ShowUI(false);
	}
}

void CWalkieTalkie::shedule_Update(u32 dt)
{
	inherited::shedule_Update(dt);
	if (!IsInHand())
		return;
	if (!H_Parent())
		return;
}

void CWalkieTalkie::UpdateCL()
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

	if (!CurrentActor->g_Alive())
	{
		TakeOff();
		m_bRadioEnabled = false;
	}

	if (m_bRadioEnabled)
	{
		if (ActiveSnd && SoundTimer <= Device.dwTimeGlobal)
		{
			if (UserSayInRadio)
			{
				SoundTimer = Device.dwTimeGlobal + (IdleSound.get_length_sec() * 1000);

				Fvector pos = { 0,0,0 };
				float volume = 0.1f;
				IdleSound.play_no_feedback(this, sm_2D, 0.f, &pos, &volume);
				UserSayInRadio = false;
			}
		}
	}
}