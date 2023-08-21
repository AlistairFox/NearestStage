#pragma once

#include "eatable_item_object.h"

class CRepairKit : public CEatableItemObject
{
	using inherited = CEatableItemObject;

public:
	CRepairKit();
	virtual					~CRepairKit();
	virtual CRepairKit* cast_repair_kit() { return this; }

	virtual void			Load(LPCSTR section);
	virtual bool			Useful() const;

	virtual BOOL			net_Spawn(CSE_Abstract* DC);

	virtual	bool			UseBy(CEntityAlive* npc);
	virtual	bool			Empty() { return PortionsNum() == 0; };
	int						PortionsNum()	const { return m_iPortionsNum; }
	int						m_iUseFor = 0;
	float					m_fRestoreCondition;
	void					ChangeInOutfit();
	void					ChangeInHelmet();
	void					ChangeInWpn1();
	void					ChangeInWpn2();
	void					ChangeRepairKitCondition(float val);
	float					GetRepairKitCondition(void) const;
	bool					UseAllowed();
protected:
	int						m_iPortionsNum;
};