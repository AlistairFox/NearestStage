#include "stdafx.h"
#include "ActorHelmet.h"
#include "Actor.h"
#include "Inventory.h"
#include "Torch.h"
#include "BoneProtections.h"
#include "../Include/xrRender/Kinematics.h"
#include "DynamicHudGlass.h"
#include "static_cast_checked.hpp"
#include "CameraEffector.h"
#include "ActorEffector.h"
//#include "CustomOutfit.h"

CHelmet::CHelmet()
{
	m_flags.set(FUsingCondition, TRUE);
	m_HitTypeProtection.resize(ALife::eHitTypeMax);
	for(int i=0; i<ALife::eHitTypeMax; i++)
		m_HitTypeProtection[i] = 1.0f;

	m_boneProtection = xr_new<SBoneProtections>();

	m_b_HasGlass = false;
	m_SuitableRepairKit = nullptr;
}


CHelmet::~CHelmet()
{
	xr_delete(m_boneProtection);
}

void CHelmet::Load(LPCSTR section) 
{
	inherited::Load(section);

	m_HitTypeProtection[ALife::eHitTypeBurn]		= pSettings->r_float(section,"burn_protection");
	m_HitTypeProtection[ALife::eHitTypeStrike]		= pSettings->r_float(section,"strike_protection");
	m_HitTypeProtection[ALife::eHitTypeShock]		= pSettings->r_float(section,"shock_protection");
	m_HitTypeProtection[ALife::eHitTypeWound]		= pSettings->r_float(section,"wound_protection");
	m_HitTypeProtection[ALife::eHitTypeRadiation]	= pSettings->r_float(section,"radiation_protection");
	m_HitTypeProtection[ALife::eHitTypeTelepatic]	= pSettings->r_float(section,"telepatic_protection");
	m_HitTypeProtection[ALife::eHitTypeChemicalBurn]= pSettings->r_float(section,"chemical_burn_protection");
	m_HitTypeProtection[ALife::eHitTypeExplosion]	= pSettings->r_float(section,"explosion_protection");
	m_HitTypeProtection[ALife::eHitTypeFireWound]	= 0.0f;//pSettings->r_float(section,"fire_wound_protection");
//	m_HitTypeProtection[ALife::eHitTypePhysicStrike]= pSettings->r_float(section,"physic_strike_protection");
	m_HitTypeProtection[ALife::eHitTypeLightBurn]	= m_HitTypeProtection[ALife::eHitTypeBurn];
	m_boneProtection->m_fHitFracActor				= pSettings->r_float(section, "hit_fraction_actor");

	if (pSettings->line_exist(section, "nightvision_sect"))
		m_NightVisionSect = pSettings->r_string(section, "nightvision_sect");
	else
		m_NightVisionSect = "";

	m_fHealthRestoreSpeed			= READ_IF_EXISTS(pSettings, r_float, section, "health_restore_speed",    0.0f );
	m_fRadiationRestoreSpeed		= READ_IF_EXISTS(pSettings, r_float, section, "radiation_restore_speed", 0.0f );
	m_fSatietyRestoreSpeed			= READ_IF_EXISTS(pSettings, r_float, section, "satiety_restore_speed",   0.0f );
	m_fPowerRestoreSpeed			= READ_IF_EXISTS(pSettings, r_float, section, "power_restore_speed",     0.0f );
	m_fBleedingRestoreSpeed			= READ_IF_EXISTS(pSettings, r_float, section, "bleeding_restore_speed",  0.0f );
	m_fPowerLoss					= READ_IF_EXISTS(pSettings, r_float, section, "power_loss",    1.0f );
	clamp							( m_fPowerLoss, 0.0f, 1.0f );

	m_BonesProtectionSect			= READ_IF_EXISTS(pSettings, r_string, section, "bones_koeff_protection",  "" );
	m_fShowNearestEnemiesDistance	= READ_IF_EXISTS(pSettings, r_float, section, "nearest_enemies_show_dist",  0.0f );

	m_b_HasGlass = !!READ_IF_EXISTS(pSettings, r_bool, section, "has_glass", FALSE);
	m_SuitableRepairKit = READ_IF_EXISTS(pSettings, r_string, section, "suitable_repair_kit", "repair_kit");
	m_helmtattach_offsets[0] = READ_IF_EXISTS(pSettings, r_fvector3, section, "helm_attach_offsets_pos", Fvector().set(0.f, 0.01f, -0.033));
	m_helmtattach_offsets[1] = READ_IF_EXISTS(pSettings, r_fvector3, section, "helm_attach_offsets_ros", Fvector().set(10.f, 10.f, 90.f));
	IsDynamicHeldAvaliable = READ_IF_EXISTS(pSettings, r_bool, section, "dynamic_helm_avaliable", false);
}

