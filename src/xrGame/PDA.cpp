﻿#include "StdAfx.h"
#include "stdafx.h"
#include "pda.h"
#include "../xrphysics/PhysicsShell.h"
#include "Entity.h"
#include "actor.h"

#include "xrserver.h"
#include "xrServer_Objects_ALife_Items.h"
#include "level.h"

#include "specific_character.h"
#include "alife_registry_wrappers.h"
#include "../xrServerEntities/script_engine.h"
#include "player_hud.h"
#include "UIGameCustom.h"
#include "ui\UIPdaWnd.h"
#include "ai_sounds.h"
#include "Inventory.h"
#include "ActorCondition.h"

CPda::CPda(void)
{
	m_idOriginalOwner = u16(-1);
	m_SpecificChracterOwner = nullptr;
	m_bZoomed = false;
	joystick = BI_NONE;
	target_screen_switch = 0.f;
	m_fLR_CameraFactor = 0.f;
	m_fLR_MovingFactor = 0.f;
	m_fLR_InertiaFactor = 0.f;
	m_fUD_InertiaFactor = 0.f;
}

CPda::~CPda()
{}

BOOL CPda::net_Spawn(CSE_Abstract* DC)
{
	inherited::net_Spawn(DC);
	CSE_Abstract* abstract = (CSE_Abstract*)(DC);
	CSE_ALifeItemPDA* pda = smart_cast<CSE_ALifeItemPDA*>(abstract);
	R_ASSERT(pda);
	m_idOriginalOwner = pda->m_original_owner;
	m_SpecificChracterOwner = pda->m_specific_character;

	return true;
}

void CPda::net_Destroy()
{
	inherited::net_Destroy();
	feel_touch.clear();
	UpdateActiveContacts();
}

void CPda::Load(LPCSTR section)
{
	inherited::Load(section);

	m_fRadius = pSettings->r_float(section, "radius");
	m_functor_str = READ_IF_EXISTS(pSettings, r_string, section, "play_function", "");

	m_joystick_bone = READ_IF_EXISTS(pSettings, r_string, section, "joystick_bone", nullptr);
	m_sounds.LoadSound(section, "snd_draw", "sndShow", true);
	m_sounds.LoadSound(section, "snd_holster", "sndHide", true);
	m_sounds.LoadSound(section, "snd_draw_empty", "sndShowEmpty", true);
	m_sounds.LoadSound(section, "snd_holster_empty", "sndHideEmpty", true);
	m_sounds.LoadSound(section, "snd_btn_press", "sndButtonPress");
	m_sounds.LoadSound(section, "snd_btn_release", "sndButtonRelease");
	m_sounds.LoadSound(section, "snd_empty", "sndEmptyBattery", true);
	m_screen_on_delay = READ_IF_EXISTS(pSettings, r_float, section, "screen_on_delay", 0.f);
	m_screen_off_delay = READ_IF_EXISTS(pSettings, r_float, section, "screen_off_delay", 0.f);
	m_thumb_rot[0] = READ_IF_EXISTS(pSettings, r_float, section, "thumb_rot_x", 0.f);
	m_thumb_rot[1] = READ_IF_EXISTS(pSettings, r_float, section, "thumb_rot_y", 0.f);
}

void CPda::OnEvent(NET_Packet& P, u16 type)
{
	switch (type)
	{
	default:
		inherited::OnEvent(P, type);
		break;
	}
}

void CPda::SwitchState(u32 S)
{
	if (OnServer())
	{
		inherited::SwitchState(S);
	}

	if (OnClient() && H_Parent() == Level().CurrentControlEntity())
	{
		SetNextState(S);
		OnStateSwitch(u32(S));
	}
}

