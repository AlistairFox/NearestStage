#include "StdAfx.h"
#include"embedded_editor_admin.h"
#include"embedded_editor_helper.h"
#include "../xrEngine/xr_ioconsole.h"
#include <imgui.h>
#include "CustomOutfit.h"


bool godmode = false;
bool noclip = false;
bool unlammo = false;
bool invis = false;
bool settime = false;
float time1 = 0.f;

void ShowAdminMenu(bool& show)
{
	ImguiWnd wnd("ADMIN Menu", &show);
	if (wnd.Collapsed)
		return;

	ImGui::Text(u8"Admin Functions");

	if (godmode)
	{
		Console->Execute("adm_god_mode 1");
	}
	else
		Console->Execute("adm_god_mode 0");

	if (noclip)
		Console->Execute("adm_no_clip 1");
	else
		Console->Execute("adm_no_clip 0");

	if (unlammo)
		Console->Execute("adm_unlimated_ammo 1");
	else
		Console->Execute("adm_unlimated_ammo 0");

	if (invis)
		Console->Execute("adm_invis 1");
	else
		Console->Execute("adm_invis 0");


	ImGui::Checkbox("GodMode", &godmode);

	ImGui::Checkbox("NoClip", &noclip);

	ImGui::Checkbox("Unlimited Ammo", &unlammo);

	ImGui::Checkbox("Invis Mode", &invis);

	/*
	if (ImGui::Button("Set EnvTime"))
		settime = !settime;
	*/

	ImGui::SliderFloat("Set EnvTime", &time1, 0.f, 24.f);

	if (ImGui::Button("SetTime"))
	{
		std::string str = "ra sv_setenvtime ";
		str += std::to_string(time1);
		Console->Execute(str.c_str());
	}



	/*
	string256 str;
	sprintf(str, "ra sv_setenvtime %f, %f, pizdecrazrabotka, %d, %llu", &time1, time2);
	*/
}