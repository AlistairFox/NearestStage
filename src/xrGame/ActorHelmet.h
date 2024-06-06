#pragma once

#include "inventory_item_object.h"

struct SBoneProtections;

class CHelmet: public CInventoryItemObject {
private:
    typedef	CInventoryItemObject inherited;
public:
							CHelmet					();
	virtual					~CHelmet				();

	virtual void			Load					(LPCSTR section);
	
	virtual void			Hit						(float P, ALife::EHitType hit_type);
	virtual void			UpdateCL();

	shared_str				m_BonesProtectionSect;
	shared_str				m_NightVisionSect;
	shared_str				m_SuitableRepairKit;

	virtual void			OnMoveToSlot			(const SInvItemPlace& previous_place);
	virtual void			OnMoveToRuck			(const SInvItemPlace& previous_place);
	virtual void			OnEvent(NET_Packet& P, u16 type);
	virtual BOOL			net_Spawn				(CSE_Abstract* DC);
	virtual void			net_Export				(NET_Packet& P);
	virtual void			net_Import				(NET_Packet& P);
	virtual void			OnH_A_Chield			();

	float					GetDefHitTypeProtection	(ALife::EHitType hit_type);
	float					GetHitTypeProtection	(ALife::EHitType hit_type, s16 element);
	float					GetBoneArmor			(s16 element);

	float					HitThroughArmor			(float hit_power, s16 element, float ap, bool& add_wound, ALife::EHitType hit_type);

	float					m_fPowerLoss;
	float					m_fHealthRestoreSpeed;
	float 					m_fRadiationRestoreSpeed;
	float 					m_fSatietyRestoreSpeed;
	float					m_fPowerRestoreSpeed;
	float					m_fBleedingRestoreSpeed;
	float					m_fPainKill;

	float					m_fShowNearestEnemiesDistance;

	bool					m_b_HasGlass;

	void					ReloadBonesProtection	();
	void					AddBonesProtection		(LPCSTR bones_section);

	virtual void			renderable_Render()override;
	virtual void			UpdateXForm() override;
protected:
	HitImmunity::HitTypeSVec	m_HitTypeProtection;
	SBoneProtections*		m_boneProtection;	
	Fvector3				m_helmtattach_offsets[2];

protected:
	virtual bool			install_upgrade_impl	( LPCSTR section, bool test );
};
