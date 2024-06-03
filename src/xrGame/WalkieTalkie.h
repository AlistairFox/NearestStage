#pragma once
#pragma once
#include "inventory_item_object.h"
#include "HudSound.h"
#include "UIWalkieTalkie.h"
#include "hud_item_object.h"

class CInventoryOwner;
class UIWalkieTalkie;

class CWalkieTalkie : public CHudItemObject
{
	using inherited = CHudItemObject;
public:

	CWalkieTalkie(void);
	virtual			~CWalkieTalkie(void);

	virtual BOOL	net_Spawn(CSE_Abstract* DC)override;
	virtual void	Load(LPCSTR section)override;

	virtual void	OnEvent(NET_Packet& P, u16 type);
	virtual void	net_Export(NET_Packet& P);
	virtual void	net_Import(NET_Packet& P);

	virtual void	OnH_A_Chield()override;
	virtual void	OnH_B_Independent(bool just_before_destroy) override;
	virtual void	OnHiddenItem() override;
	virtual void	OnActiveItem() override;
	virtual void	OnStateSwitch(u32 S) override;
	virtual void	OnAnimationEnd(u32 state) override;
	virtual void	SwitchState(u32 S) override;
	virtual BOOL	net_Relevant() override;
			void	ShowUI(bool show);
			void	TakeOn();
			void	TakeOff();
			void	ActivateVoice(bool status);
			bool	IsInHand() { return m_bRadioInHand && (GetState() == eIdle || GetState() == eHiding); }
			bool	IsEnabled() { return m_bRadioEnabled; }
			void	EnableRadio(bool status);
	virtual void	OnMoveToSlot(const SInvItemPlace& prev) override;
	virtual void	OnMoveToRuck(const SInvItemPlace& prev) override;
			void	SetRadioHZ(u16 hz);
	virtual void	UpdateHudAdditional(Fmatrix& trans) override;
	virtual void	shedule_Update(u32 dt) override;
	virtual void	UpdateCL() override;


	u16					CurrentHZ;
	u16					MinHZ;
	u16					MaxHZ;
	float				MaxDistance;
	bool				SayNow = false;
	bool				UserSayInRadio = false;

private:
	bool				ActiveSnd = false;
	LPCSTR				idle_snd;
	float				m_sFactor;
	Fvector				toggle_offsets[2];
	ref_sound			IdleSound;
	u32					SoundTimer = 0;
	bool				m_bNeedActivation;
	bool				m_bRadioInHand;
	bool				m_bRadioEnabled;
	CActor* CurrentActor;
	CInventoryOwner* CurrentInvOwner;
	UIWalkieTalkie* pUIWalkieTalkie;
};