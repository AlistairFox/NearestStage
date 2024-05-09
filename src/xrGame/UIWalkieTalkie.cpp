#include "StdAfx.h"
#include "UIWalkieTalkie.h"
#include "ui/UIXmlInit.h"
#include "ui/UIFrameWindow.h"
#include "ui/UIHelper.h"
#include "ui/UIEditBox.h"
#include "ui/UI3tButton.h"
#include "Actor.h"
#include "Inventory.h"


#define RADIO_I_XML "radio_itm.xml"

UIWalkieTalkie::UIWalkieTalkie(CWalkieTalkie* rd)
{
	Radio = rd;
}

UIWalkieTalkie::~UIWalkieTalkie()
{
	Radio = nullptr;
}

void UIWalkieTalkie::Init()
{
	xmlf.Load(CONFIG_PATH, UI_PATH, RADIO_I_XML);
	CUIXmlInit::InitWindow(xmlf, "main_wnd", 0, this);
	Back_Static = UIHelper::CreateStatic(xmlf, "background_static", this);
	m_background = UIHelper::CreateFrameWindow(xmlf, "background", Back_Static);
	DisableRadio = UIHelper::Create3tButton(xmlf, "off_radio_btn", m_background);
	EnableRadio = UIHelper::Create3tButton(xmlf, "en_radio_btn", m_background);
	AcceptHz = UIHelper::Create3tButton(xmlf, "accept_hz", m_background);
	current_hz = UIHelper::CreateTextWnd(xmlf, "current_hz", m_background);
	btn_exit = UIHelper::Create3tButton(xmlf, "btn_exit", m_background);
	min_hz = UIHelper::CreateTextWnd(xmlf, "min_hz", m_background);
	max_hz = UIHelper::CreateTextWnd(xmlf, "max_hz", m_background);
	max_distance = UIHelper::CreateTextWnd(xmlf, "max_distance", m_background);

	hz_editbox = UIHelper::CreateEditBox(xmlf, "hz_edit", m_background);
	hz_editbox->Init(5, true);
	InitCallBacks();
}

void UIWalkieTalkie::InitCallBacks()
{
	Register(DisableRadio);
	Register(EnableRadio);
	Register(AcceptHz);
	Register(btn_exit);

	AddCallback(EnableRadio, BUTTON_CLICKED, CUIWndCallback::void_function(this, &UIWalkieTalkie::EnableRadioClick));
	AddCallback(DisableRadio, BUTTON_CLICKED, CUIWndCallback::void_function(this, &UIWalkieTalkie::DisableRadioClick));
	AddCallback(AcceptHz, BUTTON_CLICKED, CUIWndCallback::void_function(this, &UIWalkieTalkie::AcceptClick));
	AddCallback(btn_exit, BUTTON_CLICKED, CUIWndCallback::void_function(this, &UIWalkieTalkie::OnBtnExit));
}

void UIWalkieTalkie::Show(bool status)
{
	inherited::Show(status);
	if (Actor())
	{
		if (status)
		{
			if (Radio)
			{
				char minhz[128];
				sprintf(minhz, "Минимальная поддерживаемая частота: [%d мГц.]", Radio->MinHZ);
				min_hz->SetText(minhz);

				char maxhz[128];
				sprintf(maxhz, "Максимальная поддерживаемая частота: [%d мГц.]", Radio->MaxHZ);
				max_hz->SetText(maxhz);

				char str[128];
				sprintf(str, "Текущая Частота: [%d мГц.]", Radio->CurrentHZ);
				current_hz->SetText(str);

				char max_dist[128];
				u16 tmp = Radio->MaxDistance;
				sprintf(max_dist, "Максимальная дальность сигнала: [%d М.]", tmp);
				max_distance->SetText(max_dist);
			}
		}
	}
}

void UIWalkieTalkie::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
	inherited::SendMessage(pWnd, msg, pData);
	CUIWndCallback::OnEvent(pWnd, msg, pData);
}

bool UIWalkieTalkie::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
	return inherited::OnMouseAction(x, y, mouse_action);;
}

bool UIWalkieTalkie::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
	if (WINDOW_KEY_PRESSED == keyboard_action)
	{
		if (dik == DIK_ESCAPE)
		{
			this->HideDialog();
			return true;
		}
	}

	return inherited::OnKeyboardAction(dik, keyboard_action);
}

void UIWalkieTalkie::ResetAll()
{
	inherited::ResetAll();
}

void UIWalkieTalkie::Update()
{
	inherited::Update();
	if (!Actor())
		return;
	if (!Radio)
		return;


	if (Radio->IsEnabled())
	{
		AcceptHz->Show(true);
		current_hz->Show(true);
		min_hz->Show(true);
		max_hz->Show(true);
		max_distance->Show(true);
		hz_editbox->Show(true);
		DisableRadio->Show(true);
		EnableRadio->Show(false);
	}
	else
	{
		AcceptHz->Show(false);
		current_hz->Show(false);
		min_hz->Show(false);
		max_hz->Show(false);
		max_distance->Show(false);
		hz_editbox->Show(false);

		DisableRadio->Show(false);
		EnableRadio->Show(true);
	}

}

void xr_stdcall UIWalkieTalkie::DisableRadioClick(CUIWindow* w, void* d)
{
	if (!Actor())
		return;

	if (!Radio)
		return;

	Radio->EnableRadio(false);
}

void xr_stdcall UIWalkieTalkie::EnableRadioClick(CUIWindow* w, void* d)
{
	if (!Actor())
		return;

	if (!Radio)
		return;

	Radio->EnableRadio(true);
}

void xr_stdcall UIWalkieTalkie::AcceptClick(CUIWindow* w, void* d)
{
	if (!Actor())
		return;

	if (!Radio)
		return;

	u16 hz;
	char str[128];
	strcpy(str, hz_editbox->GetText());
	sscanf(str, "%d", &hz);

	if (hz < 0)
		hz = 0;

	Radio->SetRadioHZ(hz);

	char str2[128];
	sprintf(str2, "Текущая Частота: [%d мГц.]", Radio->CurrentHZ);
	current_hz->SetText(str2);
}

void xr_stdcall UIWalkieTalkie::OnBtnExit(CUIWindow* w, void* d)
{
	this->HideDialog();
}



