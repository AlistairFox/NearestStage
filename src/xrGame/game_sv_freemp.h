#pragma once

#include "game_sv_mp.h"
#include "../xrEngine/pure_relcase.h"
#include "server_progress_saver.h"

class CProgressSaver;

class game_sv_freemp : public game_sv_mp, private pure_relcase
{
	typedef game_sv_mp inherited;

protected:
	CALifeSimulator* m_alife_simulator;
public:

	CProgressSaver* Saver = nullptr;

	struct inventory_boxes
	{
		CSE_Abstract* entity = nullptr;
		bool loaded = false;
	};
	xr_map<u16, inventory_boxes> inventory_boxes_cse;

	xr_map<u16, CSE_Abstract*> phy_objects_cse;

	virtual		void				OnAlifeCreate(CSE_Abstract* E);
	virtual		void				OnObjectsCreate(CSE_Abstract* E);

									game_sv_freemp();
	virtual							~game_sv_freemp();
	
	virtual		void				Create(shared_str &options);


	virtual		bool				UseSKin() const { return false; }

	virtual		LPCSTR				type_name() const { return "freemp"; };
				void				ChangeGameTime(u32 day, u32 hour, u32 minute);
				void __stdcall		net_Relcase(CObject* O) {};

	// helper functions
				void				AddMoneyToPlayer(game_PlayerState* ps, s32 amount);
				void				SpawnItemToActor(u16 actorId, LPCSTR name);

	virtual		void				OnPlayerReady(ClientID id_who);
	virtual		void				OnPlayerConnect(ClientID id_who);
	virtual		void				OnPlayerConnectFinished(ClientID id_who);
	virtual		void				OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID, u16 StaticID) override;

	virtual		void				OnPlayerKillPlayer(game_PlayerState* ps_killer, game_PlayerState* ps_killed, KILL_TYPE KillType, SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA);

	virtual		void				OnEvent(NET_Packet &tNetPacket, u16 type, u32 time, ClientID sender);

	virtual		void				Update();
	virtual		ALife::_TIME_ID		GetStartGameTime();
	virtual		ALife::_TIME_ID		GetGameTime();

	virtual		ALife::_TIME_ID		GetEnvironmentGameTime();

	virtual		BOOL				OnTouch(u16 eid_who, u16 eid_what, BOOL bForced = FALSE);

	virtual		void				on_death(CSE_Abstract* e_dest, CSE_Abstract* e_src);

	virtual		void				OnPlayerTrade(NET_Packet &P, ClientID const & clientID);
	virtual		void				OnTransferMoney(NET_Packet &P, ClientID const & clientID);

	virtual		void				RespawnPlayer(ClientID id_who, bool NoSpectator);

#pragma region Dynamic Weather System
				void				DynamicWeatherUpdate();

				u32			nowday;
				bool		weather_will_change = false;
				bool		first_update = false;
				bool		need_change_weather = false;
	///////////Dynamic Weather /////////////
#pragma endregion

#pragma region InventoryBox Dynamic Respawn System
	CInifile*	spawn_trash;
	CInifile*	spawn_boosters;
	CInifile*	spawn_weapons_devices;
	CInifile*	spawn_ammo;
	CInifile*	spawn_explosive;
	CInifile*	spawn_weapons;
	CInifile*	spawn_file;
	string_path spawn_config;
				void				   SpawnInvBoxesItems(CSE_ALifeInventoryBox* box);
				void				   OnStartSpawnInvBoxesItems(CSE_ALifeInventoryBox* box);
								   
	 u32		BoxResawnTimer = 0;
				void				   DynamicBoxFileCreate();
				void				   DynamicBoxUpdate();
#pragma endregion

#pragma region Dynamic Music System
				void				   DynamicMusicUpdate();
				void				   DynamicMusicFileCreate();
				void				   MusicPlay(CSE_ALifeObjectPhysic* obj, int pass, int obj_num);
	int			numb = 0;
	int			obj_count = 4;
	int			MusicCount;
	ref_sound	snd;
	u32			lenght = 0;
	bool		need_next_snd = true;
	int			i = 0;
	bool		first_play = true;
	bool		need_stop_music = false;
	u32			stop_timer = 0;
	CInifile*	Music;
#pragma endregion

	virtual		void				assign_RP(CSE_Abstract* E, game_PlayerState* ps_who);

};
extern int		box_respawn_time;
extern BOOL		set_next_music;
extern BOOL		af_debug_loggining;