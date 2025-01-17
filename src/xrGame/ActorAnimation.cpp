﻿#include "stdafx.h"
#include "Actor.h"
#include "ActorAnimation.h"
#include "actor_anim_defs.h"
#include "weapon.h"
#include "inventory.h"
#include "missile.h"
#include "level.h"
#ifdef DEBUG
#include "PHDebug.h"
#include "ui_base.h"
#endif
#include "hit.h"
#include "PHDestroyable.h"
#include "Car.h"
#include "../Include/xrRender/Kinematics.h"
#include "ai_object_location.h"
#include "game_cl_base.h"
#include "../xrEngine/motion.h"
#include "artefact.h"
#include "IKLimbsController.h"
#include "player_hud.h"
#include "PDA.h"
#include "WalkieTalkie.h"
#include "Backpack.h"

static const float y_spin0_factor = 0.0f;
static const float y_spin1_factor = 0.4f;
static const float y_shoulder_factor = 0.4f;
static const float y_head_factor = 0.2f;
static const float p_spin0_factor = 0.0f;
static const float p_spin1_factor = 0.2f;
static const float p_shoulder_factor = 0.7f;
static const float p_head_factor = 0.1f;
static const float r_spin0_factor = 0.3f;
static const float r_spin1_factor = 0.3f;
static const float r_shoulder_factor = 0.2f;
static const float r_head_factor = 0.2f;

CBlend* PlayMotionByParts(IKinematicsAnimated* sa, MotionID motion_ID, BOOL bMixIn, PlayCallback Callback, LPVOID CallbackParam);

void  CActor::Spin0Callback(CBoneInstance* B)
{
	CActor* A = static_cast<CActor*>(B->callback_param());	VERIFY(A);

	Fmatrix				spin;
	float				bone_yaw = angle_normalize_signed(A->r_torso.yaw - A->r_model_yaw - A->r_model_yaw_delta) * y_spin0_factor;
	float				bone_pitch = angle_normalize_signed(A->r_torso.pitch) * p_spin0_factor;
	float				bone_roll = angle_normalize_signed(A->r_torso.roll) * r_spin0_factor;
	Fvector c = B->mTransform.c;
	spin.setXYZ(-bone_pitch, bone_yaw, bone_roll);
	B->mTransform.mulA_43(spin);
	B->mTransform.c = c;
}
void  CActor::Spin1Callback(CBoneInstance* B)
{
	CActor* A = static_cast<CActor*>(B->callback_param());	VERIFY(A);

	Fmatrix				spin;
	float				bone_yaw = angle_normalize_signed(A->r_torso.yaw - A->r_model_yaw - A->r_model_yaw_delta) * y_spin1_factor;
	float				bone_pitch = angle_normalize_signed(A->r_torso.pitch) * p_spin1_factor;
	float				bone_roll = angle_normalize_signed(A->r_torso.roll) * r_spin1_factor;
	Fvector c = B->mTransform.c;
	spin.setXYZ(-bone_pitch, bone_yaw, bone_roll);
	B->mTransform.mulA_43(spin);
	B->mTransform.c = c;
}
void  CActor::ShoulderCallback(CBoneInstance* B)
{
	CActor* A = static_cast<CActor*>(B->callback_param());	VERIFY(A);
	Fmatrix				spin;
	float				bone_yaw = angle_normalize_signed(A->r_torso.yaw - A->r_model_yaw - A->r_model_yaw_delta) * y_shoulder_factor;
	float				bone_pitch = angle_normalize_signed(A->r_torso.pitch) * p_shoulder_factor;
	float				bone_roll = angle_normalize_signed(A->r_torso.roll) * r_shoulder_factor;
	Fvector c = B->mTransform.c;
	spin.setXYZ(-bone_pitch, bone_yaw, bone_roll);
	B->mTransform.mulA_43(spin);
	B->mTransform.c = c;
}

void  CActor::HeadCallback(CBoneInstance* B)
{
	CActor* A = static_cast<CActor*>(B->callback_param());	VERIFY(A);
	Fmatrix				spin;
	float				bone_yaw = angle_normalize_signed(A->r_torso.yaw - A->r_model_yaw - A->r_model_yaw_delta) * y_head_factor;
	float				bone_pitch = angle_normalize_signed(A->r_torso.pitch) * p_head_factor;
	float				bone_roll = angle_normalize_signed(A->r_torso.roll) * r_head_factor;
	if (A->UseHelmet && (A != Level().CurrentControlEntity() || !(A->MpAnimationMODE() || A->MpWoundMODE())))
	{
		spin.scale(0.7f,0.7f,0.7f);
	}
	else
	{
		if (A && Level().CurrentControlEntity() == A && (A->MpAnimationMODE() || A->MpWoundMODE() || !A->g_Alive()))
		{
			spin.scale(0.01f, 0.01f, 0.01f);
		}
		else
		{
			spin.setXYZ(-bone_pitch, bone_yaw, bone_roll);
		}
	}

	Fvector c = B->mTransform.c;
	B->mTransform.mulA_43(spin);
	B->mTransform.c = c;
}

