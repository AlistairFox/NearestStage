////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_monster_base.cpp
//	Created 	: 07.02.2007
//  Modified 	: 07.02.2007
//	Author		: Dmitriy Iassenev
//	Description : ALife mnster base class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "alife_simulator.h"
#include "xrServer.h"
#include "alife_monster_brain.h"

void CSE_ALifeMonsterBase::on_spawn				()
{
	inherited1::on_spawn		();
}

extern void add_online_impl		(CSE_ALifeDynamicObject *object, const bool &update_registries);

void CSE_ALifeMonsterBase::add_online			(const bool &update_registries)
{
	add_online_impl				(this,update_registries);
	brain().on_switch_online	();
}

extern void add_offline_impl	(CSE_ALifeDynamicObject *object, const xr_vector<ALife::_OBJECT_ID> &saved_children, const bool &update_registries);

void CSE_ALifeMonsterBase::add_offline			(const xr_vector<ALife::_OBJECT_ID> &saved_children, const bool &update_registries)
{
	add_offline_impl			(this,saved_children,update_registries);
	brain().on_switch_offline	();
}
