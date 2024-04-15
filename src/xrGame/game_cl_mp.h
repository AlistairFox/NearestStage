#pragma once

#include "game_cl_base.h"
#include "ui_defs.h"
#include "Spectator.h"
#include "../xrCore/fastdelegate.h"

class CUIWindow;
class CUISpeechMenu;
class CUIMessageBoxEx;

namespace award_system
{
	class reward_manager;
}//namespace award_system


struct SND_Message{
	ref_sound	pSound;
	u32			priority;
	u32			SoundID;
	u32			LastStarted;
	bool operator == (u32 ID){return SoundID == ID;}
	void Load(u32 const ID, u32 const prior, LPCSTR name)
	{
		SoundID = ID;
		priority = prior;
		pSound.create(name,st_Effect,sg_SourceType);
		LastStarted = 0;
	}
	~SND_Message()
	{
		SoundID = 0;
		priority = 0;
		pSound.destroy();
	}
};

struct cl_TeamStruct
{
	shared_str			caSection;		// имя секции комманды
	//-----------------------------------
	ui_shader			IndicatorShader;
	ui_shader			InvincibleShader;

	Fvector				IndicatorPos;
	float				Indicator_r1;
	float				Indicator_r2;
};

DEF_DEQUE(CL_TEAM_DATA_LIST, cl_TeamStruct);

struct cl_Message_Sound
{
	ref_sound	mSound_Voice;
	ref_sound	mSound_Radio;
};

DEF_VECTOR	(TEAMSOUND, cl_Message_Sound);

struct cl_Menu_Message {
	shared_str		pMessage;	
	DEF_VECTOR	(SOUND_VARIANTS, TEAMSOUND);
	SOUND_VARIANTS		aVariants;
};

struct cl_MessageMenu
{
	CUISpeechMenu*		m_pSpeechMenu;
	DEF_VECTOR	(MENUMESSAGES, cl_Menu_Message);
	MENUMESSAGES		m_aMessages;	

	bool operator == (CUISpeechMenu* pMenu){return pMenu == m_pSpeechMenu;}
};

struct Bonus_Struct
{
	shared_str	BonusName;
	shared_str	BonusTypeName;
	shared_str	MoneyStr;
	int			Money;	
	ui_shader	IconShader;
	//ref_shader	IconShader;
	xr_vector<Frect>	IconRects;
	Bonus_Struct()
	{
		BonusTypeName	= "";
		BonusName = "";
		MoneyStr = "";
		Money = 0;
		//IconShader = NULL;
		IconRects.clear();
	}
	~Bonus_Struct()
	{
		//if (IconShader)
		//	IconShader.destroy();
		//IconShader = NULL;
		IconRects.clear();
	}

	bool operator == (LPCSTR TypeName){return !xr_strcmp(BonusTypeName.c_str(), TypeName);}
};

class CUIVotingCategory;
class CUIVote;
class CUIMessageBoxEx;
class CUIMpAdminMenu;

namespace award_system
{
class reward_event_generator;
}


class game_cl_mp :public game_cl_GameState
{
	typedef game_cl_GameState	inherited;

private:
	u32		Color_Teams_u32[3] = { color_rgba(255,240,190,255), color_rgba(64,255,64,255), color_rgba(64,64,255,255) };
	LPCSTR	Color_Teams[3] = { "%c[255,255,240,190]", "%c[255,64,255,64]", "%c[255,64,64,255]" };

	LPCSTR Color_Main = "%c[255,192,192,192]";
	LPCSTR Color_Red = "%c[255,255,1,1]";
	LPCSTR Color_Green = "%c[255,1,255,1]";

	u32		Color_Neutral_u32 = color_rgba(255, 0, 255, 255);

protected:

	CL_TEAM_DATA_LIST				TeamList;

	DEF_VECTOR(SNDMESSAGES, SND_Message*);
	SNDMESSAGES						m_pSndMessages;
	bool							m_bJustRestarted;
	DEF_VECTOR(SNDMESSAGESINPLAY, SND_Message*);
	SNDMESSAGESINPLAY				m_pSndMessagesInPlay;

	DEF_VECTOR(BONUSES, Bonus_Struct);
	BONUSES							m_pBonusList;

	bool							m_bVotingActive;
	CUIVotingCategory*				m_pVoteStartWindow;
	CUIMpAdminMenu*					m_pAdminMenuWindow;
	CUIVote*						m_pVoteRespondWindow;
	CUIMessageBoxEx*				m_pMessageBox;
	BOOL							m_bSpectatorSelected;
		

protected:
	virtual LPCSTR		GetTeamColor(u32 team) const { return Color_Teams[team]; }
	virtual u32				GetTeamColor_u32(u32 team) const { return Color_Teams_u32[team]; }

	virtual void			LoadTeamData			(const shared_str&	TeamName);
	
	virtual	void			ChatSay					(LPCSTR	phrase, bool bAll);

	virtual	void			OnChatMessage			(NET_Packet* P);
	virtual	void			OnWarnMessage			(NET_Packet* P);
	virtual	void			OnRadminMessage			(u16 type, NET_Packet* P);

	virtual void			UpdateMapLocations		() {};

	ui_shader				m_EquipmentIconsShader;
	ui_shader				m_KillEventIconsShader;
	ui_shader				m_RadiationIconsShader;
	ui_shader				m_BloodLossIconsShader;
	ui_shader				m_RankIconsShader;

	virtual const ui_shader&		GetEquipmentIconsShader	();
	virtual const ui_shader&		GetKillEventIconsShader	();
	virtual const ui_shader&		GetRadiationIconsShader	();
	virtual const ui_shader&		GetBloodLossIconsShader	();
	virtual const ui_shader&		GetRankIconsShader();