void  CActor::VehicleHeadCallback(CBoneInstance* B)
{
	CActor* A = static_cast<CActor*>(B->callback_param());	VERIFY(A);
	Fmatrix				spin;
	float				bone_yaw = angle_normalize_signed(A->r_torso.yaw) * 0.75f;
	float				bone_pitch = angle_normalize_signed(A->r_torso.pitch) * 0.75f;
	float				bone_roll = angle_normalize_signed(A->r_torso.roll) * r_head_factor;
	Fvector c = B->mTransform.c;
	spin.setHPB(bone_yaw, bone_pitch, -bone_roll);
	B->mTransform.mulA_43(spin);
	B->mTransform.c = c;
}

void STorsoWpn::Create(IKinematicsAnimated* K, LPCSTR base0, LPCSTR base1)
{
	char			buf[128];
	moving[eIdle] = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_aim_1"));
	moving[eWalk] = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_aim_2"));
	moving[eRun] = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_aim_3"));
	moving[eSprint] = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_escape_0"));
	zoom = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_aim_0"));
	holster = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_holster_0"));
	draw = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_draw_0"));
	reload = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_reload_0"));
	reload_1 = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_reload_1"));
	reload_2 = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_reload_2"));
	drop = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_drop_0"));
	attack = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_attack_1"));
	attack_zoom = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_attack_0"));
	fire_idle = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_attack_1"));
	fire_end = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_attack_2"));
	all_attack_0 = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_all", base1, "_attack_0"));
	all_attack_1 = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_all", base1, "_attack_1"));
	all_attack_2 = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_all", base1, "_attack_2"));
}

void SAnimState::Create(IKinematicsAnimated* K, LPCSTR base0, LPCSTR base1)
{
	char			buf[128];
	legs_fwd = K->ID_Cycle(strconcat(sizeof(buf), buf, base0, base1, "_fwd_0"));
	legs_back = K->ID_Cycle(strconcat(sizeof(buf), buf, base0, base1, "_back_0"));
	legs_ls = K->ID_Cycle(strconcat(sizeof(buf), buf, base0, base1, "_ls_0"));
	legs_rs = K->ID_Cycle(strconcat(sizeof(buf), buf, base0, base1, "_rs_0"));
}

void SActorState::CreateClimb(IKinematicsAnimated* K)
{
	string128		buf, buf1;
	string16		base;

	//climb anims
	xr_strcpy(base, "cl");
	legs_idle = K->ID_Cycle(strconcat(sizeof(buf), buf, base, "_idle_1"));
	m_torso_idle = K->ID_Cycle(strconcat(sizeof(buf), buf, base, "_torso_0_aim_0"));
	m_walk.Create(K, base, "_run");
	m_run.Create(K, base, "_run");

	//norm anims
	xr_strcpy(base, "norm");
	legs_turn = K->ID_Cycle(strconcat(sizeof(buf), buf, base, "_turn"));
	death = K->ID_Cycle(strconcat(sizeof(buf), buf, base, "_death_0"));
	m_torso[0].Create(K, base, "_1");
	m_torso[1].Create(K, base, "_2");
	m_torso[2].Create(K, base, "_3");
	m_torso[3].Create(K, base, "_4");
	m_torso[4].Create(K, base, "_5");
	m_torso[5].Create(K, base, "_6");
	m_torso[6].Create(K, base, "_7");
	m_torso[7].Create(K, base, "_8");
	m_torso[8].Create(K, base, "_9");
	m_torso[9].Create(K, base, "_10");
	m_torso[10].Create(K, base, "_11");
	m_torso[11].Create(K, base, "_12");
	m_torso[12].Create(K, base, "_13");


	m_head_idle.invalidate(); ///K->ID_Cycle("head_idle_0");
	jump_begin = K->ID_Cycle(strconcat(sizeof(buf), buf, base, "_jump_begin"));
	jump_idle = K->ID_Cycle(strconcat(sizeof(buf), buf, base, "_jump_idle"));
	landing[0] = K->ID_Cycle(strconcat(sizeof(buf), buf, base, "_jump_end"));
	landing[1] = K->ID_Cycle(strconcat(sizeof(buf), buf, base, "_jump_end_1"));

	for (int k = 0; k < 12; ++k)
		m_damage[k] = K->ID_FX(strconcat(sizeof(buf), buf, base, "_damage_", itoa(k, buf1, 10)));
}