void CHelmet::ReloadBonesProtection()
{
	CObject* parent = H_Parent();
	if(IsGameTypeSingle())
		parent = smart_cast<CObject*>(Level().CurrentViewEntity());

	if(parent && parent->Visual() && m_BonesProtectionSect.size())
		m_boneProtection->reload( m_BonesProtectionSect, smart_cast<IKinematics*>(parent->Visual()));
}

BOOL CHelmet::net_Spawn(CSE_Abstract* DC)
{
	if(IsGameTypeSingle())
		ReloadBonesProtection();

	BOOL res = inherited::net_Spawn(DC);
	return					(res);
}

void CHelmet::net_Export(NET_Packet& P)
{
	inherited::net_Export(P);
	P.w_float_q8(GetCondition(),0.0f,1.0f);

	if (!IsGameTypeSingle())
	{
		u8 in_slot = u8(m_pInventory && m_pInventory->InSlot(this));
		P.w_u8(in_slot);
	}
}

void CHelmet::net_Import(NET_Packet& P)
{
	inherited::net_Import(P);
	float _cond;
	P.r_float_q8(_cond,0.0f,1.0f);
	SetCondition(_cond);

	if (!IsGameTypeSingle())
	{
		u8 in_slot = 0;
		P.r_u8(in_slot);
	}
}

void CHelmet::OnH_A_Chield()
{
	inherited::OnH_A_Chield();
//	ReloadBonesProtection();
}

void CHelmet::UpdateCL()
{
	inherited::UpdateCL();
}


void CHelmet::OnMoveToSlot(const SInvItemPlace& previous_place)
{
	inherited::OnMoveToSlot		(previous_place);
	CActor* pActor = smart_cast<CActor*> (H_Parent());
	if (pActor && !g_dedicated_server)
	{
		CEffectorCam* ec = pActor->Cameras().GetCamEffector(eCEWeaponAction);
		if (NULL == ec)
		{
			string_path			ce_path;
			string_path			anm_name = "itemuse_anm_effects\\gasmask.anm";
			if (FS.exist(ce_path, "$game_anims$", anm_name))
			{
				CAnimatorCamEffector* e = xr_new<CAnimatorCamEffector>();
				e->SetType(eCEWeaponAction);
				e->SetHudAffect(false);
				e->SetCyclic(false);
				e->Start(anm_name);
				pActor->Cameras().AddCamEffector(e);
			}
		}
	}
	if (m_pInventory && (previous_place.type==eItemPlaceSlot))
	{
		if (pActor)
		{
			CTorch* pTorch = smart_cast<CTorch*>(pActor->inventory().ItemFromSlot(TORCH_SLOT));
			if(pTorch && pTorch->GetNightVisionStatus())
				pTorch->SwitchNightVision(true, false);
		}
	}
}