	virtual void			OnPlayerKilled			(NET_Packet& P);

	virtual bool			NeedToSendReady_Actor(int key, game_PlayerState* ps) { return false; };
	virtual bool			NeedToSendReady_Spectator(int key, game_PlayerState* ps) { return false; };

	virtual	void			LoadSndMessage			(LPCSTR caSection, LPCSTR caLine, u32 ID);
	virtual		void				LoadSndMessages				();
	virtual		void				UpdateSndMessages		();

	u8			m_u8SpectatorModes;
	bool		m_bSpectator_FreeFly;
	bool		m_bSpectator_FirstEye;
	bool		m_bSpectator_LookAt;
	bool		m_bSpectator_FreeLook;
	bool		m_bSpectator_TeamCamera;

	u32			m_cur_MenuID;
	
	virtual		void		LoadBonuses				();

	award_system::reward_event_generator*	m_reward_generator;
				void				ReInitRewardGenerator	(game_PlayerState* local_ps);
public:
									game_cl_mp();
	virtual							~game_cl_mp();


	void xr_stdcall					OnBuySpawn				(CUIWindow* pWnd, void* p);
	virtual		void				TranslateGameMessage	(u32 msg, NET_Packet& P);

	virtual		bool				OnKeyboardPress			(int key);

	virtual		bool				CanBeReady() { return true; };
	virtual		void				shedule_Update			(u32 dt);
				bool				IsLocalPlayerInitialized() const;

	//// VOTING
	virtual		bool				IsVotingActive			()	{ return m_bVotingActive; };
	virtual		void				SetVotingActive			( bool Active )	{ m_bVotingActive = Active; };
	virtual		void				SendStartVoteMessage	(LPCSTR args);
	virtual		void				SendVoteYesMessage		();
	virtual		void				SendVoteNoMessage		();
				void				VotingBegin				();
				void				Vote					();
				void				OnCantVoteMsg			(LPCSTR Text);
	virtual		void				OnVoteStart				(NET_Packet& P);
	virtual		void				OnVoteStop				(NET_Packet& P);
	virtual		void				OnVoteEnd				(NET_Packet& P);
				void				GetActiveVoting			();
	virtual		void				OnPlayerChangeName		(NET_Packet& P);
	virtual		void				OnPlayerVoted			(game_PlayerState* ps);
	virtual		void				OnSpectatorSelect		();
	virtual		void				OnSkinMenuBack			() {};
	virtual		void				OnTeamMenuBack			() {};
	virtual		void				OnTeamMenu_Cancel		() {};
	virtual		void				OnMapInfoAccept			() {};
	virtual		void				OnSkinMenu_Ok			() {};
	virtual		void				OnSkinMenu_Cancel		() {};
	virtual		void				OnBuySpawnMenu_Ok		() {};
	virtual		void				OnSellItemsFromRuck		() {};
	
	virtual		void				OnTeamSelect			(int Result){};
	virtual		void				OnBuyMenu_Ok			()			{};
	virtual		void				OnBuyMenu_Cancel		()			{};


	virtual		void				OnGameMenuRespond				(NET_Packet& P);
	virtual		void				OnGameMenuRespond_Spectator		(NET_Packet& P) {};
	virtual		void				OnGameMenuRespond_ChangeTeam	(NET_Packet& P) {};
	virtual		void				OnGameMenuRespond_ChangeSkin	(NET_Packet& P) {};
	virtual		void				OnGameRoundStarted				();
	

	virtual		void				OnSwitchPhase			(u32 old_phase, u32 new_phase);	
	virtual		void				net_import_update		(NET_Packet& P);
	virtual		void				net_import_state		(NET_Packet& P);
	virtual		void				OnRankChanged			(u8 OldRank);
	virtual		void				OnTeamChanged			() {};
	virtual		void				OnMoneyChanged			() {};
	virtual		void				OnEventMoneyChanged		(NET_Packet& P);

	virtual		void				OnSwitchPhase_InProgress();

	virtual		u8					GetTeamCount			() { return 0; };
	virtual		s16					ModifyTeam				(s16 Team)	{return Team;};

	virtual		bool				Is_Spectator_TeamCamera_Allowed () {return m_bSpectator_TeamCamera && !Level().IsDemoPlay();};
	virtual		bool				Is_Spectator_Camera_Allowed			(CSpectator::EActorCameras Camera);
	virtual		bool				Is_Rewarding_Allowed			() const = 0;
				
				void				SendPlayerStarted();
	virtual		void				OnConnected				();
	virtual		LPCSTR				GetGameScore			(string32&	score_dest) = 0;
	
	award_system::reward_event_generator*	get_reward_generator() const { return m_reward_generator; };

				void				AddRewardTask	(u32 const award_id);

				void				AddSoundMessage		(LPCSTR sound_name, u32 const sound_priority, u32 const soundID);
				void				PlaySndMessage		(u32 ID);
private:
				u8*					buffer_for_compress;
				u32					buffer_for_compress_size;
				CMemoryWriter		upload_memory_writer;
				void				reinit_compress_buffer(u32 need_size);
				void				deinit_compress_buffer();
				
				award_system::reward_manager*	m_reward_manager;
				
private:
				bool							m_ready_to_open_buy_menu;
public:
				bool				is_buy_menu_ready				() const { return m_ready_to_open_buy_menu; };
				void				set_buy_menu_not_ready			() { m_ready_to_open_buy_menu = false; };


//-------------------------------------------------------------------------------------------------
				static void	generate_file_name(string_path& file_name, LPCSTR file_suffix, SYSTEMTIME const& date_time);
				static LPCSTR	make_file_name(LPCSTR session_id, string_path & dest);
//-------------------------------------------------------------------------------------------------
#include "game_cl_mp_messages_menu.h"
};
