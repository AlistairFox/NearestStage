#pragma once

#include "game_sv_mp.h"
#include "../xrEngine/pure_relcase.h"

class game_sv_freemp : public game_sv_mp, private pure_relcase
{

	typedef game_sv_mp inherited;

protected:
	CALifeSimulator* m_alife_simulator;

protected:
	xr_vector<u16> inventory_boxes;
	xr_map<u16, u32> inv_box;

public:

	u32								oldTime = 0;


									game_sv_freemp();
	virtual							~game_sv_freemp();
	
	virtual		void				Create(shared_str &options);


	virtual		bool				UseSKin() const { return false; }

	virtual		LPCSTR				type_name() const { return "freemp"; };
	void		ChangeGameTime(u32 day, u32 hour, u32 minute);
	void __stdcall		net_Relcase(CObject* O) {};

	// helper functions
	void									AddMoneyToPlayer(game_PlayerState* ps, s32 amount);
	void									SpawnItemToActor(u16 actorId, LPCSTR name);

	virtual		void				OnPlayerReady(ClientID id_who);
	virtual		void				OnPlayerConnect(ClientID id_who);
	virtual		void				OnPlayerConnectFinished(ClientID id_who);
	virtual		void				OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID);

	virtual		void				OnPlayerKillPlayer(game_PlayerState* ps_killer, game_PlayerState* ps_killed, KILL_TYPE KillType, SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA);

	virtual		void				OnEvent(NET_Packet &tNetPacket, u16 type, u32 time, ClientID sender);

	virtual		void				Update();
	virtual		ALife::_TIME_ID		GetStartGameTime();
	virtual		ALife::_TIME_ID		GetGameTime();

	virtual		ALife::_TIME_ID GetEnvironmentGameTime();

	virtual		BOOL				OnTouch(u16 eid_who, u16 eid_what, BOOL bForced = FALSE);

	virtual		void				on_death(CSE_Abstract* e_dest, CSE_Abstract* e_src);

	virtual		void				OnPlayerTrade(NET_Packet &P, ClientID const & clientID);
	virtual		void				OnTransferMoney(NET_Packet &P, ClientID const & clientID);

	virtual		void				RespawnPlayer(ClientID id_who, bool NoSpectator);


	virtual     void				SavePlayer(game_PlayerState* ps, CInifile* file);
	virtual     void				SavePlayerOutfits(game_PlayerState* ps, CInifile* outfsFile);
	virtual     bool				LoadPlayer(game_PlayerState* ps, CInifile* file);
	virtual     void				LoadPlayerOtfits(game_PlayerState* ps, CInifile* outfsFile);
	virtual		bool				HasSaveFile(game_PlayerState* ps);

	virtual		void				SaveInvBox(CSE_ALifeInventoryBox* box, CInifile* file);
	virtual		void				LoadInvBox(CSE_ALifeInventoryBox* box, CInifile* file);

	virtual		void				assign_RP(CSE_Abstract* E, game_PlayerState* ps_who);
	virtual		bool				load_position_RP(game_PlayerState* ps, Fvector& pos, Fvector& angle);
};