void CHelmet::OnMoveToRuck(const SInvItemPlace& previous_place)
{
	inherited::OnMoveToRuck		(previous_place);
	if (m_pInventory && (previous_place.type==eItemPlaceSlot))
	{
		CActor* pActor = smart_cast<CActor*> (H_Parent());
		if (pActor && !g_dedicated_server)
		{
			CEffectorCam* ec = pActor->Cameras().GetCamEffector(eCEWeaponAction);
			if (NULL == ec)
			{
				string_path			ce_path;
				string_path			anm_name = "itemuse_anm_effects\\gasmask.anm";
				if (FS.exist(ce_path, "$game_anims$", anm_name))
				{
					CAnimatorCamEffector* e = xr_new<CAnimatorCamEffector>();
					e->SetType(eCEWeaponAction);
					e->SetHudAffect(false);
					e->SetCyclic(false);
					e->Start(anm_name);
					pActor->Cameras().AddCamEffector(e);
				}
			}
		}

		if (pActor)
		{
			CTorch* pTorch = smart_cast<CTorch*>(pActor->inventory().ItemFromSlot(TORCH_SLOT));
			if(pTorch)
				pTorch->SwitchNightVision(false);
		}
	}
}

void CHelmet::OnEvent(NET_Packet& P, u16 type)
{
	switch (type)
	{
	case GEG_PLAYER_REPAIR_HELMET:
	{
		float cond;
		P.r_float(cond);
		SetCondition(cond);
	}break;
	default:
		inherited::OnEvent(P, type);
		break;
	}
}

void CHelmet::Hit(float hit_power, ALife::EHitType hit_type)
{
	hit_power *= GetHitImmunity(hit_type);
	ChangeCondition(-hit_power);
}

float CHelmet::GetDefHitTypeProtection(ALife::EHitType hit_type)
{
	return m_HitTypeProtection[hit_type]*GetCondition();
}

float CHelmet::GetHitTypeProtection(ALife::EHitType hit_type, s16 element)
{
	float fBase = m_HitTypeProtection[hit_type]*GetCondition();
	float bone = m_boneProtection->getBoneProtection(element);
	return fBase*bone;
}

float CHelmet::GetBoneArmor(s16 element)
{
	return m_boneProtection->getBoneArmor(element);
}

bool CHelmet::install_upgrade_impl( LPCSTR section, bool test )
{
	bool result = inherited::install_upgrade_impl( section, test );

	result |= process_if_exists( section, "burn_protection",          &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeBurn]        , test );
	result |= process_if_exists( section, "shock_protection",         &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeShock]       , test );
	result |= process_if_exists( section, "strike_protection",        &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeStrike]      , test );
	result |= process_if_exists( section, "wound_protection",         &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeWound]       , test );
	result |= process_if_exists( section, "radiation_protection",     &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeRadiation]   , test );
	result |= process_if_exists( section, "telepatic_protection",     &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeTelepatic]   , test );
	result |= process_if_exists( section, "chemical_burn_protection", &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeChemicalBurn], test );
	result |= process_if_exists( section, "explosion_protection",     &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeExplosion]   , test );
	result |= process_if_exists( section, "fire_wound_protection",    &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeFireWound]   , test );

	LPCSTR str;
	bool result2 = process_if_exists_set( section, "nightvision_sect", &CInifile::r_string, str, test );
	if ( result2 && !test )
	{
		m_NightVisionSect._set( str );
	}
	result |= result2;

	result |= process_if_exists( section, "health_restore_speed",    &CInifile::r_float, m_fHealthRestoreSpeed,    test );
	result |= process_if_exists( section, "radiation_restore_speed", &CInifile::r_float, m_fRadiationRestoreSpeed, test );
	result |= process_if_exists( section, "satiety_restore_speed",   &CInifile::r_float, m_fSatietyRestoreSpeed,   test );
	result |= process_if_exists( section, "power_restore_speed",     &CInifile::r_float, m_fPowerRestoreSpeed,     test );
	result |= process_if_exists( section, "bleeding_restore_speed",  &CInifile::r_float, m_fBleedingRestoreSpeed,  test );

	result |= process_if_exists( section, "power_loss", &CInifile::r_float, m_fPowerLoss, test );
	clamp( m_fPowerLoss, 0.0f, 1.0f );

	result |= process_if_exists( section, "nearest_enemies_show_dist",  &CInifile::r_float, m_fShowNearestEnemiesDistance,  test );

	result2 = process_if_exists_set( section, "bones_koeff_protection", &CInifile::r_string, str, test );
	if ( result2 && !test )
	{
		m_BonesProtectionSect	= str;
		ReloadBonesProtection	();
	}
	result2 = process_if_exists_set( section, "bones_koeff_protection_add", &CInifile::r_string, str, test );
	if ( result2 && !test )
		AddBonesProtection	(str);

	return result;
}

