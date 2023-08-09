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

	//ïðîâåðèòü íå âñå ëè åùå ñúåäåíî
	if (m_iPortionsNum == 0) return false;

	return true;
}

bool CBattery::UseBy(CEntityAlive* entity_alive)
{
	CActor* pA = smart_cast<CActor*>(entity_alive);
	if (!pA)
		return false;

	CTorch* flashlight = smart_cast<CTorch*>(pA->inventory().ItemFromSlot(TORCH_SLOT));

	if (flashlight)
	{

		NET_Packet P;
		Msg("BatteryChargetLevel: %f", m_fBatteryChargeLevel);
		Game().u_EventGen(P, GEG_PLAYER_USE_BATTERY, flashlight->object_id());
		P.w_float(m_fBatteryChargeLevel);
		Level().Send(P, net_flags(TRUE, TRUE, FALSE, TRUE));
	}

	//Msg("Battery Charge is: %f", m_fBatteryChargeLevel); //Äëÿ òåñòîâ

	if (m_iPortionsNum > 0)
		--m_iPortionsNum;
	else
		m_iPortionsNum = 0;
	return true;
}