#include "StdAfx.h"
#include"embedded_editor_opti.h"
#include"embedded_editor_helper.h"
#include "../xrEngine/xr_ioconsole.h"
#include <imgui.h>
#include "CustomOutfit.h"


void ShowOptMenu(bool& show)
{
	ImguiWnd wnd("Opts Menu", &show);
	if (wnd.Collapsed)
		return;

	char* nick;
	float money;
	ImGui::InputText(u8"Nickname", nick, 1024);

	ImGui::SliderFloat("Money", &money, 0, 500000);

	if (ImGui::Button("Transfer Money"))
	{
		std::string str = "transfer_money ";
		str += std::to_string(money);
		Console->Execute(str.c_str());
	}
}