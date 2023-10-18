#include "StdAfx.h"
#include "Actor.h"
#include "actor_anim_defs.h"
#include "../Include/xrRender/Kinematics.h"

void callbackWoundAnim(CBlend* blend)
{
	CActor* act = (CActor*)blend->CallbackParam;
	if (act)
		act->CanWoundChange = true;
}

bool CActor::MpWoundMODE() const
{
	if (!g_Alive())
		return false;

	if (Level().CurrentControlEntity() == this)
	{
		if (!CanWoundChange)
			return true;

		if (!InWoundPlay || !MidWoundPlay || !OutWoundPlay)
			return true;

		return ANIM_WOUND > 0;
	}
	else
		return !CanWoundChange;
}

void SActorStateAnimation::CreateWoundAnimationScripted(IKinematicsAnimated* K)
{
	string_path filepath;
	FS.update_path(filepath, "$game_config$", "actor_wound.ltx");
	CInifile* file = xr_new<CInifile>(filepath, true, true);

	if (file && file->section_exist("animations"))
	{
		u32 count = file->r_u32("animations", "count");
		for (int i = 0; i < count; i++)
		{
			string32 tmp = { 0 };
			itoa(i, tmp, 10);
			string32 animation = { 0 };
			xr_strcat(animation, "anim_");
			xr_strcat(animation, tmp);

			if (file->section_exist(animation))
			{
				bool anim_loop = file->r_bool(animation, "anim_loop");

				m_animation_loop[i] = anim_loop;

				LPCSTR anims_in = file->r_string(animation, "anim_in");
				LPCSTR anims_out = file->r_string(animation, "anim_out");
				LPCSTR anims_middle = file->r_string(animation, "anim_middle");
				u32 countIN = _GetItemCount(anims_in, ',');
				u32 countOUT = _GetItemCount(anims_out, ',');
				u32 countMID = _GetItemCount(anims_middle, ',');

				in_woundanims.count[i] = countIN;
				out_woundanims.count[i] = countOUT;
				middle_woundanims.count[i] = countMID;

				for (int id = 0; id != countIN; id++)
				{
					string64 anim = { 0 };
					_GetItem(anims_in, id, anim, ',');
					MotionID motionAnim = K->ID_Cycle_Safe(anim);
					in_woundanims.m_wound_animation_in[i][id] = motionAnim;
				}
				for (int id = 0; id != countOUT; id++)
				{
					string64 anim = { 0 };
					_GetItem(anims_out, id, anim, ',');
					MotionID motionAnim = K->ID_Cycle_Safe(anim);
					out_woundanims.m_wound_animation_out[i][id] = motionAnim;
				}
				for (int id = 0; id != countMID; id++)
				{
					string64 anim = { 0 };
					_GetItem(anims_middle, id, anim, ',');
					MotionID motionAnim = K->ID_Cycle_Safe(anim);
					middle_woundanims.m_wound_animation[i][id] = motionAnim;
				}
			}
		}
	}
}


void CActor::script_wound_anim(MotionID Animation, PlayCallback Callback, LPVOID CallbackParam)
{
	IKinematicsAnimated* k = smart_cast<IKinematicsAnimated*>(Visual());
	k->LL_PlayCycle(
		k->LL_GetMotionDef(Animation)->bone_or_part,
		Animation,
		TRUE,
		k->LL_GetMotionDef(Animation)->Accrue(),
		k->LL_GetMotionDef(Animation)->Falloff(),
		k->LL_GetMotionDef(Animation)->Speed(),
		k->LL_GetMotionDef(Animation)->StopAtEnd(),
		Callback, CallbackParam, 0
	);

	CanWoundChange = false;
	SendWoundAnimationToServer(Animation);

}


void CActor::ReciveWoundAnimationPacket(NET_Packet& packet)
{
	MotionID motion;
	packet.r(&motion, sizeof(motion));

	if (motion.valid())
	{
		IKinematicsAnimated* k = smart_cast<IKinematicsAnimated*>(Visual());
		k->LL_PlayCycle(
			k->LL_GetMotionDef(motion)->bone_or_part,
			motion,
			TRUE,
			k->LL_GetMotionDef(motion)->Accrue(),
			k->LL_GetMotionDef(motion)->Falloff(),
			k->LL_GetMotionDef(motion)->Speed(),
			k->LL_GetMotionDef(motion)->StopAtEnd(),
			callbackWoundAnim, this, 0
		);

		CanWoundChange = false;
	}

}

