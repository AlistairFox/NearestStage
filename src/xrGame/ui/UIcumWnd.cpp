#include "uieditbox.h"
#include "UIcumWnd.h"
#include "../xrEngine/xr_ioconsole.h"
#include "UIXmlInit.h"
#include "UIProgressBar.h"
#include "UIFrameLineWnd.h"
#include "UIFrameWindow.h"
#include "UIScrollBar.h"
#include "UIFixedScrollBar.h"
#include "UIScrollView.h"
#include "UICheckButton.h"
#include "UIHelper.h"
#include "stdafx.h"

#include "UIXmlInit.h"
#include "UIProgressBar.h"
#include "UIFrameLineWnd.h"
#include "UIFrameWindow.h"
#include "UIScrollBar.h"
#include "UIFixedScrollBar.h"
#include "UIScrollView.h"
#include "UICheckButton.h"
#include "UIHelper.h"
#include "UICharacterInfo.h"
#include "UIInventoryUtilities.h"

#define  PDA_CUM_XML		"pda_cum.xml"


void CUICumWnd::init()
{
	m_uiXml.Load(CONFIG_PATH, UI_PATH, PDA_CUM_XML);

	CUIXmlInit::InitWindow(m_uiXml, "main_wnd", 0, this);
	m_background = UIHelper::CreateFrameWindow(m_uiXml, "background", this);
	m_center_background = UIHelper::CreateFrameWindow(m_uiXml, "center_background", this);

	m_center_caption = UIHelper::CreateTextWnd(m_uiXml, "center_caption", this);

	string256 buf;
	xr_strcpy(buf, sizeof(buf), m_center_caption->GetText());
	xr_strcat(buf, sizeof(buf), CStringTable().translate("ui_logs_center_caption").c_str());
	m_center_caption->SetText(buf);
}
