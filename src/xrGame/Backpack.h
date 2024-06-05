#pragma once

#include "inventory_item_object.h"
#include "hud_item_object.h"
#include "HudSound.h"
#include "player_hud.h"

// Author: Charsi82

class CBackpack : public CHudItemObject
{
private:
    using inherited = CHudItemObject;

    Fvector				toggle_offsets[2];
    float               m_sFactor = 0;
public:
    float m_additional_weight;

    int m_iInventoryCapacity = 0;
    int				GetInventoryCapacity() const { return m_iInventoryCapacity; }
    CBackpack() : m_additional_weight(0.f) {};
    virtual ~CBackpack() = default;

    virtual void Load(LPCSTR section);
    float AdditionalInventoryWeight();

    virtual void	OnH_A_Chield()override;
    virtual void	OnH_B_Independent(bool just_before_destroy) override;
    virtual void	OnHiddenItem() override;
    virtual void	OnActiveItem() override;
    virtual bool	Action(u16 cmd, u32 flags) override;
    virtual void	OnStateSwitch(u32 S) override;
    virtual void	OnAnimationEnd(u32 state) override;
    virtual void	SwitchState(u32 S) override;
    virtual void	OnMoveToSlot(const SInvItemPlace& prev) override;
    virtual void	OnMoveToRuck(const SInvItemPlace& prev) override;
    virtual void	shedule_Update(u32 dt) override;
    virtual void	UpdateCL() override;
    virtual void	UpdateXForm() override;
    virtual void	UpdateHudAdditional(Fmatrix& trans) override;
    virtual void	renderable_Render() override;
};