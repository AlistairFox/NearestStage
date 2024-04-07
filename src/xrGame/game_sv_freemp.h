#pragma once

#include "game_sv_mp.h"
#include "../xrEngine/pure_relcase.h"

class game_sv_freemp : public game_sv_mp, private pure_relcase
{

	typedef game_sv_mp inherited;

protected:
	CALifeSimulator* m_alife_simulator;

protected:

public:


	struct inventory_boxes
	{
		CSE_Abstract* entity;
		bool loaded = false;
	};
	xr_map<u16, inventory_boxes> inventory_boxes_cse;

	struct Physics_objects
	{
		CSE_Abstract* entity;

	};
	xr_map<u16, Physics_objects> phy_objects_cse;


	virtual		void				OnAlifeCreate(CSE_Abstract* E);
	virtual void					OnObjectsCreate(CSE_Abstract* E);

									game_sv_freemp();
	virtual							~game_sv_freemp();
	
	virtual		void				Create(shared_str &options);


	virtual		bool				UseSKin() const { return false; }

	virtual		LPCSTR				type_name() const { return "freemp"; };
	void							ChangeGameTime(u32 day, u32 hour, u32 minute);
	void __stdcall					net_Relcase(CObject* O) {};

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

	virtual		ALife::_TIME_ID		GetEnvironmentGameTime();

	virtual		BOOL				OnTouch(u16 eid_who, u16 eid_what, BOOL bForced = FALSE);

	virtual		void				on_death(CSE_Abstract* e_dest, CSE_Abstract* e_src);

	virtual		void				OnPlayerTrade(NET_Packet &P, ClientID const & clientID);
	virtual		void				OnTransferMoney(NET_Packet &P, ClientID const & clientID);

	virtual		void				RespawnPlayer(ClientID id_who, bool NoSpectator);
				bool				Binnar_save_connect = false;

	///////////Info portions saving ////////////////
	 void					SavePlayerPortions(ClientID sender, shared_str info_id, bool add);
	 void					LoadPlayerPortions(game_PlayerState* ps, bool first);
	enum DialogsSavingChunks
	{
		INFO_PORTIONS_CHUNK = 0
	};
	xr_map<LPCSTR, xr_vector<shared_str>> Player_portions;
	///////////Info portions saving\ ////////////////

	///////////Dynamic Weather ////////////////
	u32 SaveWeatherTimer = 0;
	 void DynamicWeatherUpdate();
	 void ServerEnvSaveUpdateFile();
	 void ServerEnvSaveUpdateBin();

	u32 nowday;
	bool		weather_will_change = false;
	bool		first_update = false;
	bool		need_change_weather = false;

	enum EnvSavingChunks
	{
		ENV_CHUNK = 0
	};
	///////////Dynamic Weather /////////////


	//////////Dynamic Box Respawn //////////////
	CInifile* spawn_trash;
	CInifile* spawn_boosters;
	CInifile* spawn_weapons_devices;
	CInifile* spawn_ammo;
	CInifile* spawn_explosive;
	CInifile* spawn_weapons;
	CInifile* spawn_file;
	string_path spawn_config;
	 void				   SpawnInvBoxesItems(CSE_ALifeInventoryBox* box);
	 void				   OnStartSpawnInvBoxesItems(CSE_ALifeInventoryBox* box);
								   
	 u32 BoxResawnTimer = 0;
	 void				   DynamicBoxFileCreate();
	 void				   DynamicBoxUpdate();
	///////////Dynamic Box Respawn //////////////


	//////Dynamic Music /////////////
	 void				   DynamicMusicUpdate();
	 void				   DynamicMusicFileCreate();
	 void				   MusicPlay(CSE_ALifeObjectPhysic* obj, int pass, int obj_num);
	int numb = 0;
	int obj_count = 4;
	int	MusicCount;
	ref_sound snd;
	u32 lenght = 0;
	bool need_next_snd = true;
	int i = 0;
	bool first_play = true;
	bool need_stop_music = false;
	u32 stop_timer = 0;
	CInifile* Music;
	///////Dynamic Music /////////////


