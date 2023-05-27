#pragma once

#include "uieditbox.h"

class CUIMapWnd;
class CUIStatic;
class CGameTask;
class CUITaskItem;
class CUI3tButton;
class CUIFrameLineWnd;
class CUIFrameWindow;
class CUICheckButton;
class UITaskListWnd;
class UIMapLegend;
class UIHint;
class CUIXml;
class CUIGameLog;
class CUIEditBox;
class CUITextWnd;

class CUICumWnd : public CUIWindow, public CUIWndCallback
{
private:
	typedef CUIWindow	inherited;
	CUICheckButton* send_money;
	CUIFrameWindow* m_background;
	CUIFrameWindow* m_center_background;
	CUITextWnd* m_center_caption;

	CUIXml				m_uiXml;

public:
	void			init();

protected:
	CUIEditBox* UIEditBox;
	CUITextWnd* UIPrefix;
};