void CHelmet::AddBonesProtection(LPCSTR bones_section)
{
	CObject* parent = H_Parent();
	if(IsGameTypeSingle())
		parent = smart_cast<CObject*>(Level().CurrentViewEntity());

	if ( parent && parent->Visual() && m_BonesProtectionSect.size() )
		m_boneProtection->add(bones_section, smart_cast<IKinematics*>( parent->Visual() ) );
}

void CHelmet::renderable_Render()
{
	if (H_Parent())
	{
		CActor* pA = smart_cast<CActor*>(H_Parent());
		if (!pA)
			return;

		if (Level().CurrentControlEntity() == H_Parent() && (pA->MpAnimationMODE() || pA->MpWoundMODE()))
			return;
	}
	if (!IsDynamicHeldAvaliable)
		return;

	UpdateXForm();

	inherited::renderable_Render();
}

void CHelmet::UpdateXForm()
{
	if (0 == H_Parent())	return;

	// Get access to entity and its visual
	CEntityAlive* E = smart_cast<CEntityAlive*>(H_Parent());
	if (!E) return;

	if (E->cast_base_monster()) return;

	const CInventoryOwner* parent = smart_cast<const CInventoryOwner*>(E);
	CActor* pA = smart_cast<CActor*>(E);
	if (parent && parent->use_simplified_visual())
		return;

	if (parent->attached(this))
		return;

	R_ASSERT(E);
	IKinematics* V = smart_cast<IKinematics*>	(E->Visual());
	VERIFY(V);


	V->CalculateBones();

	Fmatrix	mRes = V->LL_GetTransform(V->LL_BoneID("bip01_neck"));
	Fmatrix Rotation;
	Rotation.setHPB(deg2rad(m_helmtattach_offsets[1].x), deg2rad(m_helmtattach_offsets[1].y), deg2rad(m_helmtattach_offsets[1].z));
	Rotation.c = m_helmtattach_offsets[0];
	mRes.mulA_43(E->XFORM());
	mRes.mulB_43(Rotation);
	renderable.xform = mRes;

}

float CHelmet::HitThroughArmor(float hit_power, s16 element, float ap, bool& add_wound, ALife::EHitType hit_type)
{
	float NewHitPower = hit_power;
	if(hit_type == ALife::eHitTypeFireWound)
	{
		float ba = GetBoneArmor(element);
		if(ba<0.0f)
			return NewHitPower;

		float BoneArmor = ba*GetCondition();
		if(/*!fis_zero(ba, EPS) && */(ap > BoneArmor))
		{
			//���� ������� �����
			if(!IsGameTypeSingle())
			{
				float hit_fraction = (ap - BoneArmor) / ap;
				if(hit_fraction < m_boneProtection->m_fHitFracActor)
					hit_fraction = m_boneProtection->m_fHitFracActor;

				NewHitPower *= hit_fraction;
				NewHitPower *= m_boneProtection->getBoneProtection(element);
			}

			VERIFY(NewHitPower>=0.0f);
		}
		else
		{
			//���� �� ������� �����
			NewHitPower *= m_boneProtection->m_fHitFracActor;
			add_wound = false; 	//���� ���
		}
	}
	else
	{
		float one = 0.1f;
		if(hit_type == ALife::eHitTypeStrike || 
		   hit_type == ALife::eHitTypeWound || 
		   hit_type == ALife::eHitTypeWound_2 || 
		   hit_type == ALife::eHitTypeExplosion)
		{
			one = 1.0f;
		}
		float protect = GetDefHitTypeProtection(hit_type);
		NewHitPower -= protect * one;

		if(NewHitPower < 0.f)
			NewHitPower = 0.f;
	}
	//��������� ������������ �����
	Hit(hit_power, hit_type);

	return NewHitPower;
}
