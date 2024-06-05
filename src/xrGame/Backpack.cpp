#include "StdAfx.h"
#include "Backpack.h"
#include <Level.h>
#include "Actor.h"
#include <UIGameCustom.h>
#include "ui/UIActorMenu.h"

void CBackpack::Load(LPCSTR section)
{
    inherited::Load(section);
    m_additional_weight = pSettings->r_float(section, "additional_inventory_weight");
    m_iInventoryCapacity = pSettings->r_u32(section, "inventory_capacity");

	m_sounds.LoadSound(section, "snd_draw", "sndShow");
	m_sounds.LoadSound(section, "snd_holster", "sndHide");
	toggle_offsets[0] = READ_IF_EXISTS(pSettings, r_fvector3, section, "toggle_offset_pos", Fvector().set(0.f, 0.f, 0.f));
	toggle_offsets[1] = READ_IF_EXISTS(pSettings, r_fvector3, section, "toggle_offset_rot", Fvector().set(0.f, 0.f, 0.f));
}

float CBackpack::AdditionalInventoryWeight()
{
    return m_additional_weight;
}

void CBackpack::OnH_A_Chield()
{
    inherited::OnH_A_Chield();
}

void CBackpack::OnH_B_Independent(bool just_before_destroy)
{
    inherited::OnH_B_Independent(just_before_destroy);
    SwitchState(eHidden);
    SetPending(FALSE);
    g_player_hud->detach_item(this);
}

void CBackpack::OnHiddenItem()
{
    SwitchState(eHidden);
	if (Level().CurrentControlEntity() == H_Parent())
		CurrentGameUI()->HideActorMenu();
}

void CBackpack::OnActiveItem()
{
    SwitchState(eShowing);
}

bool CBackpack::Action(u16 cmd, u32 flags)
{
	if (inherited::Action(cmd, flags)) return true;

	if (IsPending()) return false;

	if (Level().CurrentControlEntity() != H_Parent())
		return false;

	switch (cmd)
	{
	case kWPN_FIRE:
	{
		if (flags & CMD_START)
		{
			CActor* pA = smart_cast<CActor*>(Level().CurrentControlEntity());
			if (pA && !pA->inventory_disabled())
			{
				CurrentGameUI()->ShowActorMenu();
				return true;
			}
		}
	}break;
	case kWPN_ZOOM:
	{
		if (flags & CMD_START)
		{
			CActor* pA = smart_cast<CActor*>(Level().CurrentControlEntity());
			if (pA && !pA->inventory_disabled())
			{
				CurrentGameUI()->ShowActorMenu();
				return true;
			}
		}
	}break;
	default:
		break;
	}
	return false;
}

void CBackpack::OnStateSwitch(u32 S)
{
    inherited::OnStateSwitch(S);

    if (OnServer() || H_Parent() == Level().CurrentControlEntity())
    {
        switch (S)
		{
		case eShowing:
		{
			g_player_hud->attach_item(this);
			PlayHUDMotion("anm_show", true, this, GetState());
			m_sounds.PlaySound("sndShow", Fvector().set(0, 0, 0), this, true, false);
			Actor()->add_cam_effector("itemuse_anm_effects\\backpack_open.anm", 8555, false, "");
			SetPending(TRUE);
		}break;
		case eHiding:
		{
			if (H_Parent() == Level().CurrentControlEntity())
			{
				CurrentGameUI()->HideActorMenu();
			}
			m_sounds.PlaySound("sndHide", Fvector().set(0, 0, 0), this, true, false);
			PlayHUDMotion("anm_hide", true, this, GetState());
			Actor()->add_cam_effector("itemuse_anm_effects\\backpack_open.anm", 8555, false, "");
			SetPending(TRUE);
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

void CBackpack::OnAnimationEnd(u32 state)
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

void CBackpack::SwitchState(u32 S)
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

void CBackpack::OnMoveToSlot(const SInvItemPlace& prev)
{
	inherited::OnMoveToSlot(prev);
}

void CBackpack::OnMoveToRuck(const SInvItemPlace& prev)
{
	//inherited::OnMoveToRuck(prev);
	SwitchState(eHidden);
}

void CBackpack::shedule_Update(u32 dt)
{
	inherited::shedule_Update(dt);
}

void CBackpack::UpdateCL()
{
	inherited::UpdateCL();
}

void CBackpack::UpdateXForm()
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
void CBackpack::UpdateHudAdditional(Fmatrix& trans)
{
	if (!H_Parent())
		return;

	if (H_Parent() != Level().CurrentControlEntity())
		return;

	if (OnServer())
		return;

	Fvector						curr_offs, curr_rot;
	curr_offs = toggle_offsets[0];//pos,aim
	curr_rot = toggle_offsets[1];//rot,aim
	curr_offs.mul(m_sFactor);
	curr_rot.mul(m_sFactor);

	if (CurrentGameUI()->ActorMenu().IsShown())
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

void CBackpack::renderable_Render()
{
	inherited::renderable_Render();
}