	///////Binnar InvBox Save ////////
	enum InvBoxChunks
	{
		INVBOX_ITEMS_CHUNK = 0
	};
	u32 InvBoxSaveTimer = 0;
			void				BinnarSaveInvBox(CSE_ALifeInventoryBox* box, string_path& filepath);
			void				BinnarLoadInvBox(CSE_ALifeInventoryBox* box, string_path& filepath);
	///////Binnar InvBox Save ////////
	CInifile* curr_box_file;
	string_path curr_invbox_name;
	///////File InvBox Save ////////
			void				SaveInvBox(CSE_ALifeInventoryBox* box, CInifile* file);
			void				LoadInvBox(CSE_ALifeInventoryBox* box, CInifile* file);
	///////File InvBox Save ////////

	bool use_mt_save = false;
	///////Binnar Player Save ////////
	enum ActorChunks
	{
		ACTOR_STATS_CHUNK = 0,
		ACTOR_DEVICES_CHUNK = 1,
		ACTOR_INV_ITEMS_CHUNK = 2,
		ACTOR_POS = 3,
		ACTOR_TEAM = 4,
		ACTOR_MONEY = 5
	};

	struct Players_stats
	{
		float satiety = 1.f;
		float thirst = 1.f;
		float radiation = 0.f;
	};
	xr_map<LPCSTR, Players_stats> Players_condition;

	 void SavePlayersConditions(float satiety, float thirst, float radiation, game_PlayerState* ps);

	 u32 PlayerSaveTimer = 0;
			void				BinnarSavePlayer(game_PlayerState* ps, string_path& filepath);
			bool				BinnarLoadPlayer(game_PlayerState* ps, string_path& filepath);
			bool				HasBinnarSaveFile(game_PlayerState* ps);
			bool				load_position_RP_Binnar(game_PlayerState* ps, Fvector& pos, Fvector& angle, float& health);
			void				MtSavePlayer();
	std::thread* playersave_thread;
	///////Binnar Player Save ////////


	///////File Player Save ////////
	virtual		bool				HasSaveFile(game_PlayerState* ps);
	virtual		void				assign_RP(CSE_Abstract* E, game_PlayerState* ps_who);
	virtual		bool				load_position_RP(game_PlayerState* ps, Fvector& pos, Fvector& angle);
	     void				SavePlayer(game_PlayerState* ps, CInifile* file);
	     bool				LoadPlayer(game_PlayerState* ps, CInifile* file);
	///////File Player Save ///////


	//////////Death items save ///////
		 struct SPlayersOnDeathBuff
		 {
			 s32 PlayerMoney = 0;
			 u8 Team = 8;
			 //Outfit
			 bool Outfit = false;
			 LPCSTR OutfitName;
			 float OutfitCond;
			 bool OutfUpg = false;
			 string2048 OutfitUpgrades;
			 u16 OutfitSlot;

			 //Helm
			 bool helm = false;
			 LPCSTR HelmetName;
			 float HelmetCond;
			 bool HelmUpg = false;
			 string2048 HelmetUpgrades;
			 u16 HelmSlot;

			 //Detector
			 bool detector = false;
			 LPCSTR DetectorName;
			 float DetectorCond;

			 //Weapon1
			 bool weapon1 = false;
			 LPCSTR Weapon1Sect;
			 float Weapon1Cond;
			 bool weapon1Upgr = false;
			 string2048 Weapon1Upgrades;
			 u8 Weapon1AddonState;
			 u8 Weapon1CurScope;
			 u16 Weapon1Slot;

			 //Weapon2
			 bool weapon2 = false;
			 LPCSTR Weapon2Sect;
			 float Weapon2Cond;
			 bool weapon2Upgr = false;
			 string2048 Weapon2Upgrades;
			 u8 Weapon2AddonState;
			 u8 Weapon2CurScope;
			 u16 Weapon2Slot;

		 };	std::map<LPCSTR, SPlayersOnDeathBuff> MPlayersOnDeath;


		 void SavePlayersOnDeath(game_PlayerState* ps);
		 void LoadPlayersOnDeath(game_PlayerState* ps);
		 void SavePlayerOnDisconnect(LPCSTR Name, string_path path);
		 void ClearPlayersOnDeathBuffer(LPSTR name);
	////////Death items save ////////
};

extern BOOL binar_save;
extern int save_time;
extern int save_time2;
extern int save_time3;
extern int box_respawn_time;
extern BOOL		set_next_music;
extern	BOOL	save_thread;