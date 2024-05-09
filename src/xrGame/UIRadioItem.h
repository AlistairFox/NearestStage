#pragma once
#pragma once

#include "ui/UIDialogWnd.h"
#include "ui/UIWndCallback.h"
#include "../xrEngine/xr_input.h"
#include "Actor.h"
#include "RadioItem.h"
#include "ui/UIStatic.h"

class CUIXmlInit;
class CUIFrameWindow;
class CUITextWnd;
class CUI3tButton;
class CUIEditBox;
class CRadioItem;
class CUIStatic;

class UIRadioItem : public CUIDialogWnd, public CUIWndCallback
{
	typedef CUIDialogWnd inherited;

	CUIXml xmlf;
	CUIFrameWindow* m_background;
	CUI3tButton* OffRadioBtn;
	CUI3tButton* AcceptHz;
	CUI3tButton* btn_exit;

	CUIEditBox* hz_editbox;
	CUITextWnd* current_hz;
	CUITextWnd* min_hz;
	CUITextWnd* max_hz;
	CUITextWnd* max_distance;
	CUIStatic* Back_Static;

public:
	UIRadioItem(CRadioItem* R);
	virtual					~UIRadioItem();
	void			Init();
	void			InitCallBacks();
	virtual void			Show(bool status);
	virtual void			ResetAll();
	void xr_stdcall OffRadioClick(CUIWindow* w, void* d);
	void xr_stdcall AcceptClick(CUIWindow* w, void* d);
	void xr_stdcall OnBtnExit(CUIWindow* w, void* d);
	virtual void			SendMessage(CUIWindow* pWnd, s16 msg, void* pData);
	virtual bool			OnMouseAction(float x, float y, EUIMessages mouse_action);
	virtual bool			OnKeyboardAction(int dik, EUIMessages keyboard_action);

private:
	CRadioItem* Radio;
};