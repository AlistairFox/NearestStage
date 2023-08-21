////////////////////////////////////////////////////////////////////////////
//	Module 		: RepairKit.cpp
//	Created 	: 08.02.2023
//  Modified 	: 08.02.2023
//	Author		: Dance Maniac (M.F.S. Team)
//	Description : Repair kit
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RepairKit.h"
#include "Actor.h"
#include "inventory.h"
#include "CustomOutfit.h"
#include "ActorHelmet.h"
#include "Weapon.h"

CRepairKit::CRepairKit()
{
	m_iPortionsNum = -1;
	m_iUseFor = 0;
	m_fRestoreCondition = 0.0f;
	m_physic_item = 0;
}

CRepairKit::~CRepairKit()
{
}

void CRepairKit::Load(LPCSTR section)
{
	inherited::Load(section);

	m_iPortionsNum = pSettings->r_s32(section, "eat_portions_num");
	m_fRestoreCondition = READ_IF_EXISTS(pSettings, r_float, section, "restore_condition", 0.5f);
	VERIFY(m_iPortionsNum < 10000);
}

BOOL CRepairKit::net_Spawn(CSE_Abstract* DC)
{
	if (!inherited::net_Spawn(DC)) return FALSE;

	return TRUE;
};

bool CRepairKit::Useful() const
{
	if (!inherited::Useful()) return false;

	//проверить не все ли еще съедено
	if (m_iPortionsNum == 0) return false;

	if (!H_Parent())
		return false;
	CActor* pA = smart_cast<CActor*>(Level().Objects.net_Find(H_Parent()->ID()));
	if (!pA)
		return false;
	CCustomOutfit* outfit = smart_cast<CCustomOutfit*>(pA->inventory().ItemFromSlot(OUTFIT_SLOT));
	CHelmet* helmet = smart_cast<CHelmet*>(pA->inventory().ItemFromSlot(HELMET_SLOT));
	CWeapon* wpn1 = smart_cast<CWeapon*>(pA->inventory().ItemFromSlot(INV_SLOT_2));
	CWeapon* wpn2 = smart_cast<CWeapon*>(pA->inventory().ItemFromSlot(INV_SLOT_3));

	if ((outfit || helmet || wpn1 || wpn2) && m_iUseFor == 1)
	{
	/*	if (outfit && outfit->GetCondition() < 0.9f && outfit->GetCondition() >= 0.4f && outfit->m_SuitableRepairKit == this->cNameSect().c_str())
			return true;
		else if (helmet && helmet->GetCondition() < 0.9f && helmet->GetCondition() >= 0.4f && helmet->m_SuitableRepairKit == this->cNameSect().c_str())
			return true;
		else if (wpn1 && wpn1->GetCondition() < 0.9f && wpn1->GetCondition() >= 0.4f && wpn1->m_SuitableRepairKit == this->cNameSect().c_str())
			return true;
		else if (wpn2 && wpn2->GetCondition() < 0.9f && wpn2->GetCondition() >= 0.4f && wpn2->m_SuitableRepairKit == this->cNameSect().c_str())
			return true;
		else*/
			return true;
	}
	else
		return false;
}

bool CRepairKit::UseBy(CEntityAlive* entity_alive)
{
	if (!inherited::Useful()) return false;

	if (m_iUseFor == 0)
		return false;

	if (m_iPortionsNum > 0)
		--m_iPortionsNum;
	else
		m_iPortionsNum = 0;

	m_iUseFor = 0;

	return true;
}

void CRepairKit::ChangeInOutfit()
{
	CActor* pA = smart_cast<CActor*>(Level().Objects.net_Find(H_Parent()->ID()));
	if (!pA)
		return;
	Msg("ChangeInOutfit");
	CCustomOutfit* outfit = smart_cast<CCustomOutfit*>(pA->inventory().ItemFromSlot(OUTFIT_SLOT));
	float rnd_cond = ::Random.randF(0.2f, 1.f);

	if (outfit)
	{
		float old_cond = outfit->GetCondition();
		float new_cond = old_cond += rnd_cond;
		if (new_cond > 1)
			new_cond = 1;

		NET_Packet P;
		Game().u_EventGen(P, GEG_PLAYER_REPAIR_OUTFIT, outfit->object_id());
		P.w_float(new_cond);
		Level().Send(P, net_flags(TRUE, TRUE, FALSE, TRUE));
	}
}

void CRepairKit::ChangeInHelmet()
{
	if (!H_Parent())
		return;
	CActor* pA = smart_cast<CActor*>(Level().Objects.net_Find(H_Parent()->ID()));
	if (!pA)
		return;
	CHelmet* helmet = smart_cast<CHelmet*>(pA->inventory().ItemFromSlot(HELMET_SLOT));
	float rnd_cond = ::Random.randF(0.2f, 1.f);

	if (helmet)
	{
		float old_cond = helmet->GetCondition();
		float new_cond = old_cond += rnd_cond;
		if (new_cond > 1)
			new_cond = 1;
		NET_Packet P;
		Game().u_EventGen(P, GEG_PLAYER_REPAIR_HELMET, helmet->object_id());
		P.w_float(new_cond);
		Level().Send(P, net_flags(TRUE, TRUE, FALSE, TRUE));
	}
}

void CRepairKit::ChangeInWpn1()
{
	if (!H_Parent())
		return;
	CActor* pA = smart_cast<CActor*>(Level().Objects.net_Find(H_Parent()->ID()));
	if (!pA)
		return;
	CWeapon* wpn = smart_cast<CWeapon*>(pA->inventory().ItemFromSlot(INV_SLOT_2));
	float rnd_cond = ::Random.randF(0.2f, 1.f);

	if (wpn)
	{
		float old_cond = wpn->GetCondition();
		float new_cond = old_cond += rnd_cond;
		if (new_cond > 1)
			new_cond = 1;
		NET_Packet P;
		Game().u_EventGen(P, GEG_PLAYER_REPAIR_WPN1, wpn->object_id());
		P.w_float(new_cond);
		Level().Send(P, net_flags(TRUE, TRUE, FALSE, TRUE));
	}
}

void CRepairKit::ChangeInWpn2()
{
	if (!H_Parent())
		return;
	CActor* pA = smart_cast<CActor*>(Level().Objects.net_Find(H_Parent()->ID()));
	if (!pA)
		return;
	CWeapon* wpn = smart_cast<CWeapon*>(pA->inventory().ItemFromSlot(INV_SLOT_3));
	float rnd_cond = ::Random.randF(0.2f, 1.f);

	if (wpn)
	{
		float old_cond = wpn->GetCondition();
		float new_cond = old_cond += rnd_cond;
		if (new_cond > 1)
			new_cond = 1;
		NET_Packet P;
		Game().u_EventGen(P, GEG_PLAYER_REPAIR_WPN2, wpn->object_id());
		P.w_float(new_cond);
		Level().Send(P, net_flags(TRUE, TRUE, FALSE, TRUE));
	};
}

void CRepairKit::ChangeRepairKitCondition(float val)
{
	m_fRestoreCondition += val;
	clamp(m_fRestoreCondition, 0.f, 1.f);
}

float CRepairKit::GetRepairKitCondition() const
{
	return m_fRestoreCondition;
}