void CPda::OnStateSwitch(u32 S)
{
	inherited::OnStateSwitch(S);

	if (OnServer() || H_Parent() == Level().CurrentControlEntity())
	{
		switch (S)
		{
		case eShowing:
		{
			g_player_hud->attach_item(this);
			g_pGamePersistent->pda_shader_data.pda_display_factor = 0.f;

			if (!OnServer())
			{
				Msg("Play Snd");
				m_sounds.PlaySound("sndShow", Fvector().set(0, 0, 0), this, true, false);
			}

			PlayHUDMotion("anm_show", false, this, GetState());

			if (auto pda = CurrentGameUI() ? &CurrentGameUI()->PdaMenu() : nullptr)
				pda->ResetJoystick(true);

			SetPending(true);
			target_screen_switch = Device.fTimeGlobal + m_screen_on_delay;
		}
		break;
		case eHiding:
		{
			if (!OnServer())
				m_sounds.PlaySound("sndHide", Fvector().set(0, 0, 0), this, true, false);
			PlayHUDMotion("anm_hide", true, this, GetState());
			SetPending(true);
			m_bZoomed = false;
			if (auto pda = CurrentGameUI() ? &CurrentGameUI()->PdaMenu() : nullptr)
			{
				pda->Enable(false);
				pda->ResetJoystick(false);
				//CurrentGameUI()->SetMainInputReceiver(nullptr, false);
			}
			g_player_hud->reset_thumb(false);
			if (joystick != BI_NONE && HudItemData())
				HudItemData()->m_model->LL_GetBoneInstance(joystick).reset_callback();
			target_screen_switch = Device.fTimeGlobal + m_screen_off_delay;
		}
		break;
		case eHidden:
		{
			m_bZoomed = false;
			m_fZoomfactor = 0.f;
			g_player_hud->reset_thumb(true);

			CUIPdaWnd* pda = CurrentGameUI() ? &CurrentGameUI()->PdaMenu() : nullptr;
			if (pda)
			{
				if (CurrentGameUI() && CurrentGameUI()->TopInputReceiver() == pda)
					CurrentGameUI()->SetMainInputReceiver(nullptr, false);

				if (pda->IsShown())
					pda->HideDialog();
				pda->ResetJoystick(true);
			}

			if (joystick != BI_NONE && HudItemData())
				HudItemData()->m_model->LL_GetBoneInstance(joystick).reset_callback();

			g_player_hud->detach_item(this);
			SetPending(FALSE);
		}
		break;
		case eIdle:
		{
			PlayAnimIdle();
			SetPending(false);
			if (m_joystick_bone && joystick == BI_NONE && HudItemData())
				joystick = HudItemData()->m_model->LL_BoneID(m_joystick_bone);

			if (joystick != BI_NONE && HudItemData())
			{
				CBoneInstance* bi = &HudItemData()->m_model->LL_GetBoneInstance(joystick);
				if (bi)
					bi->set_callback(bctCustom, JoystickCallback, this);
			}
		}
		break;
		}
	}
}

void CPda::OnAnimationEnd(u32 state)
{
	inherited::OnAnimationEnd(state);
	switch (state)
	{
	case eShowing:
	{
		SwitchState(eIdle);
	}
	break;
	case eHiding:
	{
		SwitchState(eHidden);
	}
	break;
	}
}

// inertion
IC float _inertion(float _val_cur, float _val_trgt, float _friction)
{
	float friction_i = 1.f - _friction;
	return _val_cur * _friction + _val_trgt * friction_i;
}

void CPda::JoystickCallback(CBoneInstance* B)
{
	CPda* Pda = static_cast<CPda*>(B->callback_param());
	if (!Pda->H_Parent() || Pda->H_Parent() != Level().CurrentControlEntity())
		return;

	CUIPdaWnd* pda = CurrentGameUI() ? &CurrentGameUI()->PdaMenu() : nullptr;
	if (!pda)
		return;

	static float fAvgTimeDelta = Device.fTimeDelta;
	fAvgTimeDelta = _inertion(fAvgTimeDelta, Device.fTimeDelta, 0.8f);

	Fvector& target = pda->target_joystickrot;
	Fvector& current = pda->joystickrot;
	float& target_press = pda->target_buttonpress;
	float& press = pda->buttonpress;

	if (!target.similar(current, .0001f))
	{
		Fvector diff;
		diff = target;
		diff.sub(current);
		diff.mul(fAvgTimeDelta / .1f);
		current.add(diff);
	}
	else
		current.set(target);

	if (!fsimilar(target_press, press, .0001f))
	{
		float prev_press = press;

		float diff = target_press;
		diff -= press;
		diff *= (fAvgTimeDelta / .1f);
		press += diff;

		if (prev_press == 0.f && press < 0.f)
			Pda->m_sounds.PlaySound("sndButtonPress", B->mTransform.c, Pda->H_Root(), !!Pda->GetHUDmode());
		else if (prev_press < -.001f && press >= -.001f)
			Pda->m_sounds.PlaySound("sndButtonRelease", B->mTransform.c, Pda->H_Root(), !!Pda->GetHUDmode());
	}
	else
		press = target_press;

	Fmatrix rotation;
	rotation.identity();
	rotation.rotateX(current.x);

	Fmatrix rotation_y;
	rotation_y.identity();
	rotation_y.rotateY(current.y);
	rotation.mulA_43(rotation_y);

	rotation_y.identity();
	rotation_y.rotateZ(current.z);
	rotation.mulA_43(rotation_y);

	rotation.translate_over(0.f, press, 0.f);

	B->mTransform.mulB_43(rotation);
}

