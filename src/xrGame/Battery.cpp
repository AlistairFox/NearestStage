////////////////////////////////////////////////////////////////////////////
//	Module 		: Battery.cpp
//	Created 	: 07.04.2021
//  Modified 	: 07.04.2021
//	Author		: Dance Maniac (M.F.S. Team)
//	Description : Torch battery
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Battery.h"
#include "Torch.h"
#include "Actor.h"
#include "inventory.h"
#include "game_sv_base.h"
#include "CustomDetector.h"
#include "AnomalyDetector.h"

CBattery::CBattery()
{
	m_iPortionsNum = -1;
	m_fBatteryChargeLevel = 1.0f;
	m_physic_item = 0;
}

CBattery::~CBattery()
{
}

void CBattery::Load(LPCSTR section)
{
	inherited::Load(section);

	m_iPortionsNum = pSettings->r_s32(section, "eat_portions_num");
	m_fBatteryChargeLevel = READ_IF_EXISTS(pSettings, r_float, section, "charge_level", 1.0f);
	VERIFY(m_iPortionsNum < 10000);
}



BOOL CBattery::net_Spawn(CSE_Abstract* DC)
{
	if (!inherited::net_Spawn(DC)) return FALSE;

	return TRUE;
};

bool CBattery::Useful() const
{
	if (!inherited::Useful()) return false;
	if (!H_Parent())
		return false;
	CActor* pA = smart_cast<CActor*>(Level().Objects.net_Find(H_Parent()->ID()));
	if (!pA)
		return false;

	CTorch* flashlight = smart_cast<CTorch*>(pA->inventory().ItemFromSlot(TORCH_SLOT));
	CCustomDetector* artifact_detector = smart_cast<CCustomDetector*>(pA->inventory().ItemFromSlot(DETECTOR_SLOT));
	CDetectorAnomaly* anomaly_detector = smart_cast<CDetectorAnomaly*>(pA->inventory().ItemFromSlot(DOSIMETER_SLOT));

	if (!flashlight || !artifact_detector || !anomaly_detector)
		return false;

	//ïðîâåðèòü íå âñå ëè åùå ñúåäåíî
	if (m_iPortionsNum == 0) return false;

	return true;
}

bool CBattery::UseBy(CEntityAlive* entity_alive)
{
	if (!inherited::Useful()) 
		return false;

	if (m_iPortionsNum > 0)
		--m_iPortionsNum;
	else
		m_iPortionsNum = 0;

	return true;
}

void CBattery::ChargeTorch()
{
	if (!H_Parent())
		return;

	CActor* pA = smart_cast<CActor*>(Level().Objects.net_Find(H_Parent()->ID()));

	if (!pA)
		return;

	CTorch* flashlight = smart_cast<CTorch*>(pA->inventory().ItemFromSlot(TORCH_SLOT));

	if (flashlight)
	{
		NET_Packet P;
		Msg("BatteryChargetLevel: %f", m_fBatteryChargeLevel);
		Game().u_EventGen(P, GEG_PLAYER_CHARGE_TORCH, flashlight->object_id());
		P.w_float(m_fBatteryChargeLevel);
		Level().Send(P, net_flags(TRUE, TRUE, FALSE, TRUE));
	}
}

void CBattery::ChargeArtifactDetector()
{
	if (!H_Parent())
		return;

	CActor* pA = smart_cast<CActor*>(Level().Objects.net_Find(H_Parent()->ID()));

	if (!pA)
		return;

	CCustomDetector* artifact_detector = smart_cast<CCustomDetector*>(pA->inventory().ItemFromSlot(DETECTOR_SLOT));

	if (artifact_detector)
	{
		NET_Packet P;
		Game().u_EventGen(P, GEG_PLAYER_CHARGE_DETECTORS, artifact_detector->object_id());
		P.w_float(m_fBatteryChargeLevel);
		Level().Send(P, net_flags(TRUE, TRUE, FALSE, TRUE));
	}
}

void CBattery::ChargeAnomalyDetector()
{
	if (!H_Parent())
		return;

	CActor* pA = smart_cast<CActor*>(Level().Objects.net_Find(H_Parent()->ID()));

	if (!pA)
		return;

	CDetectorAnomaly* anomaly_detector = smart_cast<CDetectorAnomaly*>(pA->inventory().ItemFromSlot(DOSIMETER_SLOT));

	if (anomaly_detector)
	{
		NET_Packet P;
		Game().u_EventGen(P, GEG_PLAYER_CHARGE_DOSIMETER, anomaly_detector->object_id());
		P.w_float(m_fBatteryChargeLevel);
		Level().Send(P, net_flags(TRUE, TRUE, FALSE, TRUE));
	}
}