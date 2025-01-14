#pragma once
#include "game_cl_mp.h"

class CVoiceChat;
class CUIGameFMP;

class game_cl_freemp :public game_cl_mp
{
private:
	typedef game_cl_mp inherited;


public:
	CVoiceChat* m_pVoiceChat = nullptr;
	CUIGameFMP* m_game_ui;
			game_cl_freemp();
	virtual	~game_cl_freemp();

	virtual LPCSTR		GetTeamColor(u32 /*team*/) const { return "%c[255,255,240,190]"; }
	virtual u32				GetTeamColor_u32(u32 /*team*/) const { return color_rgba(255, 240, 190, 255); }

	virtual CUIGameCustom* createGameUI();
	virtual void SetGameUI(CUIGameCustom*);

	virtual void TranslateGameMessage(u32 msg, NET_Packet& P);

	virtual	void net_import_state(NET_Packet& P);
	virtual	void net_import_update(NET_Packet& P);
	
	virtual void shedule_Update(u32 dt);

	virtual void OnRender();

	virtual	bool OnKeyboardPress(int key);
	virtual	bool OnKeyboardRelease(int key);

	virtual LPCSTR GetGameScore(string32&	score_dest);
	virtual bool Is_Rewarding_Allowed()  const { return false; };

	virtual void OnConnected();

	virtual void OnScreenResolutionChanged();

	void OnChatMessage(NET_Packet* P);

	void SendHelloMsg();

private:
	void OnVoiceMessage(NET_Packet* P);
	bool adm_wallhack = false;
};