//extern bool IsMainMenuActive();

void CPda::UpdateCL()
{
	inherited::UpdateCL();

	if (!H_Parent())
		return;

	if (H_Parent() != Level().CurrentControlEntity())
		return;

	const u32 state = GetState();
	const bool b_main_menu_is_active = (g_pGamePersistent->m_pMainMenu && g_pGamePersistent->m_pMainMenu->IsActive());

	
	const auto pda = CurrentGameUI() ? &CurrentGameUI()->PdaMenu() : nullptr;
	if (pda)
	{
		if (pda->IsShown())
		{
			// Force update PDA UI if it's disabled (no input) and check for deferred enable or zoom in.
			if (!pda->IsEnabled())
			{
				pda->Update();
				if (m_bZoomed)
					pda->Enable(true);
			}
		}
		else
		{
			// Show PDA UI if possible
			if (!b_main_menu_is_active && state != eHiding && state != eHidden)
			{
				pda->ShowDialog(false); // Don't hide indicators
				CurrentGameUI()->SetMainInputReceiver(nullptr, false);
				if (!m_bZoomed)
					pda->Enable(false);
			}
		}
	}

	if (state != eHidden)
	{
		// Adjust screen brightness (smooth)
		g_pGamePersistent->pda_shader_data.pda_displaybrightness = 1.f;

		clamp(g_pGamePersistent->pda_shader_data.pda_displaybrightness, 0.f, 1.f);
		float psy_factor;
			CActor* pA = smart_cast<CActor*>(H_Parent());
			if(pA)
			{
				psy_factor = pA->conditions().GetPsy();
			}
		// Screen "Glitch" factor
		g_pGamePersistent->pda_shader_data.pda_psy_influence = psy_factor;

		// Update Display Visibility (turn on/off)
		if (target_screen_switch < Device.fTimeGlobal)
		{
			if (state == eHiding)
				// Change screen transparency (towards 0 = not visible).
				g_pGamePersistent->pda_shader_data.pda_display_factor -= Device.fTimeDelta / .25f;
			else
				// Change screen transparency (towards 1 = fully visible).
				g_pGamePersistent->pda_shader_data.pda_display_factor += Device.fTimeDelta / .75f;
		}

		clamp(g_pGamePersistent->pda_shader_data.pda_display_factor, 0.f, 1.f);
	}

}
void CPda::OnMoveToRuck(const SInvItemPlace& prev)
{
	SwitchState(eHidden);
}

void CPda::UpdateHudAdditional(Fmatrix& trans)
{
	CActor* pActor = smart_cast<CActor*>(H_Parent());
	if (!pActor)
		return;
	attachable_hud_item* hi = HudItemData();
	R_ASSERT(hi);
	Fvector curr_offs, curr_rot;
	curr_offs = hi->m_measures.m_hands_offset[0][1];//pos,aim
	curr_rot = hi->m_measures.m_hands_offset[1][1];//rot,aim
	curr_offs.mul(m_fZoomfactor);
	curr_rot.mul(m_fZoomfactor);
	Fmatrix hud_rotation;
	hud_rotation.identity();
	hud_rotation.rotateX(curr_rot.x);
	Fmatrix hud_rotation_y;
	hud_rotation_y.identity();
	hud_rotation_y.rotateY(curr_rot.y);
	hud_rotation.mulA_43(hud_rotation_y);
	hud_rotation_y.identity();
	hud_rotation_y.rotateZ(curr_rot.z);
	hud_rotation.mulA_43(hud_rotation_y);
	hud_rotation.translate_over(curr_offs);
	trans.mulB_43(hud_rotation);
	if (m_bZoomed)
	{
		m_fZoomfactor += Device.fTimeDelta / .25f;
	}
	else
	{
		m_fZoomfactor -= Device.fTimeDelta / .25f;
	}
	clamp(m_fZoomfactor, 0.f, 1.f);
}

void CPda::UpdateXForm()
{
	if (0 == H_Parent())	return;

	// Get access to entity and its visual
	CEntityAlive* E = smart_cast<CEntityAlive*>(H_Parent());
	if (!E) return;

	if (E->cast_base_monster()) return;

	const CInventoryOwner* parent = smart_cast<const CInventoryOwner*>(E);
	if (parent && parent->use_simplified_visual())
		return;

	if (parent->attached(this))
		return;

	R_ASSERT(E);
	IKinematics* V = smart_cast<IKinematics*>	(E->Visual());
	VERIFY(V);

	// Get matrices
	u16 Bone = V->LL_BoneID(bone_name().c_str());

	V->CalculateBones();
	Fmatrix& mL = V->LL_GetTransform(Bone);
	// Calculate
	Fmatrix			mRes;
	mRes = mL;
	mRes.mulA_43(E->XFORM());
	mRes.mulB_43(offset());
	renderable.xform = mRes;
}