void SActorState::Create(IKinematicsAnimated* K, LPCSTR base)
{
	string128		buf, buf1;
	legs_turn = K->ID_Cycle(strconcat(sizeof(buf), buf, base, "_turn"));
	legs_idle = K->ID_Cycle(strconcat(sizeof(buf), buf, base, "_idle_0"));
	death = K->ID_Cycle(strconcat(sizeof(buf), buf, base, "_death_0"));

	m_walk.Create(K, base, "_walk");
	m_run.Create(K, base, "_run");

	m_torso[0].Create(K, base, "_1");
	m_torso[1].Create(K, base, "_2");
	m_torso[2].Create(K, base, "_3");
	m_torso[3].Create(K, base, "_4");
	m_torso[4].Create(K, base, "_5");
	m_torso[5].Create(K, base, "_6");
	m_torso[6].Create(K, base, "_7");
	m_torso[7].Create(K, base, "_8");
	m_torso[8].Create(K, base, "_9");
	m_torso[9].Create(K, base, "_10");
	m_torso[10].Create(K, base, "_11");
	m_torso[11].Create(K, base, "_12");
	m_torso[12].Create(K, base, "_13");
	m_torso[13].Create(K, base, "_pda");

	m_torso_idle = K->ID_Cycle(strconcat(sizeof(buf), buf, base, "_torso_0_aim_0"));
	m_head_idle = K->ID_Cycle("head_idle_0");
	jump_begin = K->ID_Cycle(strconcat(sizeof(buf), buf, base, "_jump_begin"));
	jump_idle = K->ID_Cycle(strconcat(sizeof(buf), buf, base, "_jump_idle"));
	landing[0] = K->ID_Cycle(strconcat(sizeof(buf), buf, base, "_jump_end"));
	landing[1] = K->ID_Cycle(strconcat(sizeof(buf), buf, base, "_jump_end_1"));

	for (int k = 0; k < 12; ++k)
		m_damage[k] = K->ID_FX(strconcat(sizeof(buf), buf, base, "_damage_", itoa(k, buf1, 10)));
}

void SActorSprintState::Create(IKinematicsAnimated* K)
{
	//leg anims
	legs_fwd = K->ID_Cycle("norm_escape_00");
	legs_ls = K->ID_Cycle("norm_escape_ls_00");
	legs_rs = K->ID_Cycle("norm_escape_rs_00");

	legs_jump_fwd = K->ID_Cycle("norm_escape_jump_00");
	legs_jump_ls = K->ID_Cycle("norm_escape_ls_jump_00");
	legs_jump_rs = K->ID_Cycle("norm_escape_rs_jump_00");
}

//SAFE ANIMATION FUNCTIONS

void SAnimStateSafe::CreateSafe(IKinematicsAnimated* K, LPCSTR base0, LPCSTR base1)
{
	char			buf[128];
	legs_fwd = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, base1, "_fwd_1"));
	legs_back = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, base1, "_back_0"));
	legs_ls = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, base1, "_ls_0"));
	legs_rs = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, base1, "_rs_0"));
}

void SAnimStateSprintSafe::CreateSafeSprint(IKinematicsAnimated* K)
{
	legs_fwd = K->ID_Cycle("norm_escape_00");
	legs_ls = K->ID_Cycle("norm_escape_ls_00");
	legs_rs = K->ID_Cycle("norm_escape_rs_00");

	legs_jump_fwd = K->ID_Cycle("norm_escape_jump_00");
	legs_jump_ls = K->ID_Cycle("norm_escape_ls_jump_00");
	legs_jump_rs = K->ID_Cycle("norm_escape_rs_jump_00");
}

void SAnimTorsoStateSafe::CreateTorsoSafe(IKinematicsAnimated* K, LPCSTR base0, LPCSTR base1)
{
	char			buf[128];
	torso_moving_safe[eIdleSafe] = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_idle_1"));
	torso_moving_safe[eWalkSafe] = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_walk_1"));
	torso_moving_safe[eRunSafe] = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_run_1"));
	torso_moving_safe[eSprintSafe] = K->ID_Cycle_Safe(strconcat(sizeof(buf), buf, base0, "_torso", base1, "_run_1"));

}


void SAnimSafeMotions::CreateSafeAnims(IKinematicsAnimated* K, LPCSTR base)
{
	m_safe_run.CreateSafe(K, base, "_run");
	m_safe_walk.CreateSafe(K, base, "_walk");
	m_safe_sprint.CreateSafeSprint(K);

	m_safe_torso[0].CreateTorsoSafe(K, base, "_1");
	m_safe_torso[1].CreateTorsoSafe(K, base, "_2");
	m_safe_torso[2].CreateTorsoSafe(K, base, "_3");
	m_safe_torso[3].CreateTorsoSafe(K, base, "_4");
	m_safe_torso[4].CreateTorsoSafe(K, base, "_5");
	m_safe_torso[5].CreateTorsoSafe(K, base, "_6");
	m_safe_torso[6].CreateTorsoSafe(K, base, "_7");
	m_safe_torso[7].CreateTorsoSafe(K, base, "_8");
	m_safe_torso[8].CreateTorsoSafe(K, base, "_9");
	m_safe_torso[9].CreateTorsoSafe(K, base, "_10");
	m_safe_torso[10].CreateTorsoSafe(K, base, "_11");
	m_safe_torso[11].CreateTorsoSafe(K, base, "_12");
	m_safe_torso[12].CreateTorsoSafe(K, base, "_13");
	m_safe_torso[13].CreateTorsoSafe(K, base, "_pda");

	char buf[128];
	m_head_idle = K->ID_Cycle("head_idle_0");
	m_legs_idle = K->ID_Cycle(strconcat(sizeof(buf), buf, base, "_idle_1"));
	//m_torso_idle = K->ID_Cycle(strconcat(sizeof(buf), buf, base, "_torso_2_idle_1"));

}

