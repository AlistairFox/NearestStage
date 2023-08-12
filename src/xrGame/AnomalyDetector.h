#pragma once
#include "inventory_item_object.h"
#include "../xrEngine/Feel_Touch.h"
#include "hudsound.h"
#include "Battery.h"

class CCustomZone;
//îïèñàíèå òèïà çîíû
struct ZONE_TYPE
{
	//èíòåðâàë ÷àñòîò îòûãðûâàíèÿ çâóêà
	float		min_freq;
	float		max_freq;
	//çâóê ðåàêöèè äåòåêòîðà íà êîíêðåòíóþ çîíó
	HUD_SOUND_ITEM	detect_snds;

	shared_str	zone_map_location;
};

//îïèñàíèå çîíû, îáíàðóæåííîé äåòåêòîðîì
struct ZONE_INFO
{
	u32								snd_time;
	//òåêóùàÿ ÷àñòîòà ðàáîòû äàò÷èêà
	float							cur_freq;
	//particle for night-vision mode
	CParticlesObject* pParticle;

	ZONE_INFO();
	~ZONE_INFO();
};

class CInventoryOwner;

class CDetectorAnomaly :
	public CInventoryItemObject,
	public Feel::Touch
{
	//typedef	CInventoryItemObject	inherited;
	using inherited = CInventoryItemObject;
public:
	CDetectorAnomaly(void);
	virtual ~CDetectorAnomaly(void);

	virtual BOOL net_Spawn(CSE_Abstract* DC);
	virtual void Load(LPCSTR section);

	virtual void OnH_A_Chield();
	virtual void OnH_B_Independent(bool just_before_destroy);

	void OnEvent(NET_Packet& P, u16 type);

	virtual void shedule_Update(u32 dt);
	virtual void UpdateCL();

	virtual void feel_touch_new(CObject* O);
	virtual void feel_touch_delete(CObject* O);
	virtual BOOL feel_touch_contact(CObject* O);

	void TurnOn();
	void TurnOff();
	bool IsWorking() { return m_bWorking; }
	virtual void 	OnMoveToSlot(const SInvItemPlace& prev);
	virtual void 	OnMoveToRuck(const SInvItemPlace& prev);

	void UpdateChargeLevel(void);
	virtual void save(NET_Packet& output_packet);
	virtual void load(IReader& input_packet);
	float GetCurrentChargeLevel(void) const;
	void SetCurrentChargeLevel(float val);
	float GetUnchargeSpeed(void) const;
	void Recharge(float val);

	float	m_fMaxChargeLevel;
	float	m_fCurrentChargeLevel;
	float	m_fUnchargeSpeed;

	bool m_bWorking;

protected:
	void StopAllSounds();

	float m_fRadius;

	//åñëè õîçÿèí òåêóùèé àêòåð
	CActor* m_pCurrentActor;
	CInventoryOwner* m_pCurrentInvOwner;

	//èíôîðìàöèÿ îá îíàðóæèâàåìûõ çîíàõ
	DEFINE_MAP(CLASS_ID, ZONE_TYPE, ZONE_TYPE_MAP, ZONE_TYPE_MAP_IT);
	ZONE_TYPE_MAP m_ZoneTypeMap;

	//ñïèñîê îáíàðóæåííûõ çîí è èíôîðìàöèÿ î íèõ
	DEFINE_MAP(CCustomZone*, ZONE_INFO, ZONE_INFO_MAP, ZONE_INFO_MAP_IT);
	ZONE_INFO_MAP m_ZoneInfoMap;

	shared_str						m_nightvision_particle;

protected:
	u32					m_ef_detector_type;

public:
	virtual u32			ef_detector_type() const;
};