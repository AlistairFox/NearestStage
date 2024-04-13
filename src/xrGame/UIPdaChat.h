#pragma once

#include "ui/UIWindow.h"
#include "ui/UIWndCallback.h"
#include "../xrEngine/xr_input.h" 
#include "Actor.h"

class CUIXmlInit;
class CUIFrameWindow;
class CUIScrollView;
class CUITextWnd;
class CUI3tButton;
class CUIEditBox;
class CUICharacterInfo;
struct GAME_NEWS_DATA;

class UIPdaChat : public CUIWindow, public CUIWndCallback
{
	typedef CUIWindow	inherited;

	CUIXml xml;
	CActor* LocalActor;
	CActor* SecondActor;

	ClientID LocalActorCL;
	ClientID SecondActorCL;

	CUI3tButton* send_money_button;
	CUI3tButton* send_msg_to_user;
	CUIScrollView* chat_users;
	CUIScrollView* chat_text;

	CUIFrameWindow* m_background;
	CUIFrameWindow* m_background_players;
	CUIFrameWindow* m_background_chat;
	CUIFrameWindow* m_background_chat_text;


	CUICharacterInfo* infoActor;
	CUICharacterInfo* infoPartner;

	CUITextWnd* CaptionOnline;
	CUITextWnd* player_1_money;
	CUITextWnd* player_2_money;

	CUIEditBox* chat_editbox;
	CUIEditBox* money_editbox;

	//GLOBAL CHAT
	CUITextWnd* CaptionMode;
	CUITextWnd* cap_text_active;
	CUI3tButton* switch_anonimous;
	CUI3tButton* switch_mode_button;
	CUI3tButton* update_list_btn;



	virtual bool	OnMouseAction(float x, float y, EUIMessages mouse_action);
	virtual bool			OnKeyboardAction(int dik, EUIMessages keyboard_action);

	void xr_stdcall     button_click_send_msg(CUIWindow* w, void* d);
	void				Send_msg();
	void xr_stdcall     button_click_send_money(CUIWindow* w, void* d);
	void xr_stdcall	    button_click_mode_switch(CUIWindow* w, void* d);
	void xr_stdcall button_click_anonimous_mode_switch(CUIWindow* w, void* d);
	void xr_stdcall button_update_click(CUIWindow* w, void* d);

	virtual void		SendMessage(CUIWindow* pWnd, s16 msg, void* pData);
	void AddNewsData(GAME_NEWS_DATA data, ClientID PlayerID);
	void SendPacket(GAME_NEWS_DATA data);

	u32 old_second_id = 0;
	u8 old_chat = 0;

	bool Anonymous = false;
	bool ModeGlobalChat = true;

public:
	bool change_focus = false;
	xr_map<ClientID, xr_vector<GAME_NEWS_DATA>> news_data;

	xr_vector<GAME_NEWS_DATA> global_chat;

	UIPdaChat();
	virtual ~UIPdaChat();

	void			Init();

	void			InitCallBacks();
	virtual void 	Show(bool status);
	virtual void	Update();
	std::map<LPCSTR, u16> players_map;
	virtual void	ResetAll();
	void RecivePacket(NET_Packet& P);

};