// SAFE FUNCTS END

void SActorMotions::Create(IKinematicsAnimated* V)
{
	m_dead_stop = V->ID_Cycle("norm_dead_stop_0");

	m_normal.Create(V, "norm");
	m_crouch.Create(V, "cr");
	m_climb.CreateClimb(V);
	m_sprint.Create(V);

	// SE7KILLS
	m_script.CreateAnimationsScripted(V);
	m_script.CreateWoundAnimationScripted(V);
	m_safe_mode.CreateSafeAnims(V, "norm");
}

SActorVehicleAnims::SActorVehicleAnims()
{

}

void SActorVehicleAnims::Create(IKinematicsAnimated* V)
{
	for (u16 i = 0;TYPES_NUMBER > i;++i)
		m_vehicles_type_collections[i].Create(V, i);
}

SVehicleAnimCollection::SVehicleAnimCollection()
{
	for (u16 i = 0;MAX_IDLES > i;++i) idles[i].invalidate();
	idles_num = 0;
	steer_left.invalidate();
	steer_right.invalidate();
}

void SVehicleAnimCollection::Create(IKinematicsAnimated* V, u16 num)
{
	string128 buf, buff1, buff2;
	strconcat(sizeof(buff1), buff1, itoa(num, buf, 10), "_");
	steer_left = V->ID_Cycle(strconcat(sizeof(buf), buf, "steering_idle_", buff1, "ls"));
	steer_right = V->ID_Cycle(strconcat(sizeof(buf), buf, "steering_idle_", buff1, "rs"));

	for (int i = 0;MAX_IDLES > i;++i)
	{
		idles[i] = V->ID_Cycle_Safe(strconcat(sizeof(buf), buf, "steering_idle_", buff1, itoa(i, buff2, 10)));
		if (idles[i]) idles_num++;
		else break;
	}
}

void CActor::steer_Vehicle(float angle)
{
	if (!m_holder)		return;
	
	//	CCar*	car			= smart_cast<CCar*>(m_holder);
	//	u16 anim_type       = car->DriverAnimationType();
	//	SVehicleAnimCollection& anims=m_vehicle_anims->m_vehicles_type_collections[anim_type];
	//	if(angle==0.f) 		smart_cast<IKinematicsAnimated*>	(Visual())->PlayCycle(anims.idles[0]);
	//	else if(angle>0.f)	smart_cast<IKinematicsAnimated*>	(Visual())->PlayCycle(anims.steer_right);
	//	else				smart_cast<IKinematicsAnimated*>	(Visual())->PlayCycle(anims.steer_left);
	
}

void legs_play_callback(CBlend* blend)
{
	CActor* object = (CActor*)blend->CallbackParam;
	VERIFY(object);
	object->m_current_legs.invalidate();
}

void CActor::g_SetSprintAnimation(u32 mstate_rl, MotionID& head, MotionID& torso, MotionID& legs)
{
	SActorSprintState& sprint = m_anims->m_sprint;
	SAnimStateSprintSafe& sprint_safe = m_anims->m_safe_mode.m_safe_sprint;

	bool jump = (mstate_rl & mcFall) ||
		(mstate_rl & mcLanding) ||
		(mstate_rl & mcLanding) ||
		(mstate_rl & mcLanding2) ||
		(mstate_rl & mcJump);

	if (!MpSafeMODE())
	{
		if (mstate_rl & mcFwd)		legs = (!jump) ? sprint.legs_fwd : sprint.legs_jump_fwd;
		else if (mstate_rl & mcLStrafe) legs = (!jump) ? sprint.legs_ls : sprint.legs_jump_ls;
		else if (mstate_rl & mcRStrafe)	legs = (!jump) ? sprint.legs_rs : sprint.legs_jump_rs;
	}
	else
	{
		if (mstate_rl & mcFwd)		legs = (!jump) ? sprint_safe.legs_fwd : sprint_safe.legs_jump_fwd;
		else if (mstate_rl & mcLStrafe) legs = (!jump) ? sprint_safe.legs_ls : sprint_safe.legs_jump_ls;
		else if (mstate_rl & mcRStrafe)	legs = (!jump) ? sprint_safe.legs_rs : sprint_safe.legs_jump_rs;
	}
}

CMotion* FindMotionKeys(MotionID motion_ID, IRenderVisual* V)
{
	IKinematicsAnimated* VA = smart_cast<IKinematicsAnimated*>(V);
	return (VA && motion_ID.valid()) ? VA->LL_GetRootMotion(motion_ID) : 0;
}

#ifdef DEBUG
BOOL	g_ShowAnimationInfo = TRUE;
#endif // DEBUG
char* mov_state[] = {
	"idle",
	"walk",
	"run",
	"sprint",
};