void CActor::SendWoundAnimationToServer(MotionID motion)
{
	NET_Packet packet;
	u_EventGen(packet, GE_ACTOR_WOUND_SCRIPT, this->ID());
	packet.w(&motion, sizeof(motion));
	u_EventSend(packet, net_flags(true, true));
}

void CActor::SelectScriptWoundAnimation()
{
	if (!CanWoundChange)
		return;

	if (oldWoundAnim != ANIM_WOUND)
	{
		if (InWoundPlay && MidWoundPlay && OutWoundPlay)
		{
			oldWoundAnim = ANIM_WOUND;
			InputWoundAnim = 0;
			OutWoundAnim = 0;
			MidWoundAnim = 0;
		}
	}

	u32 selectedAnimation = oldWoundAnim;

	selectedWoundID = selectedAnimation;


	u32 countIN = m_anims->m_script.in_woundanims.count[selectedAnimation];

	MidWoundPlay = false;
	OutWoundPlay = false;
	InWoundPlay = false;

	MotionID script_BODY;

	if (countIN == 0)
		InWoundPlay = true;
	else
		if (InputWoundAnim >= countIN)
			InWoundPlay = true;
		else
			InWoundPlay = false;


	if (!InWoundPlay)
	{
		script_BODY = m_anims->m_script.in_woundanims.m_wound_animation_in[selectedAnimation][InputWoundAnim];
		script_wound_anim(script_BODY, callbackWoundAnim, this);
		InputWoundAnim += 1;
		NEED_WOUND_EXIT = true;

		if (m_anims->m_script.m_animation_attach[selectedAnimation].size() > 0)
		{
			shared_str attach = m_anims->m_script.m_animation_attach[selectedAnimation];
		}

		NET_Packet packet;
		u_EventGen(packet, GE_ACTOR_HIDE_ALL_STATE, this->ID());
		u_EventSend(packet, net_flags(true, true));
	}

	if (!InWoundPlay)
		return;

	u32 countMid = m_anims->m_script.middle_woundanims.count[selectedAnimation];

	if (MidWoundAnim >= countMid)
	{
		if (ANIM_WOUND == 0)
		{
			MidWoundPlay = true;
		}
		else
		{
			bool valid = selectedAnimation != ANIM_WOUND;
			if (m_anims->m_script.m_animation_loop[selectedAnimation] && !valid)
				MidWoundAnim = 0;
			else
				MidWoundPlay = true;

		}
	}
	else
	{
		if (countMid == 0)
			MidWoundPlay = true;
		else
			MidWoundPlay = false;
	}

	if (!MidWoundPlay)
	{
		script_BODY = m_anims->m_script.middle_woundanims.m_wound_animation[selectedAnimation][MidWoundAnim];
		script_wound_anim(script_BODY, callbackWoundAnim, this);
		MidWoundAnim += 1;
	}

	if (!MidWoundPlay)
		return;

	u32 countOUT = m_anims->m_script.out_woundanims.count[selectedAnimation];

	if (countOUT == 0)
		OutWoundPlay = true;

	if (OutWoundAnim >= countOUT)
		OutWoundPlay = true;
	else
		OutWoundPlay = false;

	if (!OutWoundPlay)
	{
		script_BODY = m_anims->m_script.out_woundanims.m_wound_animation_out[selectedAnimation][OutWoundAnim];
		script_wound_anim(script_BODY, callbackWoundAnim, this);
		OutWoundAnim += 1;
	}

	if (OutWoundPlay)
	{
		NEED_WOUND_EXIT = false;
		if (m_anims->m_script.m_animation_attach[selectedAnimation].size() > 0)
		{
			shared_str attach = m_anims->m_script.m_animation_attach[selectedAnimation];
		}
	}
}

void CActor::StartWoundExit()
{
	OutWoundPlay = true;
	CanWoundChange = true;
	NEED_WOUND_EXIT = false;
	ANIM_WOUND = 0;
	StopWoundAnims();
}

void CActor::StopWoundAnims()
{
	NEED_WOUND_EXIT = true;
	ANIM_WOUND = 0;
}