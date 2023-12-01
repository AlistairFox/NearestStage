#include "StdAfx.h"
#include "embedded_editor/embedded_editor_main.h"
#include "embedded_editor/embedded_editor_helper.h"
#include "../xrEngine/xr_ioconsole.h"
#include <imgui.h>
#include "CustomOutfit.h"
#include "embedded_editor_npc.h"



void ShowNpcEditor(bool& show)
{
	ImguiWnd wnd("NPC EDITOR", &show);
	if (wnd.Collapsed)
		return;

	ImGui::Text(u8"Functions");

	string256 text;
	ImGui::InputText("NPC", text, 100);
	if (ImGui::Button("ADD Element"))
		TestText(text);

}


void TestText(char* text)
{
	Msg("%s", text);
}