#pragma once
#pragma once
#include "inventory_item_object.h"
#include "HudSound.h"
#include "UIRadioItem.h"
#include "hud_item_object.h"

class CInventoryOwner;
class UIRadioItem;

class CRadioItem : public CHudItemObject
{
	using inherited = CHudItemObject;
public:

	CRadioItem(void);
	virtual			~CRadioItem(void);

	virtual BOOL	net_Spawn(CSE_Abstract* DC)override;
	virtual void	Load(LPCSTR section)override;

	virtual void	OnEvent(NET_Packet& P, u16 type);
	virtual void	net_Export(NET_Packet& P);
	virtual void	net_Import(NET_Packet& P);

	virtual void	OnH_A_Chield()override;
	virtual void	OnH_B_Independet(bool just_before_destroy);
	virtual void	OnHiddenItem() override;
	virtual void	OnActiveItem() override;
	virtual void	OnStateSwitch(u32 S) override;
	virtual void	OnAnimationEnd(u32 state) override;
	virtual void	SwitchState(u32 S) override;
	virtual void	UpdateXForm() override;
	void	ShowUI(bool show);
	void	TurnOn();
	void	TurnOff();
	void	ActivateVoice(bool status);
	bool	IsWorking() { return m_bWorking && (GetState() == eIdle || GetState() == eHiding); }
	virtual void	OnMoveToSlot(const SInvItemPlace& prev) override;
	virtual void	OnMoveToRuck(const SInvItemPlace& prev) override;
	void	SetRadioHZ(u16 hz);
	virtual void	UpdateHudAdditional(Fmatrix& trans) override;
	virtual void	shedule_Update(u32 dt) override;
	virtual void	UpdateCL() override;


	u16					CurrentHZ;
	u16					MinHZ;
	u16					MaxHZ;
	float					MaxDistance;
	bool				SayNow = false;

private:
	bool				ActiveSnd = false;
	LPCSTR				idle_snd;
	float				m_sFactor;
	Fvector				toggle_offsets[2];
	ref_sound			IdleSound;
	u32					SoundTimer = 0;
	bool				m_bNeedActivation;
	bool				m_bWorking;
	CActor* CurrentActor;
	CInventoryOwner* CurrentInvOwner;
	UIRadioItem* pUIRadioItem;
};