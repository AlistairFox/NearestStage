#pragma once
#include "game_sv_freemp.h"
#include "Level.h"
#include "Actor.h"
#include "Inventory.h"
#include "xrServer_Objects_ALife.h"

class game_sv_freemp;
class CProgressSaver
{
public:
	CProgressSaver(game_sv_freemp* Game);
	~CProgressSaver();
	game_sv_freemp* fmp;
	void				SavingUpdate();

#pragma region Inventory Boxes Saving
	enum InvBoxChunks
	{
		INVBOX_ITEMS_CHUNK = 0
	};
	u32 InvBoxFillTimer = 0;
	u32 InvBoxSaveTimer = 0;
	void				BinnarLoadInvBox(CSE_ALifeInventoryBox* box);
	void				FillInvBoxBuffer(CSE_ALifeInventoryBox* box);
	CInifile* curr_box_file;
	string_path curr_invbox_name;
#pragma endregion
	
#pragma region Save Thread Tasks
		std::thread* SaverThread;
		xrCriticalSection csSaving;
		struct SItem
		{
			string128 ItemSect;
			u16 ItemSlot;
			float ItemCond;
			bool IsWeaponAmmo = false;
			u16 AmmoBoxCurr = 0;
			bool IsWeapon = false;
			u16 AmmoElapsed = 0;
			u8 AmmoType = 0;
			u8 AddonState = 0;
			u8 CurrScope = 0;
			bool HasUpgr = false;
			string2048 Uphrades;
		};

		struct InvBox
		{
			string512 box_path;
			std::vector<SItem> Items;
		};

		struct PlayerStats
		{
			s32 money;
			float satiety = 1.f;
			float thirst = 1.f;
			float radiation = 1.f;
			u8 team = 8;
			bool SetPossition = false;
			Fvector3 pos;
			Fvector3 angle;
		};

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
			u16 DetectorSlot;

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

		};

		struct Players
		{
			PlayerStats Stats;
			std::vector<SItem> Items;
			string512 PlayerPath;
			string128 PlayerName;
			xr_vector<shared_str> InfoPortions;
		};

		struct OnDeathDisconnect
		{
			SPlayersOnDeathBuff Items;
			string512 PlayerPath;
		};

		struct GlobalServerData
		{
			string128 Time;
			string128 Data;
			string128 Weather;
			string512 GSDPath;
		};

		struct SThreadTask
		{
			InvBox* box;
			Players* players;
			OnDeathDisconnect* DisconnectBuf;
			GlobalServerData* ServerData;
		};

		std::vector<SThreadTask>  ThreadTasks;
		void				SaveThreadWorker();
#pragma endregion

#pragma region Player Saving
	enum ActorChunks
	{
		ACTOR_STATS_CHUNK = 0,
		ACTOR_DEVICES_CHUNK,
		ACTOR_INV_ITEMS_CHUNK,
		ACTOR_POS,
		ACTOR_TEAM,
		ACTOR_MONEY,
		INFO_PORTIONS_CHUNK
	};

	struct Players_stats
	{
		float satiety = 1.f;
		float thirst = 1.f;
		float radiation = 0.f;
	};
	xr_map<u16, Players_stats> Players_condition;

	u32 PlayerSaveTimer = 0;

	void				SavePlayersConditions(float satiety, float thirst, float radiation, game_PlayerState* ps);
	bool				RemovePlayerSave(game_PlayerState* ps);
	void				FillPlayerBuffer(game_PlayerState* ps);
	bool				BinnarLoadPlayer(game_PlayerState* ps);
	bool				HasBinnarSaveFile(game_PlayerState* ps);
	bool				load_position_RP_Binnar(game_PlayerState* ps, Fvector& pos, Fvector& angle);

#pragma region OnDeath Items Saving
	void				LoadPlayersOnDeath(game_PlayerState* ps);
	void				FillPlayerOnDisconnect(u16 StaticID, string_path path);
	void				ClearPlayersOnDeathBuffer(u16 StaticID);

	std::map<u16, SPlayersOnDeathBuff> MPlayersOnDeath;
#pragma endregion

#pragma region Players Tasks
	void				SavePlayerPortions(ClientID sender, shared_str info_id, bool add);
	void				LoadPlayerPortions(game_PlayerState* ps);
	xr_map<u16, xr_vector<shared_str>> Player_portions;
#pragma endregion

#pragma endregion

#pragma region Server Environment
	enum EnvSavingChunks
	{
		ENV_CHUNK = 0
	};
	void				FillServerEnvBuffer();
	u32 SaveWeatherTimer = 0;
	bool				LoadServerEnvironment(u32 &hours, u32&minutes,
	u32&seconds, u32&days, u32&months, u32&years);
#pragma endregion

};
extern int save_time;
extern int save_time2;
extern int save_time3;
extern int save_time4;