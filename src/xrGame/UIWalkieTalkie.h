#pragma once

#include "ui/UIDialogWnd.h"
#include "ui/UIWndCallback.h"
#include "../xrEngine/xr_input.h"
#include "Actor.h"
#include "WalkieTalkie.h"
#include "ui/UIStatic.h"

class CUIXmlInit;
class CUIFrameWindow;
class CUITextWnd;
class CUI3tButton;
class CUIEditBox;
class CWalkieTalkie;
class CUIStatic;

class UIWalkieTalkie : public CUIDialogWnd, public CUIWndCallback
{
	typedef CUIDialogWnd inherited;

	CUIXml xmlf;
	CUIFrameWindow* m_background;
	CUI3tButton* DisableRadio;
	CUI3tButton* EnableRadio;
	CUI3tButton* AcceptHz;
	CUI3tButton* btn_exit;

	CUIEditBox* hz_editbox;
	CUITextWnd* current_hz;
	CUITextWnd* min_hz;
	CUITextWnd* max_hz;
	CUITextWnd* max_distance;
	CUIStatic* Back_Static;

public:
	UIWalkieTalkie(CWalkieTalkie* R);
	virtual					~UIWalkieTalkie();
	void			Init();
	void			InitCallBacks();
	virtual void			Show(bool status);
	virtual void			ResetAll();

	virtual void			Update() override;
			void xr_stdcall DisableRadioClick(CUIWindow* w, void* d);
			void xr_stdcall EnableRadioClick(CUIWindow* w, void* d);
			void xr_stdcall AcceptClick(CUIWindow* w, void* d);
			void xr_stdcall OnBtnExit(CUIWindow* w, void* d);
	virtual void			SendMessage(CUIWindow* pWnd, s16 msg, void* pData);
	virtual bool			OnMouseAction(float x, float y, EUIMessages mouse_action);
	virtual bool			OnKeyboardAction(int dik, EUIMessages keyboard_action);

private:
	CWalkieTalkie* Radio;
};