void CActor::g_SetAnimation(u32 mstate_rl)
{
	if (!g_Alive())
	{
		if (m_current_legs || m_current_torso)
		{
			SActorState* ST = 0;
			if (mstate_rl & mcCrouch)		ST = &m_anims->m_crouch;
			else						ST = &m_anims->m_normal;
			mstate_real = 0;
			m_current_legs.invalidate();
			m_current_torso.invalidate();
		}

		return;
	}

	STorsoWpn::eMovingState	moving_idx = STorsoWpn::eIdle;
	SActorState* ST = 0;
	SAnimState* AS = 0;

	bool normal = false;

	if (mstate_rl & mcCrouch)
		ST = &m_anims->m_crouch;
	else if (mstate_rl & mcClimb)
		ST = &m_anims->m_climb;
	else
	{
		ST = &m_anims->m_normal;
		normal = true;
	}

	SAnimSafeMotions* ST_Safe = &m_anims->m_safe_mode;
	SAnimStateSafe* ST_Moving;

	bool bAccelerated = isActorAccelerated(mstate_rl, IsZoomAimingMode());
	// ��������
	MotionID 						M_legs;
	MotionID 						M_torso;
	MotionID 						M_head;
	//���� �� ������ ����� �� �����
	bool is_standing = false;

	// Legs
	if (!MpSafeMODE() || (mstate_rl & mcCrouch || mstate_rl & mcClimb))
	{
		if (bAccelerated)
			AS = &ST->m_run;
		else
			AS = &ST->m_walk;

		if (mstate_rl & mcAnyMove)
		{
			if (bAccelerated)
				moving_idx = STorsoWpn::eRun;
			else
				moving_idx = STorsoWpn::eWalk;
		}

		if (mstate_rl & mcLanding)
			M_legs = ST->landing[0];
		else if (mstate_rl & mcLanding2)
			M_legs = ST->landing[1];
		else if ((mstate_rl & mcTurn) && !(mstate_rl & mcClimb))
			M_legs = ST->legs_turn;
		else if (mstate_rl & mcFall)
			M_legs = ST->jump_idle;
		else if (mstate_rl & mcJump)
			M_legs = ST->jump_begin;

		else if (mstate_rl & mcFwd)
			M_legs = AS->legs_fwd;
		else if (mstate_rl & mcBack)
			M_legs = AS->legs_back;
		else if (mstate_rl & mcLStrafe)
			M_legs = AS->legs_ls;
		else if (mstate_rl & mcRStrafe)
			M_legs = AS->legs_rs;
		else
			is_standing = true;
	}
	else
	{
		if (bAccelerated)
			ST_Moving = &ST_Safe->m_safe_run;
		else
			ST_Moving = &ST_Safe->m_safe_walk;

		if (mstate_rl & mcAnyMove)
		{
			if (bAccelerated)
				moving_idx = STorsoWpn::eRun;
			else
				moving_idx = STorsoWpn::eWalk;
		}

		if (mstate_rl & mcLanding)
			M_legs = ST->landing[0];
		else if (mstate_rl & mcLanding2)
			M_legs = ST->landing[1];
		else if ((mstate_rl & mcTurn) && !(mstate_rl & mcClimb))
			M_legs = ST->legs_turn;
		else if (mstate_rl & mcFall)
			M_legs = ST->jump_idle;
		else if (mstate_rl & mcJump)
			M_legs = ST->jump_begin;

		if (mstate_rl & mcFwd)
			M_legs = ST_Moving->legs_fwd;
		else if (mstate_rl & mcBack)
			M_legs = ST_Moving->legs_back;
		else if (mstate_rl & mcLStrafe)
			M_legs = ST_Moving->legs_ls;
		else if (mstate_rl & mcRStrafe)
			M_legs = ST_Moving->legs_rs;
		else
			is_standing = true;
	}


	if (mstate_rl & mcSprint)
	{
		g_SetSprintAnimation(mstate_rl, M_head, M_torso, M_legs);
		moving_idx = STorsoWpn::eSprint;
	}

	if (this == Level().CurrentViewEntity())
	{
		if ((mstate_rl & mcSprint) != (mstate_old & mcSprint))
		{
			g_player_hud->OnMovementChanged(mcSprint);
		}
		else
			if ((mstate_rl & mcAnyMove) != (mstate_old & mcAnyMove))
			{
				g_player_hud->OnMovementChanged(mcAnyMove);
			}
	};


	//-----------------------------------------------------------------------
	// Torso

	if (mstate_rl & mcClimb)
	{
		if (mstate_rl & mcFwd)		M_torso = AS->legs_fwd;
		else if (mstate_rl & mcBack)		M_torso = AS->legs_back;
		else if (mstate_rl & mcLStrafe)	M_torso = AS->legs_ls;
		else if (mstate_rl & mcRStrafe)	M_torso = AS->legs_rs;
	}


	if (Level().CurrentControlEntity() == this)
	{
		if (MpAnimationMODE() || NEED_EXIT)
		{
			m_bAnimTorsoPlayed = false;
			m_current_torso.invalidate();
			SelectScriptAnimation();
			return;
		}
	}
	else
	{
		if (MpAnimationMODE())
			return;
	}

	if (Level().CurrentControlEntity() == this)
	{
		if (MpWoundMODE() || NEED_WOUND_EXIT)
		{
			m_bAnimTorsoPlayed = false;
			m_current_torso.invalidate();
			SelectScriptWoundAnimation();
			return;
		}
	}
	else
	{
		if (MpWoundMODE())
			return;
	}

	InputAnim = 0;
	OutAnim = 0;
	MidAnim = 0;
	InputWoundAnim = 0;
	OutWoundAnim = 0;
	MidWoundAnim = 0;

	if (!M_torso)
	{
		CInventoryItem* _i = inventory().ActiveItem();
		CHudItem* H = smart_cast<CHudItem*>(_i);
		CWeapon* W = smart_cast<CWeapon*>(_i);
		CMissile* M = smart_cast<CMissile*>(_i);
		CArtefact* A = smart_cast<CArtefact*>(_i);
		CPda* P = smart_cast<CPda*>(_i);
		CWalkieTalkie* Walkie = smart_cast<CWalkieTalkie*>(_i);
		CBackpack* BackPack = smart_cast<CBackpack*>(_i);

		if (!MpSafeMODE())
		{
			if (H)
			{
				VERIFY(H->animation_slot() <= _total_anim_slots_);
				STorsoWpn* TW = &ST->m_torso[H->animation_slot() - 1];
				if (!b_DropActivated && !fis_zero(f_DropPower))
				{
					M_torso = TW->drop;
					if (!M_torso)
					{
						Msg("! drop animation for %s", *(H->object().cName()));
						M_torso = ST->m_torso_idle;
					};
					m_bAnimTorsoPlayed = TRUE;
				}
				else
				{
					if (!m_bAnimTorsoPlayed)
					{
						if (W)
						{
							bool K = inventory().GetActiveSlot() == KNIFE_SLOT;
							bool R3 = W->IsTriStateReload();

							if (K)
							{
								switch (W->GetState())
								{
								case CWeapon::eIdle:		M_torso = TW->moving[moving_idx];		break;

								case CWeapon::eFire:
									if (is_standing)
										M_torso = M_legs = M_head = TW->all_attack_0;
									else
										M_torso = TW->attack_zoom;
									break;

								case CWeapon::eFire2:
									if (is_standing)
										M_torso = M_legs = M_head = TW->all_attack_1;
									else
										M_torso = TW->fire_idle;
									break;

								case CWeapon::eReload:		M_torso = TW->reload;					break;
								case CWeapon::eShowing:		M_torso = TW->draw;						break;
								case CWeapon::eHiding:		M_torso = TW->holster;					break;
								default:  	M_torso = TW->moving[moving_idx];		break;
								}
							}
							else
							{
								switch (W->GetState())
								{
								case CWeapon::eIdle:		M_torso = W->IsZoomed() ? TW->zoom : TW->moving[moving_idx];	break;
								case CWeapon::eFire:		M_torso = W->IsZoomed() ? TW->attack_zoom : TW->attack;				break;
								case CWeapon::eFire2:		M_torso = W->IsZoomed() ? TW->attack_zoom : TW->attack;				break;
								case CWeapon::eReload:
									if (!R3)
										M_torso = TW->reload;
									else
									{
										CWeapon::EWeaponSubStates sub_st = W->GetReloadState();
										switch (sub_st)
										{
										case CWeapon::eSubstateReloadBegin:			M_torso = TW->reload;	break;
										case CWeapon::eSubstateReloadInProcess:		M_torso = TW->reload_1; break;
										case CWeapon::eSubstateReloadEnd:			M_torso = TW->reload_2; break;
										default:									M_torso = TW->reload;	break;
										}
									}break;

								case CWeapon::eShowing:	M_torso = TW->draw;					break;
								case CWeapon::eHiding:	M_torso = TW->holster;				break;
								default:  M_torso = TW->moving[moving_idx];	break;
								}
							}
						}
						else if (M)
						{
							if (is_standing)
							{
								switch (M->GetState())
								{
								case CMissile::eShowing:		M_torso = TW->draw;			break;
								case CMissile::eHiding:		M_torso = TW->holster;		break;
								case CMissile::eIdle:		M_torso = TW->moving[moving_idx];		break;
								case CMissile::eThrowStart:		M_torso = M_legs = M_head = TW->all_attack_0;	break;
								case CMissile::eReady:		M_torso = M_legs = M_head = TW->all_attack_1;	break;
								case CMissile::eThrow:		M_torso = M_legs = M_head = TW->all_attack_2;	break;
								case CMissile::eThrowEnd:		M_torso = M_legs = M_head = TW->all_attack_2;	break;
								default:		M_torso = TW->draw;			break;
								}
							}
							else
							{
								switch (M->GetState())
								{
								case CMissile::eShowing:		M_torso = TW->draw;						break;
								case CMissile::eHiding:		M_torso = TW->holster;					break;
								case CMissile::eIdle:		M_torso = TW->moving[moving_idx];		break;
								case CMissile::eThrowStart:		M_torso = TW->attack_zoom;				break;
								case CMissile::eReady:		M_torso = TW->fire_idle;				break;
								case CMissile::eThrow:		M_torso = TW->fire_end;					break;
								case CMissile::eThrowEnd:		M_torso = TW->fire_end;					break;
								default:		M_torso = TW->draw;						break;
								}
							}
						}
						else if (A)
						{
							switch (A->GetState())
							{
							case CArtefact::eIdle: M_torso = TW->moving[moving_idx];	break;
							case CArtefact::eShowing: M_torso = TW->draw;					break;
							case CArtefact::eHiding: M_torso = TW->holster;				break;
							case CArtefact::eActivating: M_torso = TW->zoom;					break;
							default: M_torso = TW->moving[moving_idx];
							}

						}
						else if (P)
						{
							switch (P->GetState())
							{
							case CPda::eIdle: M_torso = moving_idx == STorsoWpn::eSprint ? ST->m_torso[0].moving[moving_idx] : TW->zoom;
								break;
							case CPda::eShowing: M_torso = TW->draw;
								break;
							case CPda::eHiding: M_torso = TW->holster;
								break;
							default: M_torso = ST->m_torso[4].moving[moving_idx];
								break;
							}
						}
						else if (Walkie)
						{
							switch (Walkie->GetState())
							{
							case CWalkieTalkie::eIdle: M_torso = TW->moving[moving_idx];	break;
							case CWalkieTalkie::eShowing: M_torso = TW->draw;					break;
							case CWalkieTalkie::eHiding: M_torso = TW->holster;				break;
							default: M_torso = TW->moving[moving_idx];
							}
							
						}
						else if (BackPack)
						{
							switch (BackPack->GetState())
							{
							case CBackpack::eIdle: M_torso = TW->moving[moving_idx]; break;
							case CBackpack::eShowing: M_torso = TW->draw; break;
							case CBackpack::eHiding: M_torso = TW->holster; break;
							default: M_torso = TW->moving[moving_idx];
							}
						}
					}
				}
			}
		}
		else
		{
			if (H)
			{
				if (!m_bAnimTorsoPlayed)
				{
					M_torso = ST_Safe->m_safe_torso[H->animation_slot() - 1].torso_moving_safe[moving_idx];
				}
			}
			else
			{
				if (!m_bAnimTorsoPlayed)
				{
					M_torso = ST_Safe->m_safe_torso[4].torso_moving_safe[moving_idx];
				}
			}
		}
	}

	if (!M_torso)
	{
		if (!MpSafeMODE())
		{
			M_torso = ST->m_torso[4].moving[moving_idx];
		}
		else
		{
			M_torso = ST_Safe->m_safe_torso[4].torso_moving_safe[moving_idx];
			/*
			bool sprint = moving_idx == 3 ? true : false;
			if (mstate_rl & mcFwd && !sprint)
				M_legs = ST_Moving->legs_fwd;
			else if (mstate_rl & mcBack && !sprint)
				M_legs = ST_Moving->legs_back;
			else if (mstate_rl & mcLStrafe && !sprint)
				M_legs = ST_Moving->legs_ls;
			else if (mstate_rl & mcRStrafe && !sprint)
				M_legs = ST_Moving->legs_rs;
			*/
		}
	}

	MotionID		mid = smart_cast<IKinematicsAnimated*>(Visual())->ID_Cycle("norm_idle_0");

	if (!M_legs)
	{
		if ((mstate_rl & mcCrouch) && !isActorAccelerated(mstate_rl, IsZoomAimingMode()))
		{
			M_legs = smart_cast<IKinematicsAnimated*>(Visual())->ID_Cycle("cr_idle_1");
		}
		else
		{
			if (mstate_rl & mcCrouch || mstate_rl & mcClimb)
				M_legs = ST->legs_idle;
			else
				M_legs = !MpSafeMODE() ? ST->legs_idle : ST_Safe->m_legs_idle;
		}
	}

	if (!M_head)
	{
		M_head = !MpSafeMODE() ? ST->m_head_idle : ST_Safe->m_head_idle;
	}

	if (!M_torso)
	{
		if (m_bAnimTorsoPlayed)
		{
			M_torso = m_current_torso;
		}
		else
		{
			//M_torso = !MpSafeMODE() ? ST->m_torso_idle : ST_Safe->m_torso_idle;
			M_torso = ST->m_torso_idle;
		}
	}

	// ���� �������� ��� ����� - �������� / ����� �������� �������� �� ������
	if (m_current_torso != M_torso)
	{
		if (m_bAnimTorsoPlayed)
			m_current_torso_blend = smart_cast<IKinematicsAnimated*>(Visual())->PlayCycle(M_torso, TRUE, AnimTorsoPlayCallBack, this);
		else
			m_current_torso_blend = smart_cast<IKinematicsAnimated*>(Visual())->PlayCycle(M_torso);

		m_current_torso = M_torso;
	}

	if (m_current_head != M_head)
	{
		if (M_head)
			smart_cast<IKinematicsAnimated*>(Visual())->PlayCycle(M_head);

		m_current_head = M_head;
	}

	if (m_current_legs != M_legs)
	{
		float pos = 0.f;
		VERIFY(!m_current_legs_blend || !fis_zero(m_current_legs_blend->timeTotal));
		if ((mstate_real & mcAnyMove) && (mstate_old & mcAnyMove) && m_current_legs_blend)
			pos = fmod(m_current_legs_blend->timeCurrent, m_current_legs_blend->timeTotal) / m_current_legs_blend->timeTotal;


		IKinematicsAnimated* ka = smart_cast<IKinematicsAnimated*>(Visual());
		m_current_legs_blend = PlayMotionByParts(ka, M_legs, TRUE, legs_play_callback, this);
		//		m_current_legs_blend		= smart_cast<IKinematicsAnimated*>(Visual())->PlayCycle(M_legs,TRUE,legs_play_callback,this);


		if ((!(mstate_old & mcAnyMove)) && (mstate_real & mcAnyMove))
		{
			pos = 0.5f;//0.5f*Random.randI(2);
		}
		if (m_current_legs_blend)
			m_current_legs_blend->timeCurrent = m_current_legs_blend->timeTotal * pos;

		m_current_legs = M_legs;

		CStepManager::on_animation_start(M_legs, m_current_legs_blend);
	}



#ifdef DEBUG
	if (bDebug && g_ShowAnimationInfo)
	{
		UI().Font().pFontStat->OutSetI(0, 0);
		UI().Font().pFontStat->OutNext("[%s]", mov_state[moving_idx]);
		IKinematicsAnimated* KA = smart_cast<IKinematicsAnimated*>(Visual());
		if (M_torso)
			UI().Font().pFontStat->OutNext("torso [%s]", KA->LL_MotionDefName_dbg(M_torso).first);
		if (M_head)
			UI().Font().pFontStat->OutNext("head [%s]", KA->LL_MotionDefName_dbg(M_head).first);
		if (M_legs)
			UI().Font().pFontStat->OutNext("legs [%s]", KA->LL_MotionDefName_dbg(M_legs).first);
	}
#endif

#ifdef DEBUG
	if ((Level().CurrentControlEntity() == this) && g_ShowAnimationInfo) {
		string128 buf;
		xr_strcpy(buf, "");
		if (isActorAccelerated(mstate_rl, IsZoomAimingMode()))		xr_strcat(buf, "Accel ");
		if (mstate_rl & mcCrouch)		xr_strcat(buf, "Crouch ");
		if (mstate_rl & mcFwd)		xr_strcat(buf, "Fwd ");
		if (mstate_rl & mcBack)		xr_strcat(buf, "Back ");
		if (mstate_rl & mcLStrafe)	xr_strcat(buf, "LStrafe ");
		if (mstate_rl & mcRStrafe)	xr_strcat(buf, "RStrafe ");
		if (mstate_rl & mcJump)		xr_strcat(buf, "Jump ");
		if (mstate_rl & mcFall)		xr_strcat(buf, "Fall ");
		if (mstate_rl & mcTurn)		xr_strcat(buf, "Turn ");
		if (mstate_rl & mcLanding)	xr_strcat(buf, "Landing ");
		if (mstate_rl & mcLLookout)	xr_strcat(buf, "LLookout ");
		if (mstate_rl & mcRLookout)	xr_strcat(buf, "RLookout ");
		if (m_bJumpKeyPressed)		xr_strcat(buf, "+Jumping ");
		UI().Font().pFontStat->OutNext("MSTATE:     [%s]", buf);
		/*
				switch (m_PhysicMovementControl->Environment())
				{
				case CPHMovementControl::peOnGround:	xr_strcpy(buf,"ground");			break;
				case CPHMovementControl::peInAir:		xr_strcpy(buf,"air");				break;
				case CPHMovementControl::peAtWall:		xr_strcpy(buf,"wall");				break;
				}
				UI().Font().pFontStat->OutNext	(buf);
				UI().Font().pFontStat->OutNext	("Accel     [%3.2f, %3.2f, %3.2f]",VPUSH(NET_SavedAccel));
				UI().Font().pFontStat->OutNext	("V         [%3.2f, %3.2f, %3.2f]",VPUSH(m_PhysicMovementControl->GetVelocity()));
				UI().Font().pFontStat->OutNext	("vertex ID   %d",ai_location().level_vertex_id());

				Game().m_WeaponUsageStatistic->Draw();
				*/
	};
#endif

	if (!m_current_torso_blend)
		return;

	IKinematicsAnimated* skeleton_animated = smart_cast<IKinematicsAnimated*>(Visual());

	CMotionDef* motion0 = skeleton_animated->LL_GetMotionDef(m_current_torso);
	VERIFY(motion0);
	if (!(motion0->flags & esmSyncPart))
		return;

	if (!m_current_legs_blend)
		return;

	CMotionDef* motion1 = skeleton_animated->LL_GetMotionDef(m_current_legs);
	VERIFY(motion1);
	if (!(motion1->flags & esmSyncPart))
		return;


	m_current_torso_blend->timeCurrent = m_current_legs_blend->timeCurrent / m_current_legs_blend->timeTotal * m_current_torso_blend->timeTotal;
}