void CPda::OnActiveItem()
{
	SwitchState(eShowing);
}

void CPda::OnHiddenItem()
{
	SwitchState(eHidden);
}

void CPda::shedule_Update(u32 dt)
{
	inherited::shedule_Update(dt);

	if (!H_Parent()) return;

	Position().set(H_Parent()->Position());

		CEntityAlive* EA = smart_cast<CEntityAlive*>(H_Parent());
		if (!EA || !EA->g_Alive())
			return;

		if (H_Parent() == Level().CurrentControlEntity())
		{
			feel_touch_update(Position(), m_fRadius);
			UpdateActiveContacts();
		}
}

void CPda::UpdateActiveContacts()
{
	m_active_contacts.clear();
	xr_vector<CObject*>::iterator it = feel_touch.begin();
	for (;it != feel_touch.end();++it) {
		CEntityAlive* pEA = smart_cast<CEntityAlive*>(*it);
		if (!!pEA->g_Alive() && !pEA->cast_base_monster())
		{
			m_active_contacts.push_back(*it);
		}
	}
}

void CPda::feel_touch_new(CObject* O)
{
	if (CInventoryOwner* pNewContactInvOwner = smart_cast<CInventoryOwner*>(O))
	{
		CInventoryOwner* pOwner = smart_cast<CInventoryOwner*>(H_Parent());VERIFY(pOwner);
		pOwner->NewPdaContact(pNewContactInvOwner);
	}
}

void CPda::feel_touch_delete(CObject* O)
{
	if (!H_Parent())							return;
	if (CInventoryOwner* pLostContactInvOwner = smart_cast<CInventoryOwner*>(O))
	{
		CInventoryOwner* pOwner = smart_cast<CInventoryOwner*>(H_Parent());VERIFY(pOwner);
		pOwner->LostPdaContact(pLostContactInvOwner);
	}
}

BOOL CPda::feel_touch_contact(CObject* O)
{
	CEntityAlive* entity_alive = smart_cast<CEntityAlive*>(O);

	if (entity_alive && entity_alive->cast_base_monster())
	{
		return false;
	}
	else if (CInventoryOwner* pInvOwner = smart_cast<CInventoryOwner*>(O))
	{
		if (this != pInvOwner->GetPDA())
		{
			CEntityAlive* pEntityAlive = smart_cast<CEntityAlive*>(O);
			if (pEntityAlive)
				return true;
		}
		else
			return false;
	}

	return false;
}

void CPda::OnH_A_Chield()
{
	if (H_Parent()->ID() == m_idOriginalOwner) {
		if (m_sFullName.empty()) {
			m_sFullName.assign(NameItem());
			m_sFullName += " ";
			m_sFullName += (smart_cast<CInventoryOwner*>(H_Parent()))->Name();
		}
	};
	inherited::OnH_A_Chield();
}

void CPda::OnH_B_Independent(bool just_before_destroy)
{
	inherited::OnH_B_Independent(just_before_destroy);
	SwitchState(eHidden);
}


CInventoryOwner* CPda::GetOriginalOwner()
{
	CObject* pObject = Level().Objects.net_Find(GetOriginalOwnerID());
	CInventoryOwner* pInvOwner = smart_cast<CInventoryOwner*>(pObject);

	return pInvOwner;
}



void CPda::ActivePDAContacts(xr_vector<CPda*>& res)
{
	res.clear();
	xr_vector<CObject*>::iterator it = m_active_contacts.begin();
	xr_vector<CObject*>::iterator it_e = m_active_contacts.end();

	for (;it != it_e;++it)
	{
		CPda* p = GetPdaFromOwner(*it);
		if (p)
			res.push_back(p);
	}
}

void CPda::save(NET_Packet& output_packet)
{
	inherited::save(output_packet);
	save_data(m_sFullName, output_packet);
}

void CPda::load(IReader& input_packet)
{
	inherited::load(input_packet);
	load_data(m_sFullName, input_packet);
}

CObject* CPda::GetOwnerObject()
{
	return				Level().Objects.net_Find(GetOriginalOwnerID());
}

CPda* CPda::GetPdaFromOwner(CObject* owner)
{
	return smart_cast<CInventoryOwner*>(owner)->GetPDA();
}

void CPda::PlayScriptFunction()
{
	if (xr_strcmp(m_functor_str, ""))
	{
		luabind::functor<void> m_functor;
		R_ASSERT(ai().script_engine().functor(m_functor_str.c_str(), m_functor));
		m_functor();
	}
}